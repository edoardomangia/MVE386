/* 
 * src/DetectorConstruction.cc
 * Builds geometry and materials
 */

#include "DetectorConstruction.hh"
#include "CADMesh.hh"

#include <memory>
#include "G4Box.hh"
#include "G4Material.hh"
#include "G4NistManager.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4MultiFunctionalDetector.hh"
#include "G4SDManager.hh"
#include "G4PSDoseDeposit.hh"

#include <cctype>
#include <algorithm>
#include <fstream>
#include <map>
#include <stack>


namespace {
struct STLBounds {
    bool ok = false;
    G4ThreeVector min;
    G4ThreeVector max;
};

// Read a binary STL (and fall back to ASCII) to estimate bounds.
STLBounds ComputeSTLBounds(const std::string& path)
{
    STLBounds b;
    std::ifstream f(path, std::ios::binary);
    if (!f) return b;

    f.seekg(0, std::ios::end);
    std::streamsize fileSize = f.tellg();
    f.seekg(0, std::ios::beg);

    char header[80];
    f.read(header, 80);

    uint32_t nTriangles = 0;
    f.read(reinterpret_cast<char*>(&nTriangles), sizeof(uint32_t));

    std::streamsize expectedSize = 84 + static_cast<std::streamsize>(nTriangles) * 50;
    auto accumulateVertex = [&](double x, double y, double z) {
        if (!b.ok) {
            b.min = {x, y, z};
            b.max = {x, y, z};
            b.ok = true;
        } else {
            b.min.setX(std::min(b.min.x(), x));
            b.min.setY(std::min(b.min.y(), y));
            b.min.setZ(std::min(b.min.z(), z));
            b.max.setX(std::max(b.max.x(), x));
            b.max.setY(std::max(b.max.y(), y));
            b.max.setZ(std::max(b.max.z(), z));
        }
    };

    if (fileSize >= expectedSize) {
        // Likely binary STL
        for (uint32_t i = 0; i < nTriangles && f; ++i) {
            float normal[3];
            float verts[9];
            uint16_t attr;
            f.read(reinterpret_cast<char*>(normal), sizeof(normal));
            f.read(reinterpret_cast<char*>(verts), sizeof(verts));
            f.read(reinterpret_cast<char*>(&attr), sizeof(attr));
            for (int v = 0; v < 3; ++v) {
                double x = verts[3*v + 0];
                double y = verts[3*v + 1];
                double z = verts[3*v + 2];
                accumulateVertex(x, y, z);
            }
        }
        if (b.ok) return b;
    }

    // Fallback: ASCII parser
    f.clear();
    f.seekg(0, std::ios::beg);
    std::string word;
    while (f >> word) {
        if (word == "vertex") {
            double x, y, z;
            f >> x >> y >> z;
            accumulateVertex(x, y, z);
        }
    }
    return b;
}

// Tiny chemical formula expander (supports parentheses and integer counts)
// Returns element -> atom count
static std::map<std::string, int> ExpandFormula(const std::string& f)
{
    std::stack<std::map<std::string, int>> st;
    st.push({});

    auto parseInt = [&](size_t& i) {
        int val = 0;
        while (i < f.size() && std::isdigit(static_cast<unsigned char>(f[i]))) {
            val = val * 10 + (f[i] - '0');
            ++i;
        }
        return val == 0 ? 1 : val;
    };

    for (size_t i = 0; i < f.size();) {
        char c = f[i];
        if (c == '(') {
            st.push({});
            ++i;
        } else if (c == ')') {
            ++i;
            int mult = parseInt(i);
            auto top = st.top(); st.pop();
            for (auto& kv : top) st.top()[kv.first] += kv.second * mult;
        } else if (std::isupper(static_cast<unsigned char>(c))) {
            std::string el(1, c);
            ++i;
            if (i < f.size() && std::islower(static_cast<unsigned char>(f[i]))) {
                el += f[i++];
            }
            int mult = parseInt(i);
            st.top()[el] += mult;
        } else {
            ++i; // ignore anything unexpected
        }
    }
    return st.top();
}
}

G4VPhysicalVolume* DetectorConstruction::Construct()
{
    auto& beam = config.beam;
    auto& obj  = config.object;

    // World size: span source to detector (+ some margin) along x
    double srcX = beam.source_pos_mm[0];
    double detX = beam.detector_pos_mm[0];
    double minX = std::min(srcX, detX);
    double maxX = std::max(srcX, detX);
    double halfX = 0.5 * (maxX - minX + 100.0); // +100 mm margin

    double halfY = 150.0; // mm, arbitrary generous
    double halfZ = 150.0;

    auto* nist = G4NistManager::Instance();
    auto* air = nist->FindOrBuildMaterial("G4_AIR");

    auto* worldSolid = new G4Box("World",
                                 halfX*mm, halfY*mm, halfZ*mm);
    auto* worldLV = new G4LogicalVolume(worldSolid, air, "WorldLV");
    auto* worldPV = new G4PVPlacement(
        nullptr, {}, worldLV, "WorldPV", nullptr, false, 0, true);

    // Material from setup.JSON chemical formula
    auto* objectMat = [&]() -> G4Material* {
        double density = obj.material.density_g_cm3 * g/cm3;

        // Reuse if already constructed (Geant4 keeps a materials table)
        if (auto* existing = G4Material::GetMaterial("ModelMat", false)) {
            return existing;
        }

        auto atoms = ExpandFormula(obj.material.formula);

        auto* m = new G4Material("ModelMat", density, atoms.size());
        for (const auto& kv : atoms) {
            auto* el = nist->FindOrBuildElement(kv.first);
            m->AddElement(el, kv.second);
        }
        return m;
    }();

    // Mesh via CADMesh
    // Use Assimp reader so binary STL (and other formats) are accepted
    auto mesh = CADMesh::TessellatedMesh::FromSTL(
        obj.mesh_path,
        std::make_shared<CADMesh::File::ASSIMPReader>());

    // Handle units from JSON ("mm", "cm", "m", ...)
    double unitScale = 1.0;           // CADMesh expects mm by default
    if (obj.units == "cm") {
        unitScale = 10.0;             // 1 cm = 10 mm
    } else if (obj.units == "m") {
        unitScale = 1000.0;           // 1 m = 1000 mm
    } else {
        unitScale = 1.0;              // assume mm
    }

    // Fit model into the voxel cube so ParaView shows shape properly
    double fitScale = 1.0;
    G4ThreeVector translation(0, 0, 0);
    auto bounds = ComputeSTLBounds(obj.mesh_path);
    if (bounds.ok) {
        G4ThreeVector extent = bounds.max - bounds.min;
        double maxDim = std::max({extent.x(), extent.y(), extent.z()});
        if (maxDim > 0.0) {
            double targetSize = 2.0 * config.voxel_grid.half_size_mm * 0.9; // leave a margin
            double maxDim_mm = maxDim * unitScale;
            fitScale = targetSize / maxDim_mm;
            G4ThreeVector center = (bounds.min + bounds.max) * 0.5;
            G4ThreeVector center_mm = center * unitScale * fitScale;
            translation = -center_mm * mm; // bring mesh center to origin
        }
    }

    mesh->SetScale(unitScale * fitScale);

    auto* modelSolid = mesh->GetSolid();
    auto* modelLV    = new G4LogicalVolume(modelSolid, objectMat, "ModelLV");

    // Place model at computed position; CADMesh keeps units in mm
    new G4PVPlacement(
        nullptr, translation, modelLV,
        "ModelPV", worldLV, false, 0, true);

    return worldPV;
}

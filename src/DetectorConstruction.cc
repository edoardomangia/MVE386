/* 
 * src/DetectorConstruction.cc
 * Builds geometry and materials
 */

#include "DetectorConstruction.hh"
#include "CADMesh.hh"

#include "G4Box.hh"
#include "G4NistManager.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4MultiFunctionalDetector.hh"
#include "G4SDManager.hh"
#include "G4PSDoseDeposit.hh"

G4VPhysicalVolume* DetectorConstruction::Construct()
{
auto& beam = config.beam;
    auto& obj  = config.dragon;

    // --- World size: span source to detector (+ some margin) along x ---
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

    // Material: hard-code from formula in JSON (hydroxyapatite)
    // Ca10(PO4)6(OH)2 -> Ca:10, P:6, O:26, H:2
    auto* dragonMat = [&]() -> G4Material* {
        double density = obj.material.density_g_cm3 * g/cm3;

        G4Element* Ca = nist->FindOrBuildElement("Ca");
        G4Element* P  = nist->FindOrBuildElement("P");
        G4Element* O  = nist->FindOrBuildElement("O");
        G4Element* H  = nist->FindOrBuildElement("H");

        auto* m = new G4Material("DragonMat", density, 4);
        m->AddElement(Ca, 10);
        m->AddElement(P,  6);
        m->AddElement(O, 26);
        m->AddElement(H,  2);
        return m;
    }();

    // Mesh via CADMesh
    auto mesh = CADMesh::TessellatedMesh::FromSTL(obj.mesh_path);

    // Handle units from JSON ("mm", "cm", "m", ...)
    double scale = 1.0;           // CADMesh expects mm by default
    if (obj.units == "cm") {
        scale = 10.0;             // 1 cm = 10 mm
    } else if (obj.units == "m") {
        scale = 1000.0;           // 1 m = 1000 mm
    } else {
        scale = 1.0;              // assume mm
    }
    mesh->SetScale(scale);

    auto* dragonSolid = mesh->GetSolid();
    auto* dragonLV    = new G4LogicalVolume(dragonSolid, dragonMat, "DragonLV");

    // Place dragon at origin; CADMesh keeps units in mm
    new G4PVPlacement(
        nullptr, G4ThreeVector(), dragonLV,
        "DragonPV", worldLV, false, 0, true);

    return worldPV;
}

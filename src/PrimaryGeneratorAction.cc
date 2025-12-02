/* src/PrimaryGeneratorAction.cc
 * What particles we shoot
 */

#include "PrimaryGeneratorAction.hh"

#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4SystemOfUnits.hh"
#include "G4Event.hh"
#include "G4ThreeVector.hh"

#include <algorithm>
#include <random>
#include <cmath>

PrimaryGeneratorAction::PrimaryGeneratorAction(const SceneConfig& cfg)
    : config(cfg)
{
    fParticleGun = new G4ParticleGun(1);

    auto* particleTable = G4ParticleTable::GetParticleTable();
    auto* gamma = particleTable->FindParticle("gamma");
    fParticleGun->SetParticleDefinition(gamma);

    fParticleGun->SetParticleEnergy(config.beam.mono_energy_keV * keV);
}

PrimaryGeneratorAction::~PrimaryGeneratorAction()
{
    delete fParticleGun;
}

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event)
{
    const auto& b = config.beam;
    const auto& a = config.acquisition;

    auto rotate = [](const G4ThreeVector& v, const G4ThreeVector& axisUnit, double angleRad) {
        // Rodrigues rotation formula
        double c = std::cos(angleRad);
        double s = std::sin(angleRad);
        return v * c + axisUnit.cross(v) * s + axisUnit * (axisUnit.dot(v)) * (1.0 - c);
    };

    // Compute projection angle based on acquisition mode
    auto angle_deg = [&]() {
        double span = a.end_angle_deg - a.start_angle_deg;
        int totalEvents = std::max(1, a.total_events);
        if (a.mode == "fly") {
            double frac = 0.0;
            if (totalEvents > 1) {
                frac = std::min(1.0, event->GetEventID() / static_cast<double>(totalEvents - 1));
            }
            return a.start_angle_deg + frac * span;
        }
        // default: step-and-shoot
        int projections = std::max(1, a.num_projections);
        int eventsPerProj = std::max(1, totalEvents / projections);
        int projIdx = event->GetEventID() / eventsPerProj;
        projIdx = std::min(projections - 1, projIdx);
        double frac = projections > 1 ? projIdx / static_cast<double>(projections - 1) : 0.0;
        return a.start_angle_deg + frac * span;
    }();

    double angle_rad = angle_deg * deg;

    G4ThreeVector axis(a.rotation_axis[0], a.rotation_axis[1], a.rotation_axis[2]);
    if (axis.mag2() == 0.0) axis = G4ThreeVector(0, 0, 1);
    axis = axis.unit();

    G4ThreeVector pivot(a.rotation_center_mm[0]*mm,
                        a.rotation_center_mm[1]*mm,
                        a.rotation_center_mm[2]*mm);

    // Rotate beamline around pivot; equivalent to rotating the sample
    G4ThreeVector src0(b.source_pos_mm[0]*mm,
                       b.source_pos_mm[1]*mm,
                       b.source_pos_mm[2]*mm);
    G4ThreeVector det0(b.detector_pos_mm[0]*mm,
                       b.detector_pos_mm[1]*mm,
                       b.detector_pos_mm[2]*mm);
    G4ThreeVector up0(b.detector_up[0], b.detector_up[1], b.detector_up[2]);

    auto rotateAboutPivot = [&](const G4ThreeVector& v) {
        return pivot + rotate(v - pivot, axis, angle_rad);
    };

    G4ThreeVector src = rotateAboutPivot(src0);
    G4ThreeVector det = rotateAboutPivot(det0);
    G4ThreeVector up  = rotate(up0, axis, angle_rad).unit();

    G4ThreeVector dir = (det - src).unit();
    fParticleGun->SetParticleMomentumDirection(dir);

    // Uniform beam cross-section: sample within detector area
    static thread_local std::mt19937 rng{12345};
    double sx = b.detector_pixel_size_mm[0] * b.detector_pixels[0]; // full size in mm
    double sy = b.detector_pixel_size_mm[1] * b.detector_pixels[1];
    
    /*
     * With 2048px x 0.05mm, beam footprint ~102mm wide/thread_local
     * > larger than the default 20mm voxel cube
     * > cube is fully illuminated
     * > lots of photons miss for nothing
     */

    std::uniform_real_distribution<double> ux(-0.5*sx*mm, 0.5*sx*mm);
    std::uniform_real_distribution<double> uy(-0.5*sy*mm, 0.5*sy*mm);

    // Build orthonormal basis (dir, u_hat, v_hat)
    G4ThreeVector u_hat = up.cross(dir).unit();
    G4ThreeVector v_hat = dir.cross(u_hat).unit();

    double u = ux(rng);
    double v = uy(rng);

    G4ThreeVector pos = src + u*u_hat + v*v_hat;

    fParticleGun->SetParticlePosition(pos);
    fParticleGun->GeneratePrimaryVertex(event);
}

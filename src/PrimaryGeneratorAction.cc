/* src/PrimaryGeneratorAction.cc
 * What particles we shoot
 */
#include "PrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4SystemOfUnits.hh"
#include "G4Event.hh"
#include "G4ThreeVector.hh"
#include <random>

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

    // Direction: from source to detector (parallel beam)
    G4ThreeVector src(b.source_pos_mm[0]*mm,
                      b.source_pos_mm[1]*mm,
                      b.source_pos_mm[2]*mm);
    G4ThreeVector det(b.detector_pos_mm[0]*mm,
                      b.detector_pos_mm[1]*mm,
                      b.detector_pos_mm[2]*mm);

    G4ThreeVector dir = (det - src).unit();
    fParticleGun->SetParticleMomentumDirection(dir);

    // Simple uniform beam cross-section: sample within detector area
    static thread_local std::mt19937 rng{12345};
    double sx = b.detector_pixel_size_mm[0] * b.detector_pixels[0]; // full size in mm
    double sy = b.detector_pixel_size_mm[1] * b.detector_pixels[1];

    std::uniform_real_distribution<double> ux(-0.5*sx*mm, 0.5*sx*mm);
    std::uniform_real_distribution<double> uy(-0.5*sy*mm, 0.5*sy*mm);

    // Build orthonormal basis (dir, u_hat, v_hat)
    G4ThreeVector up(b.detector_up[0],
                     b.detector_up[1],
                     b.detector_up[2]);
    up = up.unit();

    G4ThreeVector u_hat = up.cross(dir).unit();
    G4ThreeVector v_hat = dir.cross(u_hat).unit();

    double u = ux(rng);
    double v = uy(rng);

    G4ThreeVector pos = src + u*u_hat + v*v_hat;

    fParticleGun->SetParticlePosition(pos);
    fParticleGun->GeneratePrimaryVertex(event);
}


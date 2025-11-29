#include "PMPrimaryGenerator.hh"

PMPrimaryGenerator::PMPrimaryGenerator()
{

    // TODO: Change to from ParticleGun to G4GeneralParticleSource

    G4int n_particle = 1;
    fParticleGun = new G4ParticleGun(n_particle);

    // Particle position
    // OBS! One could consider randomizing the position to simulate a parallel beam?
    G4double x = 0. * m;
    G4double y = 0. * m;
    G4double z = -50 * cm;

    G4ThreeVector pos(x, y, z);

    // Particle direction (momentum)
    // OBS! One could consider randomizing directions for a beam with divergence
    G4double px = 0.;
    G4double py = 0.;
    G4double pz = 1.;

    G4ThreeVector mom(px, py, pz);

    // Particle type
    G4ParticleTable *particleTable = G4ParticleTable::GetParticleTable();
    G4ParticleDefinition *particle = particleTable->FindParticle("gamma"); // X-ray (gamma) photon

    fParticleGun->SetParticlePosition(pos);
    fParticleGun->SetParticleMomentumDirection(mom);
    fParticleGun->SetParticleEnergy(50. * keV); // X-ray energy range: 100 eV - 100 keV (soft to hard X-rays)
    // fParticleGun->SetParticleEnergy(1. * MeV);
    fParticleGun->SetParticleDefinition(particle);
}

PMPrimaryGenerator::~PMPrimaryGenerator()
{
    delete fParticleGun;
}

void PMPrimaryGenerator::GeneratePrimaries(G4Event *anEvent)
{
    // Create vertex
    fParticleGun->GeneratePrimaryVertex(anEvent);
}
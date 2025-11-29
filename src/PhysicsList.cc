/* 
 * src/PhysicsList.cc
 * Defines physics processes
 */
#include "PhysicsList.hh"

#include "G4EmLivermorePhysics.hh"
#include "G4EmParameters.hh"
#include "G4SystemOfUnits.hh"

PhysicsList::PhysicsList()
{
    // Default cut values 
    defaultCutValue = 0.1*mm;
    SetVerboseLevel(1);

    // Electromagnetic physics
    RegisterPhysics(new G4EmLivermorePhysics());

    // Enable detailed atomic de-excitation
    auto* emParams = G4EmParameters::Instance();
    emParams->SetFluo(true);
    emParams->SetAuger(true);
    emParams->SetAugerCascade(true);
    emParams->SetPixe(true);   
}

/* G4EmLivermorePhysics:
 *  - Low-energy electromagnetic model, good for X-rays down to a few keV
 *  - Photoelectric effect
 *  - Compton scattering
 *  - Rayleigh scattering
 *  - Bremsstrahlung, pair production 
 *  - Electron multiple scattering
 */

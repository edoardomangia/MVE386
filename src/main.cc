/*
 * src/main.cc
 */

#include "G4RunManagerFactory.hh"
#include "G4UImanager.hh"
#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"

#include "DetectorConstruction.hh"
#include "PhysicsList.hh"
#include "ActionInitialization.hh"
#include "SceneConfig.hh"



int main(int argc, char** argv)
{
    // Number of events 
    G4int nEvents = 1e5;
    std::string configPath = "setup.json";

    if (argc > 1) nEvents = std::atoi(argv[1]);
    if (argc > 2) configPath = argv[2];
    
    SceneConfig cfg = SceneConfig::Load(configPath);

    // Create run manager 
    auto* runManager =
        // G4RunManagerFactory::CreateRunManager(G4RunManagerType::Default);
        G4RunManagerFactory::CreateRunManager(G4RunManagerType::Serial);

    // User initializations
    runManager->SetUserInitialization(new DetectorConstruction(cfg));
    runManager->SetUserInitialization(new PhysicsList());
    runManager->SetUserInitialization(new ActionInitialization(cfg));

    // Initialize Geant4 kernel
    runManager->Initialize();

    // Run N events, aka N primary photons through the setup
    runManager->BeamOn(nEvents);

    delete runManager;
    return 0;
}


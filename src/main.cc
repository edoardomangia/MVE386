/*
 * src/main.cc
 */

#include "DetectorConstruction.hh"
#include "PhysicsList.hh"
#include "ActionInitialization.hh"
#include "SceneConfig.hh"

#include "G4RunManagerFactory.hh"
#include "G4UImanager.hh"
#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"
#include <chrono>
#include <filesystem>
#include <iostream>

int main(int argc, char** argv)
{
    // Number of events, aka shoot N primary photons through the setup
    G4int nEvents = 1e5;
    std::filesystem::path configPath;

    // Prefer the real project root (parent of the executable dir) for defaults
    auto exePath = std::filesystem::canonical(argv[0]);
    auto exeDir = exePath.parent_path();
    auto projectRoot = exeDir.parent_path();
    std::filesystem::path defaultConfig = projectRoot / "setup.json";
    if (!std::filesystem::exists(defaultConfig)) {
        defaultConfig = "setup.json";
    }

    if (argc > 1) nEvents = std::atoi(argv[1]);
    if (argc > 2) {
        configPath = argv[2];
    } else {
        configPath = defaultConfig;
    }
    
    SceneConfig cfg = SceneConfig::Load(configPath.string());

    // Create run manager 
    auto* runManager =
        // TODO Setup multithreading, now each is writing a .vti 
        // G4RunManagerFactory::CreateRunManager(G4RunManagerType::Default);
        G4RunManagerFactory::CreateRunManager(G4RunManagerType::Serial);

    // User initializations
    runManager->SetUserInitialization(new DetectorConstruction(cfg));
    runManager->SetUserInitialization(new PhysicsList());
    runManager->SetUserInitialization(new ActionInitialization(cfg));

    // Initialize Geant4 kernel
    runManager->Initialize();

    auto wall_start = std::chrono::steady_clock::now();

    // Run N events    
    runManager->BeamOn(nEvents);

    auto wall_end = std::chrono::steady_clock::now();
    double elapsed_s = std::chrono::duration_cast<std::chrono::duration<double>>(wall_end - wall_start).count();
    std::cout << "Run completed in " << elapsed_s << " s" << std::endl;

    delete runManager;
    return 0;
}

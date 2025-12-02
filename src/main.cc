/*
 * src/main.cc
 */

#include "ActionInitialization.hh"
#include "DetectorConstruction.hh"
#include "PhysicsList.hh"
#include "SceneConfig.hh"

#include "G4RunManagerFactory.hh"
#include "G4Threading.hh"
#include "G4UIExecutive.hh"
#include "G4UImanager.hh"
#include "G4VisExecutive.hh"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>

int main(int argc, char **argv) {
  auto programStart = std::chrono::steady_clock::now();

  // Number of events, aka shoot N primary photons through the setup
  // Derived from photon_flux_per_s * exposure_time_s
  G4int nEvents = 0;
  std::filesystem::path configPath;

  // Prefer real project root
  auto exePath = std::filesystem::canonical(argv[0]);
  auto exeDir = exePath.parent_path();
  auto projectRoot = exeDir.parent_path();
  std::filesystem::path defaultConfig = projectRoot / "setup.json";
  if (!std::filesystem::exists(defaultConfig)) {
    defaultConfig = "setup.json";
  }

  if (argc > 2) {
    configPath = argv[2];
  } else {
    configPath = defaultConfig;
  }
  SceneConfig cfg = SceneConfig::Load(configPath.string());
  // Default event count from flux * exposure; for step mode
  auto photonsPerProjection =
      cfg.beam.photon_flux_per_s * cfg.beam.exposure_time_s;
  double totalPhotons = photonsPerProjection;
  if (cfg.acquisition.mode != "fly") {
    totalPhotons *= std::max(1, cfg.acquisition.num_projections);
  }

  if (argc > 1) {
    nEvents = std::atoi(argv[1]);
  } else {
    auto suggested = static_cast<long long>(std::llround(totalPhotons));
    auto maxAllowed = std::numeric_limits<G4int>::max();
    if (suggested > maxAllowed) {
      std::cerr
          << "[warn] Requested photons from flux exceed G4int; clamping to "
          << maxAllowed << " (" << suggested << " requested)\n";
      nEvents = maxAllowed;
    } else if (suggested <= 0) {
      nEvents = 0;
    } else {
      nEvents = static_cast<G4int>(suggested);
    }
  }

  cfg.acquisition.total_events = nEvents;

  // Create run manager
  auto *runManager =
      G4RunManagerFactory::CreateRunManager(G4RunManagerType::Default);

  // Configure threads, env G4NUM_THREADS overrides auto-detect
  int nThreads = G4Threading::G4GetNumberOfCores();
  if (const char *env = std::getenv("G4NUM_THREADS")) {
    int v = std::atoi(env);
    if (v > 0)
      nThreads = v;
  }
  runManager->SetNumberOfThreads(nThreads);

  // User initializations
  runManager->SetUserInitialization(new DetectorConstruction(cfg));
  runManager->SetUserInitialization(new PhysicsList());
  runManager->SetUserInitialization(new ActionInitialization(cfg));

  // Initialize Geant4 kernel
  runManager->Initialize();
  runManager->BeamOn(nEvents);

  auto programEnd = std::chrono::steady_clock::now();
  double total_s = std::chrono::duration_cast<std::chrono::duration<double>>(
                       programEnd - programStart)
                       .count();

  // Info on the run
  std::cout << "\n --- Summary --- \n";
  std::cout << "  Total time           : " << total_s << " s\n";
  std::cout << "  Threads              : " << nThreads << "\n";
  std::cout << "  Events               : " << nEvents << "\n";
  std::cout << "  Beam energy          : " << cfg.beam.mono_energy_keV
            << " keV\n";
  std::cout << "  Beam flux            : " << cfg.beam.photon_flux_per_s
            << " ph/s\n";
  std::cout << "  Beam exposure        : " << cfg.beam.exposure_time_s
            << " s\n";
  std::cout << "  Detector             : " << cfg.beam.detector_pixels[0] << "x"
            << cfg.beam.detector_pixels[1] << " px @ "
            << cfg.beam.detector_pixel_size_mm[0] << "x"
            << cfg.beam.detector_pixel_size_mm[1] << " mm\n";
  std::cout << "  Acquisition          : " << cfg.acquisition.mode << " ("
            << cfg.acquisition.start_angle_deg << " -> "
            << cfg.acquisition.end_angle_deg
            << " deg, projections=" << cfg.acquisition.num_projections << ")\n";
  std::cout << "  Voxel grid           : " << cfg.voxel_grid.nx << "x"
            << cfg.voxel_grid.ny << "x" << cfg.voxel_grid.nz
            << " in a cube half-size " << cfg.voxel_grid.half_size_mm
            << " mm\n";
  std::cout << "  Output dir           : " << cfg.output_dir << "\n";

  delete runManager;
  return 0;
}

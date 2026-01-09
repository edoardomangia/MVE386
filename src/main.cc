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
#include "PrimaryGeneratorAction.hh"
#include "RunAction.hh"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <vector>

int main(int argc, char **argv) {
  auto programStart = std::chrono::steady_clock::now();

  // Number of events, aka shoot N primary photons through the setup
  // Derived from photon_flux_per_s * exposure_time_s
  std::filesystem::path configPath;

  // Prefer real project root
  auto exePath = std::filesystem::canonical(argv[0]);
  auto exeDir = exePath.parent_path();
  auto projectRoot = exeDir.parent_path();
  std::filesystem::path defaultConfig = projectRoot / "setups" / "setup.json";
  if (!std::filesystem::exists(defaultConfig)) {
    defaultConfig = std::filesystem::path("setups") / "setup.json";
  }

  std::optional<long long> cliEvents;
  std::optional<std::filesystem::path> cliConfig;
  std::vector<std::string> positionals;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--setup") {
      if (i + 1 < argc) {
        cliConfig = std::filesystem::path(argv[++i]);
      }
      continue;
    }
    if (arg == "--events") {
      if (i + 1 < argc) {
        cliEvents = std::strtoll(argv[++i], nullptr, 10);
      }
      continue;
    }
    if (arg == "--help") {
      std::cout << "Usage: ./run [--events N] [--setup PATH]\n"
                   "       ./run [N] [PATH] (positional) \n";
      return 0;
    }
    if (!arg.empty() && arg[0] == '-') {
      std::cerr << "Unknown option: " << arg << "\n";
      return 1;
    }
    positionals.push_back(arg);
  }

  if (!cliConfig && positionals.size() > 1) {
    cliConfig = std::filesystem::path(positionals[1]);
  }
  if (!cliEvents && positionals.size() > 0) {
    cliEvents = std::strtoll(positionals[0].c_str(), nullptr, 10);
  }

  if (cliConfig) {
    configPath = *cliConfig;
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

  long long targetEvents = 0;
  if (cliEvents.has_value()) {
    targetEvents = *cliEvents;
  } else {
    auto suggested = static_cast<long long>(std::llround(totalPhotons));
    if (suggested > 0) {
      targetEvents = suggested;
    }
  }
  if (targetEvents < 0)
    targetEvents = 0;
  cfg.acquisition.total_events = targetEvents;

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
  const auto maxG4Events =
      static_cast<long long>(std::numeric_limits<G4int>::max());
  long long chunkSize = maxG4Events;
  if (const char *env = std::getenv("G4_CHUNK_EVENTS")) {
    long long requested = std::strtoll(env, nullptr, 10);
    if (requested > 0) {
      chunkSize = std::min(requested, maxG4Events);
    }
  }
  if (targetEvents > 0 && chunkSize > targetEvents) {
    chunkSize = targetEvents;
  }

  long long eventOffset = 0;
  if (targetEvents <= 0) {
    RunAction::SetIsFinalChunk(true);
    PrimaryGeneratorAction::SetEventOffset(0);
    runManager->BeamOn(0);
  } else {
    int chunkIndex = 0;
    while (eventOffset < targetEvents) {
      long long remaining = targetEvents - eventOffset;
      auto thisChunk =
          static_cast<G4int>(std::min<long long>(chunkSize, remaining));
      RunAction::SetIsFinalChunk(remaining <= chunkSize);
      PrimaryGeneratorAction::SetEventOffset(eventOffset);
      runManager->BeamOn(thisChunk);
      eventOffset += thisChunk;
      ++chunkIndex;
    }
  }

  auto programEnd = std::chrono::steady_clock::now();
  double total_s = std::chrono::duration_cast<std::chrono::duration<double>>(
                       programEnd - programStart)
                       .count();

  // Info on the run
  std::cout << " --- Energy --- \n \n";
  std::cout << "Total time           : " << total_s << " s\n";
  std::cout << "Threads              : " << nThreads << "\n";
  std::cout << "Events               : " << targetEvents << "\n";
  // std::cout << "Flux                 : " << cfg.beam.photon_flux_per_s
  std::cout << "Flux                 : " << targetEvents << " ph/s\n";
  std::cout << "Exposure time        : " << cfg.beam.exposure_time_s << " s\n";
  std::cout << "Energy               : " << cfg.beam.mono_energy_keV
            << " keV\n";
  std::cout << "Detector             : " << cfg.beam.detector_pixels[0] << "x"
            << cfg.beam.detector_pixels[1] << " px @ "
            << cfg.beam.detector_pixel_size_mm[0] << "x"
            << cfg.beam.detector_pixel_size_mm[1] << " mm\n";
  std::cout << "Acquisition          : " << cfg.acquisition.mode << " ("
            << cfg.acquisition.start_angle_deg << " -> "
            << cfg.acquisition.end_angle_deg
            << "deg, projections=" << cfg.acquisition.num_projections << ")\n";
  std::cout << "Voxel grid size      : " << cfg.voxel_grid.nx << "x"
            << cfg.voxel_grid.ny << "x"
            << cfg.voxel_grid.nz
            // << " in a cube half-size " << cfg.voxel_grid.half_size_mm
            << " mm\n";
  std::cout << "\n";
  std::cout << "Output               : " << cfg.output_dir << "\n";

  delete runManager;
  return 0;
}

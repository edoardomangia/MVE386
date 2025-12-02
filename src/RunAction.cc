/* 
 * src/RunAction.cc
 * Summary of what we are running and logs
 */

#include "RunAction.hh"
#include "DoseVoxelGrid.hh"
#include "GenVTI.hh"
#include "SceneConfig.hh"

#include "G4Run.hh"
#include "G4SystemOfUnits.hh"

#include <vector>
#include <sstream>
#include <utility>
#include <filesystem>

RunAction::RunAction(const SceneConfig& cfg)
    : config(cfg)
{}

void RunAction::BeginOfRunAction(const G4Run* run)
{
    // Phantom from -half_size to +half_size in mm
    int NX = config.voxel_grid.nx;
    int NY = config.voxel_grid.ny;
    int NZ = config.voxel_grid.nz;

    float half = static_cast<float>(config.voxel_grid.half_size_mm);
    float xmin = -half, ymin = -half, zmin = -half;
    float dx = (2.0f * half) / NX;   // mm per voxel
    float dy = (2.0f * half) / NY;
    float dz = (2.0f * half) / NZ;

    DoseVoxelGrid::Instance().Initialize(NX, NY, NZ, xmin, ymin, zmin, dx, dy, dz);

}

void RunAction::EndOfRunAction(const G4Run* run)
{
    if (!IsMaster()) return; // Only master writes output

    auto& grid = DoseVoxelGrid::Instance();

    // Collect metadata for .vti file 
    std::vector<std::pair<std::string, std::string>> meta;
    
    meta.emplace_back("material_formula", 
            config.object.material.formula);
    
    meta.emplace_back("material_density_g_cm3", 
            std::to_string(config.object.material.density_g_cm3));
    
    meta.emplace_back("beam_mono_energy_keV", 
            std::to_string(config.beam.mono_energy_keV));
    
    meta.emplace_back("beam_photon_flux_per_s", 
            std::to_string(config.beam.photon_flux_per_s));
    
    meta.emplace_back("beam_exposure_time_s", 
            std::to_string(config.beam.exposure_time_s));
    
    meta.emplace_back("simulated_events", 
            std::to_string(run->GetNumberOfEvent()));

    std::filesystem::path outPath = std::filesystem::path(config.output_dir) / "dose.vti";

    VTIWriter::Write(outPath.string(),
                     grid.Data(),
                     grid.NX, grid.NY, grid.NZ,
                     grid.xmin, grid.ymin, grid.zmin,
                     grid.dx, grid.dy, grid.dz,
                     meta);
}

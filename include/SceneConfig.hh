/*
 * SceneConfig.hh
 *
 */
#pragma once
#include <string>
#include <array>

struct BeamConfig {
    std::string type;                     // "parallel" or "point"
    std::array<double,3> source_pos_mm;
    std::array<double,3> detector_pos_mm;
    std::array<double,3> detector_up;
    std::array<int,2>    detector_pixels;
    std::array<double,2> detector_pixel_size_mm;
    double mono_energy_keV;
    double photon_flux_per_s;
    double exposure_time_s;
};

struct ObjectMaterial {
    std::string formula;
    double density_g_cm3;
    double cp_J_kgK;
};

struct ObjectConfig {
    std::string id;
    std::string mesh_path;
    std::string units;        // "mm"
    ObjectMaterial material;
};

struct VoxelGridConfig {
    int nx = 100;
    int ny = 100;
    int nz = 100;
    double half_size_mm = 10.0; // Cube half-length; default [-10, +10] mm
};

struct AcquisitionConfig {
    std::string mode = "step";            // "step" (step-and-shoot) or "fly" (continuous)
    int    num_projections = 1;
    double start_angle_deg = 0.0;
    double end_angle_deg   = 360.0;
    std::array<double,3> rotation_axis    = {0.0, 0.0, 1.0};   // axis in world coords
    std::array<double,3> rotation_center_mm = {0.0, 0.0, 0.0}; // pivot point
    int total_events = 0;                 // filled in from CLI nEvents
};

struct SceneConfig {
    BeamConfig beam;
    ObjectConfig object;
    VoxelGridConfig voxel_grid;
    AcquisitionConfig acquisition;
    std::string config_dir;    // Absolute directory containing the config file
    std::string output_dir;    // Where to store simulation outputs

    static SceneConfig Load(const std::string& path);
};

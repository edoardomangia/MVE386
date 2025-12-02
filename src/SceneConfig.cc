/*
 * SceneConfig.cc
 * Setup based on setup.json
 */

#include "SceneConfig.hh"
#include "json.hpp"

#include <fstream>
#include <filesystem>

using json = nlohmann::json;

SceneConfig SceneConfig::Load(const std::string& path)
{
    std::filesystem::path cfgPath = std::filesystem::absolute(path);
    std::ifstream f(cfgPath);
    json j;
    f >> j;

    SceneConfig cfg;
    cfg.config_dir = cfgPath.parent_path().string();
    cfg.output_dir = (cfgPath.parent_path() / "output").string();

    auto jb = j["beam"];
    cfg.beam.type               = jb.value("type", "parallel");
    cfg.beam.source_pos_mm      = { jb["source_position_mm"][0],
                                    jb["source_position_mm"][1],
                                    jb["source_position_mm"][2] };
    cfg.beam.detector_pos_mm    = { jb["detector_position_mm"][0],
                                    jb["detector_position_mm"][1],
                                    jb["detector_position_mm"][2] };
    cfg.beam.detector_up        = { jb["detector_up"][0],
                                    jb["detector_up"][1],
                                    jb["detector_up"][2] };
    cfg.beam.detector_pixels    = { jb["detector_pixels"][0],
                                    jb["detector_pixels"][1] };
    cfg.beam.detector_pixel_size_mm = { jb["detector_pixel_size_mm"][0],
                                        jb["detector_pixel_size_mm"][1] };
    cfg.beam.mono_energy_keV    = jb["mono_energy_keV"];
    cfg.beam.photon_flux_per_s  = jb["photon_flux_per_s"];
    cfg.beam.exposure_time_s    = jb.value("exposure_time_s", 1.0);

    // Only one object in setup.JSON
    auto jo = j["objects"][0];
    cfg.object.id        = jo["id"];
    std::filesystem::path meshPath = jo["mesh_path"].get<std::string>();
    if (meshPath.is_relative()) {
        meshPath = cfgPath.parent_path() / meshPath;
    }
    cfg.object.mesh_path = meshPath.string();
    cfg.object.units     = jo.value("units", "mm");

    auto jm = jo["material"];
    cfg.object.material.formula        = jm["formula"];
    cfg.object.material.density_g_cm3  = jm["density_g_cm3"];
    cfg.object.material.cp_J_kgK       = jm["cp_J_kgK"];

    // Voxel grid config to keep Geant and ParaView in sync
    if (j.contains("voxel_grid")) {
        auto jvg = j["voxel_grid"];
        if (jvg.contains("counts")) {
            cfg.voxel_grid.nx = jvg["counts"][0];
            cfg.voxel_grid.ny = jvg["counts"][1];
            cfg.voxel_grid.nz = jvg["counts"][2];
        }
        cfg.voxel_grid.half_size_mm = jvg.value("half_size_mm", cfg.voxel_grid.half_size_mm);
    }

    // Acquisition / rotation setup
    if (j.contains("acquisition")) {
        auto ja = j["acquisition"];
        cfg.acquisition.mode = ja.value("mode", cfg.acquisition.mode);
        cfg.acquisition.num_projections = ja.value("num_projections", cfg.acquisition.num_projections);
        cfg.acquisition.start_angle_deg = ja.value("start_angle_deg", cfg.acquisition.start_angle_deg);
        cfg.acquisition.end_angle_deg   = ja.value("end_angle_deg", cfg.acquisition.end_angle_deg);
        if (ja.contains("rotation_axis")) {
            cfg.acquisition.rotation_axis = { ja["rotation_axis"][0],
                                               ja["rotation_axis"][1],
                                               ja["rotation_axis"][2] };
        }
        if (ja.contains("rotation_center_mm")) {
            cfg.acquisition.rotation_center_mm = { ja["rotation_center_mm"][0],
                                                   ja["rotation_center_mm"][1],
                                                   ja["rotation_center_mm"][2] };
        }
    }

    return cfg;
}

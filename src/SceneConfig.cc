/*
 * SceneConfig.cc
 * Setup based on setup.json
 */

#include "SceneConfig.hh"
#include <fstream>
#include "json.hpp"

using json = nlohmann::json;

SceneConfig SceneConfig::Load(const std::string& path)
{
    std::ifstream f(path);
    json j;
    f >> j;

    SceneConfig cfg;

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

    // Only one object in your JSON: Dragon
    auto jo = j["objects"][0];
    cfg.dragon.id        = jo["id"];
    cfg.dragon.mesh_path = jo["mesh_path"];
    cfg.dragon.units     = jo.value("units", "mm");

    auto jm = jo["material"];
    cfg.dragon.material.formula        = jm["formula"];
    cfg.dragon.material.density_g_cm3  = jm["density_g_cm3"];
    cfg.dragon.material.cp_J_kgK       = jm["cp_J_kgK"];

    return cfg;
}


#!/usr/bin/env python
"""
Utilities to configure gvxr scenes from setup.json-style configs.
"""
import json
import numpy as np
from gvxrPython3 import gvxr
# import xraylib

def load_config(path):
    with open(path, "r") as f:
        return json.load(f)

def setup_gvxr_scene(cfg):
    gvxr.createOpenGLContext()
    gvxr.setWindowBackGroundColour(1.0, 1.0, 1.0)
    gvxr.setZoom(2500.0)

    for obj in cfg["objects"]:
        obj_id = obj["id"]
        mesh_path = obj["mesh_path"]
        units = obj.get("units", "mm")

        gvxr.loadMeshFile(obj_id, mesh_path, units)
        
        if obj.get("move_to_centre", True):
            gvxr.moveToCentre(obj_id)
        
        # t = obj.get("translation_mm", [0.0, 0.0, 0.0])
        # gvxr.translateNode(obj_id, *t, "mm")

        # Material
        mat = obj["material"]
        formula = mat["formula"]          
        density = mat["density_g_cm3"]   

        gvxr.setCompound(obj_id, formula)
        gvxr.setDensity(obj_id, density, "g/cm3")

        colour = mat.get("colour_rgba", [1.0, 0.2, 0.2, 1.0])
        gvxr.setColour(obj_id, *colour)

    # Beam and detector
    beam = cfg["beam"]

    if beam.get("type", "parallel") == "parallel":
        gvxr.useParallelBeam()
    else:
        gvxr.usePointSource()

    src = beam["source_position_mm"]
    det = beam["detector_position_mm"]
    up  = beam["detector_up"]

    gvxr.setSourcePosition(*src, "mm")
    gvxr.setDetectorPosition(*det, "mm")
    gvxr.setDetectorUpVector(*up)

    nx, ny = beam["detector_pixels"]
    sx, sy = beam["detector_pixel_size_mm"]
    gvxr.setDetectorNumberOfPixels(nx, ny)
    gvxr.setDetectorPixelSize(sx, sy, "mm")

    if beam.get("mono", True):
        E = beam["mono_energy_keV"]
        flux = beam["photon_flux_per_s"] 
        gvxr.setMonoChromatic(E, "keV", flux)
    else:
        raise NotImplementedError("Polybeam not wired yet")

def run_gvxr(cfg_path, output_prefix="output/test"):
    cfg = load_config(cfg_path)
    setup_gvxr_scene(cfg)

    # Normal radiograph
    image = gvxr.computeXRayImage()
    image = np.array(image, dtype=np.float32)
    np.save(f"{output_prefix}_image.npy", image)


    nx, ny = cfg["beam"]["detector_pixels"]
    
    L_buffers = {}
    for obj in cfg["objects"]:
        obj_id = obj["id"]
      
        # Directly get the L-buffer for this object
        try:
            L = gvxr.computeLBuffer(obj_id)   # <-- THIS returns the 2D array
            L = np.array(L, dtype=np.float32)
      
            # Optional: unit sanity check (convert mm -> cm if needed)
            if np.max(L) < 0.05:  # suspiciously tiny for cm
                print(f"L-buffer for {obj_id} looks like mm, converting to cm")
                L = L * 0.1
      
            L_buffers[obj_id] = L
     
        except AttributeError as e:
            print(f"computeLBuffer failed for {obj_id}: {e}")
            continue
      
    if L_buffers:
        np.savez_compressed(
            f"{output_prefix}_lbuffers.npz",
            **L_buffers,
        ) 
 
    # Save beam + material info
    np.savez(
        f"{output_prefix}_meta.npz",
        detector_pixels=np.array(cfg["beam"]["detector_pixels"]),
        detector_pixel_size_mm=np.array(cfg["beam"]["detector_pixel_size_mm"]),
        energy_keV=np.array([cfg["beam"]["mono_energy_keV"]]),
        photon_flux_per_s=np.array([cfg["beam"]["photon_flux_per_s"]]),
        exposure_time_s=np.array([cfg["beam"].get("exposure_time_s", 1.0)]),
    )

if __name__ == "__main__":
    run_gvxr("setup.json")

#!/usr/bin/env python
# heat_gvxr.py
"""
Compute dose/temperature from gvxr L-buffers and optionally render the scene.
"""

import json
import numpy as np
import matplotlib.pyplot as plt
import xraylib 
from gvxrPython3 import gvxr
from gvxr_setup import setup_gvxr_scene



def load_config(path):
    with open(path, "r") as f:
        return json.load(f)

def compute_temperature(config_path, meta_path, lbuffers_path, output_prefix="output/test"):
    cfg = load_config(config_path)
    meta = np.load(meta_path)
    lbuffs = np.load(lbuffers_path)

    beam = cfg["beam"]
    objects = cfg["objects"]

    # Beam parameters
    E_keV = float(beam["mono_energy_keV"])
    flux  = float(beam["photon_flux_per_s"])    # photons / s / pixels
    t_exp = float(beam.get("exposure_time_s", 1.0)) # s

    # Photons per pixel
    N_phot = flux * t_exp   # photons / pixel

    # Photon energy in joules
    E_photon_J = E_keV * 1e3 * 1.60218e-19  # keV -> eV -> J

    # Incident energy per pixel (same for all pixels here)
    E_incident = N_phot * E_photon_J                # J / pixel

    # Detector geometry
    det_pixels = meta["detector_pixels"]            # [nx, ny]
    pix_size_mm = meta["detector_pixel_size_mm"]    # [sx, sy]
    sx_mm, sy_mm = map(float, pix_size_mm)
    A_pixel_cm2 = (sx_mm / 10.0) * (sy_mm / 10.0)   # mm -> cm; area in cm^2

    # For now, handle just the first object
    obj = objects[0]
    obj_id = obj["id"]
    mat   = obj["material"]
    formula = mat["formula"]                        # "Ca10(PO4)6(OH)2"
    rho_g_cm3 = float(mat["density_g_cm3"])         # g/cm3
    cp = float(mat["cp_J_kgK"])                     # J/kg/K

    # Get path length map for this object
    if obj_id not in lbuffs.files:
        raise RuntimeError(f"L-buffer for object '{obj_id}' not found in {lbuffers_path}")
    L = lbuffs[obj_id]  # 2D array, path length

    # TODO units of L??? gVXR's Beer-Lambert uses cm,
    L_cm = np.array(L, dtype=np.float64)

    # Get linear attenuation coefficient μ (cm^-1) at E_keV
    # Use xraylib's compound cross-section (cm^2/g)
    mu_mass = xraylib.CS_Total_CP(formula, E_keV)   # cm^2 / g
    mu_linear = mu_mass * rho_g_cm3                # (cm^2/g * g/cm^3) = cm^-1

    # Fraction absorbed along each ray
    # Avoid overflow: clamp argument or rely on numpy's exp
    f_abs = 1.0 - np.exp(-mu_linear * L_cm)      

    # Energy absorbed per pixel
    E_abs = E_incident * f_abs                     # J / pixel (2D)

    # Pixel column volume V = A_pixel * L (cm^3)
    V_cm3 = A_pixel_cm2 * L_cm                     # cm^3

    # Mass per pixel: ρ [g/cm^3] * V [cm^3] -> g; convert to kg
    m_kg = (rho_g_cm3 * V_cm3) / 1000.0            # kg

    # Avoid division by zero where L_cm = 0 (no material in that pixel)
    mask = m_kg > 0.0

    # Dose: D = E_abs / m (Gy = J/kg)
    D = np.zeros_like(E_abs)
    D[mask] = E_abs[mask] / m_kg[mask]

    # Temperature rise: ΔT = D / cp  (since D in J/kg)
    delta_T = np.zeros_like(D)
    delta_T[mask] = D[mask] / cp                   # K

    # Save
    np.save(f"{output_prefix}_dose_Gy.npy", D.astype(np.float32))
    np.save(f"{output_prefix}_deltaT_K.npy", delta_T.astype(np.float32))

    print(f"deltaT min/max (K): {delta_T.min():.3e} / {delta_T.max():.3e}")
    print("mu_linear:", mu_linear)
    print("L_cm stats:", L_cm.min(), L_cm.max(), L_cm.mean())
    print("E_incident:", E_incident)
    print("f_abs stats:", f_abs.min(), f_abs.max(), f_abs.mean())
    print("m_kg stats:", m_kg[m_kg > 0].min(), m_kg[m_kg > 0].max())
    print("D stats:", D.min(), D.max(), D.mean())
    print("deltaT stats:", delta_T.min(), delta_T.max(), delta_T.mean())


deltaT = np.load("output/test_deltaT_K.npy")
plt.imshow(deltaT, cmap="inferno")
plt.colorbar(label="deltaT [K]")
plt.show()


def show_scene(cfg_path):
    cfg = load_config(cfg_path)
    setup_gvxr_scene(cfg)

    # Show the 3D scene
    gvxr.displayScene()

    print("\n3D viewer controls")
    print("Mouse right button: rotate")
    print("Mouse wheel: zoom")
    print("Q or Esc: quit")
    print("B: toggle beam visibility")
    print("H: toggle detector visibility")
    print("W: wireframe/solid")

    gvxr.renderLoop()



if __name__ == "__main__":
    compute_temperature(
        "setups/setup.json",
        "output/test_meta.npz",
        "output/test_lbuffers.npz",
        output_prefix="output/test",
        )

    show_scene("setups/setup.json")

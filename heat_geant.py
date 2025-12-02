#!/usr/bin/env python
# heat_geant.py

"""
Convert Geant4 energy deposition (dose.vti) into dose (Gy) and temperature rise (K).

Usage:
  python heat_geant.py dose.vti n_events [setup.json] [output_prefix]
    dose.vti      VTI from the Geant4 run (cell data in keV per voxel)
    n_events      Number of simulated primaries used to generate dose.vti
    setup.json    Optional; defaults to setup.json next to the script
    output_prefix Optional; defaults to output/temp (dir must exist)

Assumptions:
- dose.vti stores energy deposition per voxel in keV.
- Beam flux and exposure come from setup.json; number of simulated events is provided on CLI.
- Material density and cp are taken from the first object in setup.json.

Outputs (under output_prefix):
- <prefix>_dose_Gy.npy    Dose per voxel (Gy)
- <prefix>_deltaT_K.npy   Temperature rise per voxel (K)
- <prefix>_deltaT.vti     ParaView-friendly VTI of deltaT with metadata
"""

import json
import sys
import numpy as np
import xml.etree.ElementTree as ET
import os
from typing import Tuple


def parse_vti(path: str):
    tree = ET.parse(path)
    root = tree.getroot()

    image = root.find("ImageData")
    if image is None:
        raise ValueError("No ImageData node in VTI")

    spacing = tuple(map(float, image.attrib["Spacing"].split()))
    origin = tuple(map(float, image.attrib["Origin"].split()))
    extent = tuple(map(int, image.attrib["WholeExtent"].split()))
    x0, x1, y0, y1, z0, z1 = extent
    # If data is stored as CellData, the number of cells is (x1 - x0), not +1
    cell_array = root.find(".//CellData/DataArray")
    point_array = root.find(".//PointData/DataArray")
    data_array = cell_array if cell_array is not None else point_array
    if data_array is None or data_array.text is None:
        raise ValueError("No DataArray with data in VTI")
    text = " ".join(data_array.text.strip().split())
    values = np.fromstring(text, sep=" ", dtype=np.float64)
    if cell_array is not None:
        nx = x1 - x0
        ny = y1 - y0
        nz = z1 - z0
    else:
        nx = x1 - x0 + 1
        ny = y1 - y0 + 1
        nz = z1 - z0 + 1
    if values.size != nx * ny * nz:
        raise ValueError(f"VTI data size mismatch: {values.size} vs {nx*ny*nz}")

    return values, (nx, ny, nz), origin, spacing


def write_vti(path: str, data: np.ndarray, dims: Tuple[int, int, int], origin, spacing, name="deltaT_K", metadata=None):
    nx, ny, nz = dims
    # Cell data: extent counts points, so upper bound is number of cells
    x0 = y0 = z0 = 0
    x1 = nx
    y1 = ny
    z1 = nz

    def fmt(v):
        return f"{v:.9g}"

    flat = data.ravel()
    with open(path, "w") as f:
        f.write('<?xml version="1.0"?>\n')
        f.write('<VTKFile type="ImageData" version="0.1" byte_order="LittleEndian">\n')
        f.write(f'  <ImageData WholeExtent="{x0} {x1} {y0} {y1} {z0} {z1}" ')
        f.write(f'Origin="{fmt(origin[0])} {fmt(origin[1])} {fmt(origin[2])}" ')
        f.write(f'Spacing="{fmt(spacing[0])} {fmt(spacing[1])} {fmt(spacing[2])}">\n')
        if metadata:
            f.write("    <FieldData>\n")
            for k, v in metadata.items():
                f.write(f'      <DataArray type="String" Name="{k}" format="ascii" NumberOfComponents="1">\n')
                f.write(f"        {v}\n")
                f.write("      </DataArray>\n")
            f.write("    </FieldData>\n")
        f.write(f'    <Piece Extent="{x0} {x1} {y0} {y1} {z0} {z1}">\n')
        f.write("      <PointData/>\n")
        f.write(f'      <CellData Scalars="{name}">\n')
        f.write(f'        <DataArray type="Float32" Name="{name}" format="ascii">\n')
        f.write(" ".join(fmt(v) for v in flat))
        f.write("\n        </DataArray>\n")
        f.write("      </CellData>\n")
        f.write("    </Piece>\n")
        f.write("  </ImageData>\n")
        f.write("</VTKFile>\n")


def main():
    if len(sys.argv) < 3:
        print("Usage: python heat_geant.py dose.vti n_events [setup.json] [output_prefix]")
        sys.exit(1)

    vti_path = sys.argv[1]
    if not os.path.exists(vti_path):
        alt = os.path.join("output", vti_path)
        if os.path.exists(alt):
            vti_path = alt
            print(f"Using VTI from {vti_path}")
        else:
            raise FileNotFoundError(f"VTI file not found at '{vti_path}' or '{alt}'")
    n_events = float(sys.argv[2])
    setup_path = sys.argv[3] if len(sys.argv) > 3 else "setup.json"
    prefix = sys.argv[4] if len(sys.argv) > 4 else "output/temp"

    data_keV, dims, origin, spacing = parse_vti(vti_path)

    cfg = json.load(open(setup_path))
    beam = cfg["beam"]
    obj = cfg["objects"][0]
    mat = obj["material"]

    flux = float(beam["photon_flux_per_s"])
    exposure = float(beam.get("exposure_time_s", 1.0))
    rho_g_cm3 = float(mat["density_g_cm3"])
    cp = float(mat["cp_J_kgK"])

    # Scale from simulated events to physical photons
    # scale = 1.0
    scale = (flux * exposure) / n_events

    # Energy to joules
    data_keV_scaled = data_keV * scale
    data_J = data_keV_scaled * 1e3 * 1.602176634e-19

    # Voxel volume and mass
    dx, dy, dz = spacing  # mm
    voxel_volume_m3 = (dx * 1e-3) * (dy * 1e-3) * (dz * 1e-3)
    rho_kg_m3 = rho_g_cm3 * 1000.0
    voxel_mass_kg = voxel_volume_m3 * rho_kg_m3

    dose_Gy = data_J / voxel_mass_kg  # J/kg
    deltaT_K = dose_Gy / cp

    np.save(f"{prefix}_dose_Gy.npy", dose_Gy.astype(np.float32))
    np.save(f"{prefix}_deltaT_K.npy", deltaT_K.astype(np.float32))
    meta = {
        "material_formula": obj["material"]["formula"],
        "material_density_g_cm3": rho_g_cm3,
        "beam_mono_energy_keV": beam["mono_energy_keV"],
        "beam_photon_flux_per_s": flux,
        "beam_exposure_time_s": exposure,
        "simulated_events": n_events,
        "voxel_cp_J_kgK": cp,
    }
    write_vti(f"{prefix}_deltaT.vti", deltaT_K.astype(np.float32), dims, origin, spacing, name="deltaT_K", metadata=meta)

    print(f"Wrote {prefix}_dose_Gy.npy, {prefix}_deltaT_K.npy, {prefix}_deltaT.vti")
    print(f"Assumed n_events={n_events}, flux={flux} ph/s, exposure={exposure} s, rho={rho_g_cm3} g/cm3, cp={cp} J/kg/K")


if __name__ == "__main__":
    main()

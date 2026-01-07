#!/usr/bin/env python
"""
Convert Geant4 energy deposition (dose.vti) into absorbed dose (Gy).

Usage:
  python dosage_geant.py dose.vti [setups/setup.json] [output_prefix]
    dose.vti           VTI from the Geant4 run (cell data in keV per voxel)
    setups/setup.json  Optional; defaults to setups/setup.json
    output_prefix      Optional; defaults to output/temp (dir must exist)

Assumptions:
- dose.vti stores energy deposition per voxel in keV.
- Material density is taken from the first object in setups/setup.json.

Outputs (under output_prefix):
- <prefix>_dose_Gy.npy   Dose per voxel (Gy)
- <prefix>_dose_Gy.vti   Same field as VTI (cell data)
"""

import json
import os
import sys
import numpy as np
import xml.etree.ElementTree as ET
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
        raise ValueError(f"VTI data size mismatch: {values.size} vs {nx * ny * nz}")

    return values, (nx, ny, nz), origin, spacing


def write_vti(path: str, data: np.ndarray, dims: Tuple[int, int, int], origin, spacing, name="dose_Gy", metadata=None):
    nx, ny, nz = dims
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
    if len(sys.argv) < 2:
        print("Usage: python dosage_geant.py dose.vti [setups/setup.json] [output_prefix]")
        sys.exit(1)

    vti_path = sys.argv[1]
    if not os.path.exists(vti_path):
        alt = os.path.join("output", vti_path)
        if os.path.exists(alt):
            vti_path = alt
            print(f"Using VTI from {vti_path}")
        else:
            raise FileNotFoundError(f"VTI file not found at '{vti_path}' or '{alt}'")

    if len(sys.argv) > 2:
        setup_path = sys.argv[2]
    else:
        setup_path = os.path.join("setups", "setup.json")
    prefix = sys.argv[3] if len(sys.argv) > 3 else "output/temp"

    data_keV, dims, origin, spacing = parse_vti(vti_path)

    cfg = json.load(open(setup_path))
    obj = cfg["objects"][0]
    mat = obj["material"]
    rho_g_cm3 = float(mat["density_g_cm3"])

    # Energy to joules
    data_J = data_keV * 1e3 * 1.602176634e-19

    # Voxel volume and mass
    dx, dy, dz = spacing  # mm
    voxel_volume_m3 = (dx * 1e-3) * (dy * 1e-3) * (dz * 1e-3)
    rho_kg_m3 = rho_g_cm3 * 1000.0
    voxel_mass_kg = voxel_volume_m3 * rho_kg_m3

    dose_Gy = data_J / voxel_mass_kg  # J/kg

    np.save(f"{prefix}_dose_Gy.npy", dose_Gy.astype(np.float32))
    meta = {
        "material_formula": mat["formula"],
        "material_density_g_cm3": rho_g_cm3,
    }
    write_vti(f"{prefix}_dose_Gy.vti", dose_Gy.astype(np.float32), dims, origin, spacing, name="dose_Gy", metadata=meta)

    out_dir = os.path.dirname(prefix) or "."
    print()
    print(" --- Dose --- ")
    print()
    print(f"rho                  : {rho_g_cm3:.3e} g/cm3")
    print(f"Voxel grid size      : {dims} mm")
    print()
    print(f"Output directory     : {out_dir}")
    print(f"Output               : {prefix}_dose_Gy.npy, {prefix}_dose_Gy.vti")
    print()


if __name__ == "__main__":
    main()

#!/usr/bin/env python
"""
Compute simple radiolysis yields from Geant4 energy deposition (dose.vti).

Usage:
  python radiolisys.py dose.vti [setups/setup.json] [output_prefix]
    dose.vti           VTI from the Geant4 run (cell data in keV per voxel)
    setups/setup.json  Optional; defaults to setups/setup.json
    output_prefix      Optional; defaults to output/radiolysis (dir must exist)

Assumptions:
- dose.vti stores energy deposition per voxel in keV.
- Beam exposure time comes from setups/setup.json.
- material.radiolysis.g_values_molecules_per_100eV provides species-specific yields.

Outputs (under output_prefix):
- <prefix>_<species>_M.npy         Concentration (mol/L) per voxel
- <prefix>_<species>_M.vti         Same field as VTI (cell data)
- <prefix>_<species>_rate_Mps.npy  Production rate (mol/L/s) per voxel
"""

import json
import os
import sys
from typing import Dict, Tuple
import numpy as np
import xml.etree.ElementTree as ET

AVOGADRO = 6.02214076e23

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


def write_vti_scalar(path: str, data: np.ndarray, dims: Tuple[int, int, int], origin, spacing, name: str, metadata=None):
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


def compute_radiolysis(dose_keV: np.ndarray, exposure_s: float, spacing_mm, g_values: Dict[str, float]):
    """
    dose_keV: energy deposition per voxel from Geant4 (keV)
    exposure_s: beam exposure time (s)
    spacing_mm: (dx, dy, dz) voxel spacing in mm
    g_values: mapping species -> G-value (molecules / 100 eV)
    """
    energy_eV = dose_keV * 1e3
    factor = energy_eV / 100.0  # number of 100 eV chunks

    dx, dy, dz = spacing_mm
    voxel_volume_m3 = (dx * 1e-3) * (dy * 1e-3) * (dz * 1e-3)
    voxel_volume_L = voxel_volume_m3 * 1e3

    results = {}
    for species, g in g_values.items():
        molecules = factor * g
        mol = molecules / AVOGADRO
        conc_M = mol / voxel_volume_L  # mol/L
        if exposure_s > 0:
            rate_Mps = conc_M / exposure_s
        else:
            rate_Mps = np.zeros_like(conc_M)
        results[species] = (conc_M.astype(np.float32), rate_Mps.astype(np.float32))
    return results


def main():
    if len(sys.argv) < 2:
        print("Usage: python radiolisys.py dose.vti [setups/setup.json] [output_prefix]")
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
    prefix = sys.argv[3] if len(sys.argv) > 3 else "output/radiolysis"

    dose_keV, dims, origin, spacing = parse_vti(vti_path)

    cfg = json.load(open(setup_path))
    beam = cfg["beam"]
    obj = cfg["objects"][0]
    mat = obj["material"]

    exposure = float(beam.get("exposure_time_s", 1.0))

    radiolysis_cfg = mat.get("radiolysis")
    if not radiolysis_cfg:
        raise ValueError("Missing material.radiolysis in setup file (need G-values).")
    g_values = radiolysis_cfg.get("g_values_molecules_per_100eV")
    if not isinstance(g_values, dict) or not g_values:
        raise ValueError("material.radiolysis.g_values_molecules_per_100eV must be a non-empty object.")
    g_values = {k: float(v) for k, v in g_values.items()}
    g_meta = ";".join(f"{k}:{v}" for k, v in g_values.items())

    results = compute_radiolysis(dose_keV, exposure, spacing, g_values)

    out_dir = os.path.dirname(prefix) or "."
    if out_dir and out_dir != ".":
        os.makedirs(out_dir, exist_ok=True)

    for species, (conc, rate) in results.items():
        base = f"{prefix}_{species}"
        np.save(f"{base}_M.npy", conc)
        np.save(f"{base}_rate_Mps.npy", rate)
        meta = {
            "material_formula": obj["material"]["formula"],
            "beam_mono_energy_keV": beam["mono_energy_keV"],
            "beam_exposure_time_s": exposure,
            "g_values_molecules_per_100eV": g_meta,
            "species": species,
        }
        if "source" in radiolysis_cfg:
            meta["radiolysis_source"] = radiolysis_cfg["source"]
        write_vti_scalar(f"{base}_M.vti", conc, dims, origin, spacing, name=f"{species}_M", metadata=meta)

    print()
    print(f" --- Radiolysis --- ")
    print()
    print(f"Computed radiolysis for {len(results)} species using G-values (molecules/100 eV): {g_meta}")
    print()
    print(f"Exposure time        : {exposure:.3e} s")
    print(f"Voxel grid size      : {dims} mm")
    print()
    print(f"Output directory     : {out_dir}")
    print(f"Output               : {prefix}_<species>_M.npy/.vti and _rate_Mps.npy")
    print()


if __name__ == "__main__":
    main()

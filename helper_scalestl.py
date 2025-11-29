#!/usr/bin/env python
"""
Scale and recenter STLs to match the Geant4 voxel box, writing ASCII *_ascii_scaled.stl.

Reads setup.json for voxel_grid.half_size_mm and mesh units, fits meshes to 90%
of the cube, recenters to the origin, and writes <mesh_name>_ascii_scaled.stl
alongside the source mesh.

Usage:
  python helper_scalestl.py                # process data/*.stl (excluding *_ascii_scaled)
  python helper_scalestl.py path1.stl ...  # explicit files
"""

import argparse
import glob
import json
import os
import struct
from typing import List, Tuple


def read_stl(path: str) -> List[Tuple[Tuple[float, float, float], Tuple[float, ...]]]:
    """
    Minimal STL reader supporting binary and ASCII.
    Returns list of (normal, verts) where normal is 3 floats, verts is 9 floats.
    """
    with open(path, "rb") as f:
        data = f.read()

    if len(data) < 84:
        raise ValueError("STL file too small")

    # Heuristic: check header for 'solid' and absence of binary layout
    header = data[:80]
    n_triangles = struct.unpack("<I", data[80:84])[0]
    expected_size = 84 + n_triangles * 50

    def parse_binary(buf: bytes, n_tris: int) -> List[Tuple[Tuple[float, float, float], Tuple[float, ...]]]:
        tris_local = []
        offset = 84
        for _ in range(n_tris):
            if offset + 50 > len(buf):
                raise ValueError("Binary STL truncated")
            normal = struct.unpack("<3f", buf[offset : offset + 12])
            verts = struct.unpack("<9f", buf[offset + 12 : offset + 48])
            tris_local.append((normal, verts))
            offset += 50
        return tris_local

    # Binary path: size matches layout and header not starting with ASCII "solid"
    header_ascii = header[:5].lower()
    if len(data) == expected_size and header_ascii != b"solid":
        return parse_binary(data, n_triangles)

    # ASCII fallback
    try:
        text = data.decode("utf-8", errors="ignore")
    except Exception as e:
        raise ValueError(f"Failed to decode ASCII STL: {e}")

    tris: List[Tuple[Tuple[float, float, float], Tuple[float, ...]]] = []
    tokens = text.replace("\r", "").split()
    i = 0
    while i < len(tokens):
        if tokens[i].lower() == "facet" and i + 13 < len(tokens):
            # facet normal nx ny nz outer loop vertex ... vertex ... vertex ...
            nx = float(tokens[i + 2]); ny = float(tokens[i + 3]); nz = float(tokens[i + 4])
            verts = []
            # find the next three "vertex"
            vcount = 0
            j = i + 5
            while j < len(tokens) and vcount < 3:
                if tokens[j].lower() == "vertex" and j + 3 < len(tokens):
                    verts.extend([float(tokens[j + 1]), float(tokens[j + 2]), float(tokens[j + 3])])
                    vcount += 1
                    j += 4
                else:
                    j += 1
            if vcount == 3:
                tris.append(((nx, ny, nz), tuple(verts)))
                i = j
            else:
                i += 1
        else:
            i += 1

    if not tris:
        raise ValueError("Could not parse STL (binary or ASCII)")
    return tris


def write_ascii_stl(path: str, tris: List[Tuple[Tuple[float, float, float], Tuple[float, ...]]]):
    with open(path, "w") as o:
        o.write("solid scaled\n")
        for normal, verts in tris:
            o.write(f"  facet normal {normal[0]} {normal[1]} {normal[2]}\n")
            o.write("    outer loop\n")
            for i in range(3):
                x, y, z = verts[3 * i : 3 * i + 3]
                o.write(f"      vertex {x} {y} {z}\n")
            o.write("    endloop\n")
            o.write("  endfacet\n")
        o.write("endsolid scaled\n")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("paths", nargs="*", help="STL files to scale (binary or ASCII)")
    args = parser.parse_args()

    cfg = json.load(open("setup.json"))
    voxel_half_size = float(cfg.get("voxel_grid", {}).get("half_size_mm", 10.0))
    target = 2 * voxel_half_size * 0.9  # 90% margin

    # Units from setup.json for the configured mesh, else default mm
    setup_units = "mm"
    if cfg.get("objects"):
        setup_units = cfg["objects"][0].get("units", "mm")

    # Build target list
    targets: List[str] = []
    if args.paths:
        targets = args.paths
    else:
        targets = glob.glob(os.path.join("data", "*.stl"))
        targets = [p for p in targets if not p.endswith("_ascii_scaled.stl")]

    for path in targets:
        if not os.path.isfile(path):
            print(f"[skip] {path} (not found)")
            continue

        units = setup_units  # single-object config; override per-file if needed
        unit_scale = {"mm": 1.0, "cm": 10.0, "m": 1000.0}.get(units, 1.0)

        try:
            tris = read_stl(path)
        except Exception as e:
            print(f"[fail] {path}: {e}")
            continue

        xmin = ymin = zmin = float("inf")
        xmax = ymax = zmax = float("-inf")
        for _, v in tris:
            for i in range(3):
                x, y, z = v[3 * i : 3 * i + 3]
                xmin = min(xmin, x)
                ymin = min(ymin, y)
                zmin = min(zmin, z)
                xmax = max(xmax, x)
                ymax = max(ymax, y)
                zmax = max(zmax, z)
        extent = (xmax - xmin, ymax - ymin, zmax - zmin)
        max_dim = max(extent)

        fit_scale = target / (max_dim * unit_scale) if max_dim > 0 else 1.0
        scale = unit_scale * fit_scale

        cx = 0.5 * (xmin + xmax)
        cy = 0.5 * (ymin + ymax)
        cz = 0.5 * (zmin + zmax)

        scaled_tris = []
        for norm, verts in tris:
            out_verts = []
            for i in range(3):
                x, y, z = verts[3 * i : 3 * i + 3]
                out_verts.extend([(x - cx) * scale, (y - cy) * scale, (z - cz) * scale])
            scaled_tris.append((norm, tuple(out_verts)))

        base, _ = os.path.splitext(os.path.basename(path))
        out_dir = os.path.dirname(path) or "."
        out_path = os.path.join(out_dir, f"{base}_ascii_scaled.stl")
        write_ascii_stl(out_path, scaled_tris)
        print(f"[ok] {path} -> {out_path} "
              f"(scale={scale:.4g}, unit={units}, half_size_mm={voxel_half_size})")


if __name__ == "__main__":
    main()

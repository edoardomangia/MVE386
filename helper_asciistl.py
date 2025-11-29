#!/usr/bin/env python
"""
Convert STL files to ASCII so CADMesh/Geant4 can load them easily.

Usage:
  python helper_asciistl.py [files...]
If no files are passed, all *.stl under data/ are processed.
Binary STLs are rewritten to <name>_ascii.stl; ASCII STLs are left alone.
"""

import glob
import os
import struct
import sys
from typing import List


def is_ascii_stl(path: str) -> bool:
    try:
        with open(path, "rb") as f:
            prefix = f.read(5)
        return prefix.lower() == b"solid"
    except OSError:
        return False


def convert_binary_to_ascii(src: str, dst: str) -> None:
    with open(src, "rb") as f:
        f.read(80)  # header
        n_triangles_bytes = f.read(4)
        if len(n_triangles_bytes) != 4:
            raise ValueError(f"{src}: cannot read triangle count")
        n_triangles = struct.unpack("<I", n_triangles_bytes)[0]

        tris = []
        for i in range(n_triangles):
            normal_bytes = f.read(12)
            verts_bytes = f.read(36)
            attr_bytes = f.read(2)
            if len(normal_bytes) != 12 or len(verts_bytes) != 36 or len(attr_bytes) != 2:
                raise ValueError(f"{src}: truncated at triangle {i}/{n_triangles}")
            normal = struct.unpack("<3f", normal_bytes)
            verts = struct.unpack("<9f", verts_bytes)
            tris.append((normal, verts))

    with open(dst, "w") as out:
        out.write("solid converted\n")
        for normal, verts in tris:
            out.write(f"  facet normal {normal[0]} {normal[1]} {normal[2]}\n")
            out.write("    outer loop\n")
            out.write(f"      vertex {verts[0]} {verts[1]} {verts[2]}\n")
            out.write(f"      vertex {verts[3]} {verts[4]} {verts[5]}\n")
            out.write(f"      vertex {verts[6]} {verts[7]} {verts[8]}\n")
            out.write("    endloop\n")
            out.write("  endfacet\n")
        out.write("endsolid converted\n")


def main(paths: List[str]) -> None:
    for src in paths:
        if not os.path.isfile(src):
            print(f"[skip] {src} (not found)")
            continue

        base, _ = os.path.splitext(src)
        dst = f"{base}_ascii.stl"

        if is_ascii_stl(src):
            if os.path.isfile(dst):
                print(f"[ok]   {src} already ASCII (existing {dst})")
            else:
                print(f"[ok]   {src} already ASCII")
            continue

        try:
            convert_binary_to_ascii(src, dst)
            print(f"[done] {src} -> {dst}")
        except Exception as e:
            print(f"[fail] {src}: {e}")


if __name__ == "__main__":
    args = sys.argv[1:]
    if not args:
        args = glob.glob(os.path.join("data", "*.stl"))
    main(args)

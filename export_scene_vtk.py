#!/usr/bin/env python

"""
Export a minimal ParaView-friendly scene from setups/*.json.

Generates an ASCII VTK PolyData with:
- Source (small cube)
- Beam ray (line)
- Detector plane (quad)
- Voxel box (cube)

Color in ParaView by the cell array "part_id":
  1 = source, 2 = beam, 3 = detector, 4 = voxel_box
"""

import argparse
import json
import math
import os
from typing import List, Tuple


Vec3 = Tuple[float, float, float]


def v_add(a: Vec3, b: Vec3) -> Vec3:
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])


def v_sub(a: Vec3, b: Vec3) -> Vec3:
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def v_mul(a: Vec3, s: float) -> Vec3:
    return (a[0] * s, a[1] * s, a[2] * s)


def v_dot(a: Vec3, b: Vec3) -> float:
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def v_cross(a: Vec3, b: Vec3) -> Vec3:
    return (
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    )


def v_norm(a: Vec3) -> float:
    return math.sqrt(v_dot(a, a))


def v_unit(a: Vec3, fallback: Vec3 = (1.0, 0.0, 0.0)) -> Vec3:
    n = v_norm(a)
    if n < 1e-12:
        return fallback
    return (a[0] / n, a[1] / n, a[2] / n)


def orthonormal_basis(direction: Vec3, up_hint: Vec3) -> Tuple[Vec3, Vec3, Vec3]:
    n = v_unit(direction, fallback=(1.0, 0.0, 0.0))
    up_proj = v_sub(up_hint, v_mul(n, v_dot(up_hint, n)))
    if v_norm(up_proj) < 1e-12:
        up_proj = (0.0, 1.0, 0.0) if abs(n[1]) < 0.9 else (0.0, 0.0, 1.0)
        up_proj = v_sub(up_proj, v_mul(n, v_dot(up_proj, n)))
    up = v_unit(up_proj, fallback=(0.0, 1.0, 0.0))
    right = v_unit(v_cross(n, up), fallback=(0.0, 0.0, 1.0))
    up = v_unit(v_cross(right, n), fallback=up)
    return right, up, n


def add_cube(points: List[Vec3], polys: List[List[int]], center: Vec3, half: float) -> List[List[int]]:
    cx, cy, cz = center
    hx = hy = hz = half
    corners = [
        (cx - hx, cy - hy, cz - hz),
        (cx + hx, cy - hy, cz - hz),
        (cx + hx, cy + hy, cz - hz),
        (cx - hx, cy + hy, cz - hz),
        (cx - hx, cy - hy, cz + hz),
        (cx + hx, cy - hy, cz + hz),
        (cx + hx, cy + hy, cz + hz),
        (cx - hx, cy + hy, cz + hz),
    ]
    base = len(points)
    points.extend(corners)
    faces = [
        [base + 0, base + 1, base + 2, base + 3],  # -Z
        [base + 4, base + 5, base + 6, base + 7],  # +Z
        [base + 0, base + 1, base + 5, base + 4],  # -Y
        [base + 2, base + 3, base + 7, base + 6],  # +Y
        [base + 1, base + 2, base + 6, base + 5],  # +X
        [base + 0, base + 3, base + 7, base + 4],  # -X
    ]
    polys.extend(faces)
    return faces


def add_oriented_box(points: List[Vec3],
                     polys: List[List[int]],
                     center: Vec3,
                     right: Vec3,
                     up: Vec3,
                     forward: Vec3,
                     half_w: float,
                     half_h: float,
                     half_l: float) -> List[List[int]]:
    axes = (
        v_mul(right, half_w),
        v_mul(up, half_h),
        v_mul(forward, half_l),
    )
    corners = []
    for sx in (-1.0, 1.0):
        for sy in (-1.0, 1.0):
            for sz in (-1.0, 1.0):
                offset = v_add(v_add(v_mul(axes[0], sx), v_mul(axes[1], sy)), v_mul(axes[2], sz))
                corners.append(v_add(center, offset))
    base = len(points)
    points.extend(corners)
    faces = [
        [base + 0, base + 1, base + 3, base + 2],
        [base + 4, base + 5, base + 7, base + 6],
        [base + 0, base + 1, base + 5, base + 4],
        [base + 2, base + 3, base + 7, base + 6],
        [base + 1, base + 3, base + 7, base + 5],
        [base + 0, base + 2, base + 6, base + 4],
    ]
    polys.extend(faces)
    return faces


def write_vtk_polydata(path: str,
                       points: List[Vec3],
                       vertices: List[List[int]],
                       lines: List[List[int]],
                       polys: List[List[int]],
                       part_ids_vertices: List[int],
                       part_ids_lines: List[int],
                       part_ids_polys: List[int]):
    with open(path, "w", encoding="ascii") as f:
        f.write("# vtk DataFile Version 3.0\n")
        f.write("scene\n")
        f.write("ASCII\n")
        f.write("DATASET POLYDATA\n")
        f.write(f"POINTS {len(points)} float\n")
        for p in points:
            f.write(f"{p[0]:.6f} {p[1]:.6f} {p[2]:.6f}\n")

        if vertices:
            total = sum(1 + len(v) for v in vertices)
            f.write(f"VERTICES {len(vertices)} {total}\n")
            for v in vertices:
                f.write(f"{len(v)} " + " ".join(str(i) for i in v) + "\n")

        if lines:
            total = sum(1 + len(l) for l in lines)
            f.write(f"LINES {len(lines)} {total}\n")
            for l in lines:
                f.write(f"{len(l)} " + " ".join(str(i) for i in l) + "\n")

        if polys:
            total = sum(1 + len(p) for p in polys)
            f.write(f"POLYGONS {len(polys)} {total}\n")
            for p in polys:
                f.write(f"{len(p)} " + " ".join(str(i) for i in p) + "\n")

        cell_part_ids = part_ids_vertices + part_ids_lines + part_ids_polys
        if cell_part_ids:
            f.write(f"CELL_DATA {len(cell_part_ids)}\n")
            f.write("SCALARS part_id int 1\n")
            f.write("LOOKUP_TABLE default\n")
            for pid in cell_part_ids:
                f.write(f"{pid}\n")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("setup", nargs="?", default=os.path.join("setups", "setup.json"))
    parser.add_argument("-o", "--output", default=None)
    args = parser.parse_args()

    with open(args.setup, "r", encoding="utf-8") as f:
        cfg = json.load(f)

    beam = cfg["beam"]
    src = tuple(float(x) for x in beam["source_position_mm"])
    det = tuple(float(x) for x in beam["detector_position_mm"])
    up_hint = tuple(float(x) for x in beam.get("detector_up", [0.0, 1.0, 0.0]))
    det_pixels = beam["detector_pixels"]
    det_pix_size = beam["detector_pixel_size_mm"]

    voxel = cfg.get("voxel_grid", {})
    half_size = float(voxel.get("half_size_mm", 10.0))

    out_path = args.output
    if out_path is None:
        base = os.path.splitext(os.path.basename(args.setup))[0]
        out_path = os.path.join("output", f"{base}_scene.vtk")

    os.makedirs(os.path.dirname(out_path) or ".", exist_ok=True)

    points: List[Vec3] = []
    vertices: List[List[int]] = []
    lines: List[List[int]] = []
    polys: List[List[int]] = []
    part_ids_vertices: List[int] = []
    part_ids_lines: List[int] = []
    part_ids_polys: List[int] = []

    PART_SOURCE = 1
    PART_BEAM = 2
    PART_DETECTOR = 3
    PART_VOXEL_BOX = 4

    # Source cube
    box_size = min(5.0, max(0.2, 0.05 * 2.0 * half_size))
    source_faces = add_cube(points, polys, src, box_size * 0.5)
    part_ids_polys.extend([PART_SOURCE] * len(source_faces))

    # Detector plane
    direction = v_sub(det, src)
    right, up, forward = orthonormal_basis(direction, up_hint)
    half_w = 0.5 * float(det_pixels[0]) * float(det_pix_size[0])
    half_h = 0.5 * float(det_pixels[1]) * float(det_pix_size[1])
    c = det
    det_pts = [
        v_add(v_add(c, v_mul(right, -half_w)), v_mul(up, -half_h)),
        v_add(v_add(c, v_mul(right,  half_w)), v_mul(up, -half_h)),
        v_add(v_add(c, v_mul(right,  half_w)), v_mul(up,  half_h)),
        v_add(v_add(c, v_mul(right, -half_w)), v_mul(up,  half_h)),
    ]
    det_base = len(points)
    points.extend(det_pts)
    polys.append([det_base + 0, det_base + 1, det_base + 2, det_base + 3])
    part_ids_polys.append(PART_DETECTOR)

    # Beam volume (box spanning source to detector, with detector footprint)
    beam_center = v_mul(v_add(src, det), 0.5)
    beam_half_l = 0.5 * v_norm(v_sub(det, src))
    beam_faces = add_oriented_box(points, polys, beam_center, right, up, forward,
                                  half_w, half_h, beam_half_l)
    part_ids_polys.extend([PART_BEAM] * len(beam_faces))

    # Voxel box
    voxel_faces = add_cube(points, polys, (0.0, 0.0, 0.0), half_size)
    part_ids_polys.extend([PART_VOXEL_BOX] * len(voxel_faces))

    write_vtk_polydata(out_path, points, vertices, lines, polys,
                       part_ids_vertices, part_ids_lines, part_ids_polys)

    print(f"[ok] Wrote {out_path}")
    print("part_id legend: 1=source, 2=beam, 3=detector, 4=voxel_box")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

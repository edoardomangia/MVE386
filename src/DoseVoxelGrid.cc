/*
 * DoseVoxelGrid.cc
 *
 */

#include "DoseVoxelGrid.hh"

DoseVoxelGrid& DoseVoxelGrid::Instance() {
    static DoseVoxelGrid instance;
    return instance;
}

void DoseVoxelGrid::Initialize(int NX_, int NY_, int NZ_,
                               float xmin_, float ymin_, float zmin_,
                               float dx_, float dy_, float dz_) 
{
    G4AutoLock lock(&mutex);
    NX = NX_; NY = NY_; NZ = NZ_;
    xmin = xmin_; ymin = ymin_; zmin = zmin_;
    dx = dx_; dy = dy_; dz = dz_;

    grid.assign(NX * NY * NZ, 0.0f);
}

void DoseVoxelGrid::AddEnergy(float x_mm, float y_mm, float z_mm, float edep_keV)
{
    G4AutoLock lock(&mutex);
    int ix = int((x_mm - xmin) / dx);
    int iy = int((y_mm - ymin) / dy);
    int iz = int((z_mm - zmin) / dz);

    if (ix < 0 || ix >= NX ||
        iy < 0 || iy >= NY ||
        iz < 0 || iz >= NZ)
        return;

    int idx = ix + NX * (iy + NY * iz);
    grid[idx] += edep_keV;
}

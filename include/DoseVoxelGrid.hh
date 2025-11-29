/* 
 * include/DoseVoxelGrid.hh
 */

#pragma once

#include <vector>

class DoseVoxelGrid {
public:
    static DoseVoxelGrid& Instance();

    void Initialize(int NX, int NY, int NZ,
                    float xmin, float ymin, float zmin,
                    float dx, float dy, float dz);

    void AddEnergy(float x_mm, float y_mm, float z_mm, float edep_keV);
    const std::vector<float>& Data() const { return grid; }

    int NX, NY, NZ;
    float xmin, ymin, zmin;
    float dx, dy, dz;

private:
    DoseVoxelGrid() = default;
    std::vector<float> grid;
};

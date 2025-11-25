/* src/RunAction.cc
 * Summary of what we are running and logs
 */

#include "RunAction.hh"
#include "G4Run.hh"
#include "G4SystemOfUnits.hh"
#include "DoseVoxelGrid.hh"
#include "GenVTI.hh"

void RunAction::BeginOfRunAction(const G4Run* run)
{
    // Phantom from -10 mm to +10 mm
    int NX = 100, NY = 100, NZ = 100;
    float xmin = -10, ymin = -10, zmin = -10;
    float dx = 20.0f / NX;   // mm per voxel
    float dy = 20.0f / NY;
    float dz = 20.0f / NZ;

    DoseVoxelGrid::Instance().Initialize(NX, NY, NZ, xmin, ymin, zmin, dx, dy, dz);

    G4cout << "Voxel grid initialised: "
           << NX << "×" << NY << "×" << NZ << " voxels\n";
}

void RunAction::EndOfRunAction(const G4Run* run)
{
    auto& grid = DoseVoxelGrid::Instance();

    VTIWriter::Write("dose.vti",
                     grid.Data(),
                     grid.NX, grid.NY, grid.NZ,
                     grid.xmin, grid.ymin, grid.zmin,
                     grid.dx, grid.dy, grid.dz);

    G4cout << "Wrote dose.vti for ParaView." << G4endl;}


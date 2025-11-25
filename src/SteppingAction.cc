/* 
 * src/SteppingAction.cc
 * Data extraction, logs energy deposition (...in water box now)
 */
#include "SteppingAction.hh"

#include "G4Step.hh"
#include "G4Track.hh"
#include "G4VPhysicalVolume.hh"
#include "G4SystemOfUnits.hh"
#include "DoseVoxelGrid.hh"
#include "GenVTI.hh"
#include <fstream>

// Helper, a static ofstream, so we open the file only once
static std::ofstream& GetStepLogFile()
{
    static std::ofstream file("steps.csv");
    static bool headerWritten = false;

    if (file.is_open() && !headerWritten) {
        file << "x_mm,y_mm,z_mm,edep_keV\n";
        headerWritten = true;
    }
    return file;
}

void SteppingAction::UserSteppingAction(const G4Step* step)
{
    // Total energy deposit in this step
    auto edep = step->GetTotalEnergyDeposit();
    if (edep <= 0.) return;     // nothing deposited

    // Only care about energy deposition in the liquid volume
    auto pre = step->GetPreStepPoint();
    auto vol = pre->GetPhysicalVolume();
    if (!vol) return;

    if (vol->GetName() != "DragonPV") return;

    auto pos = pre->GetPosition();
    
    // Append data
    DoseVoxelGrid::Instance().AddEnergy(
        pos.x()/mm, pos.y()/mm, pos.z()/mm,
        edep/keV
    );
}

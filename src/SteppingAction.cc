/*
 * src/SteppingAction.cc
 * Data extraction, logs energy deposition (...in water box now)
 */

#include "SteppingAction.hh"
#include "DoseVoxelGrid.hh"

#include "G4Step.hh"
#include "G4SystemOfUnits.hh"
#include "G4Threading.hh"
#include "G4Track.hh"
#include "G4VPhysicalVolume.hh"

#include <filesystem>
#include <fstream>

SteppingAction::SteppingAction(const std::string &output_dir) {
  // std::filesystem::create_directories(output_dir);
  // auto tid = G4Threading::G4GetThreadId();
  // auto filename = std::string("steps");
  // if (tid >= 0) {
  //   filename += "_t" + std::to_string(tid);
  // }
  // filename += ".csv";
  // auto path = std::filesystem::path(output_dir) / filename;
  // file_.open(path);
  // if (file_.is_open() && !headerWritten_) {
  //   file_ << "x_mm,y_mm,z_mm,edep_keV\n";
  //   headerWritten_ = true;
  // }
}

void SteppingAction::UserSteppingAction(const G4Step *step) {
  // Total energy deposit in this step
  auto edep = step->GetTotalEnergyDeposit();
  if (edep <= 0.)
    return; // nothing deposited

  // Only care about energy deposition in the liquid volume
  auto pre = step->GetPreStepPoint();
  auto vol = pre->GetPhysicalVolume();
  if (!vol)
    return;

  if (vol->GetName() != "ModelPV")
    return;

  auto pos = pre->GetPosition();

  if (file_.is_open()) {
    file_ << pos.x() / mm << "," << pos.y() / mm << "," << pos.z() / mm << ","
          << edep / keV << "\n";
  }

  // Append data
  DoseVoxelGrid::Instance().AddEnergy(pos.x() / mm, pos.y() / mm, pos.z() / mm,
                                      edep / keV);
}

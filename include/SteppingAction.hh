/*
 * include/SteppingAction.hh
 */
#pragma once

#include "G4UserSteppingAction.hh"

#include <fstream>
#include <string>

class SteppingAction : public G4UserSteppingAction {
public:
    explicit SteppingAction(const std::string& output_dir);
    ~SteppingAction() override = default;

    void UserSteppingAction(const G4Step* step) override;

private:
    std::ofstream file_;
    bool headerWritten_ = false;
};

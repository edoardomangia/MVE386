/*
 * include/DetectorConstruction.hh
 */
#pragma once

#include "SceneConfig.hh"

#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"

class DetectorConstruction : public G4VUserDetectorConstruction {
public:
    explicit DetectorConstruction(const SceneConfig& cfg)
        : config(cfg) {}
    ~DetectorConstruction() override = default;

    G4VPhysicalVolume* Construct() override;

private:
    SceneConfig config;
};

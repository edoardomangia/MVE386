/*
 * include/PrimaryGeneratorAction.hh
 */

#pragma once

#include "SceneConfig.hh"

#include "G4VUserPrimaryGeneratorAction.hh"

class G4ParticleGun;

class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction {
public:
    explicit PrimaryGeneratorAction(const SceneConfig& cfg);
    ~PrimaryGeneratorAction() override;

    void GeneratePrimaries(G4Event* event) override;

private:
    SceneConfig config;
    G4ParticleGun* fParticleGun;
};

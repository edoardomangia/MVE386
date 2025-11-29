/*
 * include/RunAction.hh
 */

#pragma once

#include "SceneConfig.hh"

#include "G4UserRunAction.hh"

class RunAction : public G4UserRunAction {
public:
    explicit RunAction(const SceneConfig& cfg);
    ~RunAction() override = default;

    void BeginOfRunAction(const G4Run*) override;
    void EndOfRunAction(const G4Run*) override;

private:
    SceneConfig config;
};

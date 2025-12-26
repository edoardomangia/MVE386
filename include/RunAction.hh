/*
 * include/RunAction.hh
 */

#pragma once

#include "SceneConfig.hh"

#include "G4UserRunAction.hh"
#include <atomic>

class RunAction : public G4UserRunAction {
public:
    explicit RunAction(const SceneConfig& cfg);
    ~RunAction() override = default;

    void BeginOfRunAction(const G4Run*) override;
    void EndOfRunAction(const G4Run*) override;

    static void SetIsFinalChunk(bool v);
    static bool IsFinalChunk();

private:
    SceneConfig config;
};

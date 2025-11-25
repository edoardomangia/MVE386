/* 
 * src/ActionInitialization.hh
 */
#pragma once

#include "G4VUserActionInitialization.hh"
#include "SceneConfig.hh"

class ActionInitialization : public G4VUserActionInitialization {
public:
    explicit ActionInitialization(const SceneConfig& cfg)
        : config(cfg) {}
    ~ActionInitialization() override = default;

    void Build() const override;

private:
    SceneConfig config;
};


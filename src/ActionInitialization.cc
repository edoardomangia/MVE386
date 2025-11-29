/* 
 * src/ActionInitialization.cc
 * Wires primary generation, run and stepping actions
 */

#include "ActionInitialization.hh"
#include "PrimaryGeneratorAction.hh"
#include "RunAction.hh"
#include "SteppingAction.hh"

void ActionInitialization::Build() const
{
    SetUserAction(new PrimaryGeneratorAction(config));
    
    SetUserAction(new RunAction(config));
    
    SetUserAction(new SteppingAction(config.output_dir));
}

#include "ActionInitialization.hh"
#include "AnalysisConfig.hh"
#include "PrimaryGeneratorAction.hh"
#include "RunAction.hh"
#include "EventAction.hh"
#include "SteppingAction.hh"

ActionInitialization::ActionInitialization()
    : G4VUserActionInitialization()
{
}

ActionInitialization::~ActionInitialization()
{
}

void ActionInitialization::BuildForMaster() const
{
  RunAction *runAction = new RunAction(&fAnalysisConfig);
  SetUserAction(runAction);
}

void ActionInitialization::Build() const
{
  RunAction *runAction = new RunAction(&fAnalysisConfig);
  SetUserAction(runAction);

  EventAction *eventAction = new EventAction(runAction, &fAnalysisConfig);
  SetUserAction(eventAction);

  PrimaryGeneratorAction *primaryGeneratorAction =
      new PrimaryGeneratorAction(eventAction, &fAnalysisConfig);
  SetUserAction(primaryGeneratorAction);

  SteppingAction *steppingAction =
      new SteppingAction(eventAction, &fAnalysisConfig);
  SetUserAction(steppingAction);
}
#include "RunAction.hh"

#include "G4Run.hh"
#include "G4ios.hh"

RunAction::RunAction(const AnalysisConfig *config)
    : G4UserRunAction(),
      fAnalysisConfig(config)
{
}

RunAction::~RunAction()
{
}

void RunAction::BeginOfRunAction(const G4Run *aRun)
{
    G4cout << "### Run " << aRun->GetRunID() << " Start." << G4endl;

    if (fAnalysisConfig)
    {
        G4cout << "  enableReactionPosition = " << fAnalysisConfig->enableReactionPosition << G4endl;
        G4cout << "  enableEdep             = " << fAnalysisConfig->enableEdep << G4endl;
        G4cout << "  enableTrackLen         = " << fAnalysisConfig->enableTrackLen << G4endl;
        G4cout << "  enableAttenuation      = " << fAnalysisConfig->enableAttenuation << G4endl;
    }
}

void RunAction::EndOfRunAction(const G4Run *aRun)
{
    G4cout << "### Run " << aRun->GetRunID() << " Stop." << G4endl;
}

const AnalysisConfig *RunAction::GetAnalysisConfig() const
{
    return fAnalysisConfig;
}
#ifndef RunAction_h
#define RunAction_h 1

#include "G4UserRunAction.hh"
#include "globals.hh"
#include "G4UnitsTable.hh"
#include "AnalysisConfig.hh"

class G4Run;

class RunAction : public G4UserRunAction
{
  public:
    RunAction(const AnalysisConfig* config);
    virtual ~RunAction();

    virtual void BeginOfRunAction(const G4Run*);
    virtual void EndOfRunAction(const G4Run*);

    const AnalysisConfig* GetAnalysisConfig() const;

  private:
    const AnalysisConfig* fAnalysisConfig;
};

#endif
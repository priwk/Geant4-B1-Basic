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
  RunAction(const AnalysisConfig *config);
  virtual ~RunAction();

  virtual void BeginOfRunAction(const G4Run *);
  virtual void EndOfRunAction(const G4Run *);

  const AnalysisConfig *GetAnalysisConfig() const;

  void CountIncident();
  void CountCapture();
  void CountTransmit();

  void AddNeutronTrackLength(G4double len);
  G4double GetTotalNeutronTrackLength() const;
  G4double GetSigmaEff() const;

  G4int GetNIncident() const { return fNIncident; }
  G4int GetNCapture() const { return fNCapture; }
  G4int GetNTransmit() const { return fNTransmit; }

private:
  const AnalysisConfig *fAnalysisConfig;

  G4int fNIncident;
  G4int fNCapture;
  G4int fNTransmit;

  G4double fTotalNeutronTrackLength;
};

#endif
#ifndef SteppingAction_h
#define SteppingAction_h 1

#include "G4UserSteppingAction.hh"
#include "globals.hh"
#include <fstream>

class EventAction;
class DetectorConstruction;
class AnalysisConfig;
class G4Step;

class SteppingAction : public G4UserSteppingAction
{
public:
  SteppingAction(EventAction *eventAction, const AnalysisConfig *config);
  virtual ~SteppingAction();

  virtual void UserSteppingAction(const G4Step *step);

private:
  EventAction *fEventAction;
  DetectorConstruction *fDetector;
  const AnalysisConfig *fAnalysisConfig;

  // CSV file for step information
  static std::ofstream fStepCSVFile;
  static G4bool fStepCSVInitialized;
};

#endif
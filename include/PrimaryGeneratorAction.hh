#ifndef PrimaryGeneratorAction_h
#define PrimaryGeneratorAction_h 1

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4GeneralParticleSource.hh"
#include "globals.hh"

#include <vector>

class G4Event;
class EventAction;
class AnalysisConfig;

// 步骤源项记录
struct StepSourceRecord
{
  G4int sourceLabel = -1;
  G4int eventID = -1;
  G4int trackID = -1;
  G4int stepID = -1;

  G4String particle;

  G4double bnWt = 0.0;
  G4double znsWt = 0.0;
  G4double thicknessUm = 0.0;

  G4double captureX = 0.0;
  G4double captureY = 0.0;
  G4double captureZ = 0.0;
  G4double captureDepth = 0.0;

  G4double xPre = 0.0;
  G4double yPre = 0.0;
  G4double zPre = 0.0;

  G4double xPost = 0.0;
  G4double yPost = 0.0;
  G4double zPost = 0.0;

  G4double xMid = 0.0;
  G4double yMid = 0.0;
  G4double zMid = 0.0;
  G4double depthMid = 0.0;

  G4double stepLenUm = 0.0;
  G4double edepKeV = 0.0;
  G4double ekinPreKeV = 0.0;
  G4double ekinPostKeV = 0.0;
};

// 生成粒子事件
class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction
{
public:
  PrimaryGeneratorAction(EventAction *eventAction,
                         const AnalysisConfig *config);
  ~PrimaryGeneratorAction() override;

public:
  void GeneratePrimaries(G4Event *) override;

private:
  void LoadReplayCSV();
  void ResetReplayCache();

private:
  G4GeneralParticleSource *fParticleGun;
  EventAction *fEventAction;
  const AnalysisConfig *fAnalysisConfig;

  G4int fLoadedReplayThicknessUm;

  bool fReplayInitialized;
  size_t fReplayEventCursor;
  std::vector<std::vector<StepSourceRecord>> fReplayEvents;

  G4int GetCurrentThicknessLabelUm() const;
};
#endif
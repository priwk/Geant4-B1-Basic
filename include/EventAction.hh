#ifndef EventAction_h
#define EventAction_h 1

#include "G4UserEventAction.hh"
#include "globals.hh"
#include <fstream>

class G4Event;
class RunAction;
class AnalysisConfig;

class EventAction : public G4UserEventAction
{
public:
  EventAction(RunAction *runAction, const AnalysisConfig *config);
  virtual ~EventAction();

  virtual void BeginOfEventAction(const G4Event *event);
  virtual void EndOfEventAction(const G4Event *event);

  // ---- 路径长度累计 ----
  void AddAlphaTrackLen(G4double len);
  void AddLi7TrackLen(G4double len);

  // ---- 沉积能量累计 ----
  void AddEdep(G4double edep);

  void SetSourcePosition(G4double x, G4double y);

  // ---- 俘获信息 ----
  void SetCaptureInfo(G4double bnWt,
                      G4double znsWt,
                      G4double x,
                      G4double y,
                      G4double z,
                      G4double depth);

  G4bool HasCapture() const;

  void SetTransmitted();
  G4bool HasTransmit() const;

  // ---- 可选：给 SteppingAction / RunAction 读取 ----
  G4double GetAlphaTrackLen() const;
  G4double GetLi7TrackLen() const;
  G4double GetEdep() const;

private:
  RunAction *fRunAction;
  const AnalysisConfig *fAnalysisConfig;

  // 事件级累计量
  G4double fAlphaTrackLen;
  G4double fLi7TrackLen;
  G4double fEdep;

  // 源位置
  G4double fSourceX;
  G4double fSourceY;

  // 俘获信息
  G4bool fHasCapture;
  G4bool fHasTransmit;
  G4double fBNWt;
  G4double fZnSWt;
  G4double fCaptureX;
  G4double fCaptureY;
  G4double fCaptureZ;
  G4double fDepth;

  // 反应位置 CSV
  static std::ofstream fCSVFile;
  static G4bool fCSVInitialized;
};

#endif
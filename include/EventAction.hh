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

  // ---- 源位置 ----
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

  // ---- 给 SteppingAction / RunAction 读取 ----
  G4double GetAlphaTrackLen() const;
  G4double GetLi7TrackLen() const;
  G4double GetEdep() const;
  G4double GetGeneratedPhotons() const;

  // ---- replay 光子信息 ----
  // replay_photons: 本事件生成并发射的光子数
  // detected/escaped photons: 成功从 Film 背面透射到 World 的光子数
  void SetReplayPhotonCount(G4int n);
  void AddDetectedPhoton(G4double x, G4double y);
  G4double GetReplayPhotonCount() const;

private:
  RunAction *fRunAction;
  const AnalysisConfig *fAnalysisConfig;

  // 事件级累计量
  G4double fAlphaTrackLen;
  G4double fLi7TrackLen;
  G4double fEdep;
  G4double fGeneratedPhotons;

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

  // 按厚度分文件的 CSV 句柄
  static std::ofstream fCSVFile; // neutron_capture_positions
  static G4bool fCSVInitialized;

  static std::ofstream fEdepCSVFile; // energy_deposition
  static G4bool fEdepCSVInitialized;

  // replay 光子统计
  G4int fReplayPhotonCount;
  G4int fDetectedPhotonCount;
  G4double fDetectedSumX;
  G4double fDetectedSumY;
  G4double fDetectedSumX2;
  G4double fDetectedSumY2;
};

#endif
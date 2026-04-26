#include "EffectiveSigmaCaptureProcess.hh"

#include "DetectorConstruction.hh"

#include "G4Material.hh"
#include "G4Neutron.hh"
#include "G4ParticleDefinition.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4TrackStatus.hh"
#include "G4SystemOfUnits.hh"
#include "G4VParticleChange.hh"

#include <cfloat>

EffectiveSigmaCaptureProcess::EffectiveSigmaCaptureProcess(DetectorConstruction *detector,
                                                           G4double sigmaEff,
                                                           const G4String &processName)
    : G4VDiscreteProcess(processName, fHadronic),
      fDetector(detector),
      fSigmaEff(sigmaEff)
{
}

G4bool EffectiveSigmaCaptureProcess::IsApplicable(const G4ParticleDefinition &particle)
{
  return (&particle == G4Neutron::Neutron());
}

void EffectiveSigmaCaptureProcess::SetSigmaEff(G4double value)
{
  fSigmaEff = value;
}

G4double EffectiveSigmaCaptureProcess::GetSigmaEff() const
{
  // 关键点：同一个 mac 内多次调用 /det/setSigmaEffPerCm 时，
  // 已经挂到 neutron process manager 上的 process 实例不会重新构造。
  // 因此这里必须优先读取 DetectorConstruction 中的当前值，
  // 而不是只使用构造函数传入的旧 fSigmaEff。
  if (fDetector && fDetector->UseManualSigmaEff())
  {
    return fDetector->GetManualSigmaEff();
  }

  return fSigmaEff;
}

G4bool EffectiveSigmaCaptureProcess::IsTargetMaterial(const G4Material *mat) const
{
  if (!fDetector || !mat)
    return false;

  G4Material *filmMat = fDetector->GetFilmMaterial();
  if (!filmMat)
    return false;

  return (mat == filmMat);
}

G4double EffectiveSigmaCaptureProcess::GetMeanFreePath(const G4Track &track,
                                                       G4double,
                                                       G4ForceCondition *condition)
{
  if (condition)
    *condition = NotForced;

  if (track.GetDefinition() != G4Neutron::Neutron())
    return DBL_MAX;

  // 没有 /det/setSigmaEffPerCm 时，该过程保持惰性，不影响默认 Geant4 模式。
  if (!fDetector || !fDetector->UseManualSigmaEff())
    return DBL_MAX;

  const G4Material *mat = track.GetMaterial();
  if (!IsTargetMaterial(mat))
    return DBL_MAX;

  const G4double sigmaEff = GetSigmaEff();
  if (sigmaEff <= 0.0)
    return DBL_MAX;

  return 1.0 / sigmaEff;
}

G4VParticleChange *EffectiveSigmaCaptureProcess::PostStepDoIt(const G4Track &track,
                                                              const G4Step &)
{
  fParticleChange.Initialize(track);

  // 没有 /det/setSigmaEffPerCm 时，该过程不做任何事。
  if (!fDetector || !fDetector->UseManualSigmaEff())
  {
    return &fParticleChange;
  }

  // 手动 Sigma_eff 模式的定义：
  //   - 该过程只表示一次等效宏观吸收/等效俘获；
  //   - 发生点由 Geant4 离散过程抽样决定，即 post-step point；
  //   - SteppingAction 通过 process name == "EffectiveSigmaCapture" 识别该点，
  //     并写入 neutron_capture_positions；
  //   - 本项目不在这里人工生成 alpha / Li7，不给局域沉积能量。
  fParticleChange.ProposeTrackStatus(fStopAndKill);
  fParticleChange.ProposeEnergy(0.0);
  fParticleChange.ProposeLocalEnergyDeposit(0.0);

  return &fParticleChange;
}

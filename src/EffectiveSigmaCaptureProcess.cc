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

  // 只对 neutron 生效
  if (track.GetDefinition() != G4Neutron::Neutron())
    return DBL_MAX;

  // 只对 Film 的均相材料生效
  const G4Material *mat = track.GetMaterial();
  if (!IsTargetMaterial(mat))
    return DBL_MAX;

  // 非法或未设置的 Sigma_eff：视为不发生该过程
  if (fSigmaEff <= 0.0)
    return DBL_MAX;

  // lambda = 1 / Sigma_eff
  return 1.0 / fSigmaEff;
}

G4VParticleChange *EffectiveSigmaCaptureProcess::PostStepDoIt(const G4Track &track,
                                                              const G4Step &)
{
  fParticleChange.Initialize(track);

  // 这里把发生点视为“等效俘获点”
  // 当前阶段只关心吸收率与俘获坐标，不生成真实 alpha / Li 次级粒子
  fParticleChange.ProposeTrackStatus(fStopAndKill);
  fParticleChange.ProposeEnergy(0.0);
  fParticleChange.ProposeLocalEnergyDeposit(0.0);

  return &fParticleChange;
}
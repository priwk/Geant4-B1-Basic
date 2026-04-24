#ifndef EffectiveSigmaCaptureProcess_h
#define EffectiveSigmaCaptureProcess_h 1

#include "G4VDiscreteProcess.hh"
#include "G4ParticleChange.hh"
#include "globals.hh"

class DetectorConstruction;
class G4Material;
class G4Track;
class G4Step;
class G4ParticleDefinition;

class EffectiveSigmaCaptureProcess : public G4VDiscreteProcess
{
public:
  EffectiveSigmaCaptureProcess(DetectorConstruction *detector,
                               G4double sigmaEff,
                               const G4String &processName = "EffectiveSigmaCapture");
  ~EffectiveSigmaCaptureProcess() override = default;

  G4bool IsApplicable(const G4ParticleDefinition &particle) override;

  G4double GetMeanFreePath(const G4Track &track,
                           G4double previousStepSize,
                           G4ForceCondition *condition) override;

  G4VParticleChange *PostStepDoIt(const G4Track &track,
                                  const G4Step &step) override;

  void SetSigmaEff(G4double value);
  G4double GetSigmaEff() const;

private:
  G4bool IsTargetMaterial(const G4Material *mat) const;

private:
  DetectorConstruction *fDetector = nullptr;
  G4double fSigmaEff = 0.0; // 宏观有效吸收系数，单位 1/length
  G4ParticleChange fParticleChange;
};

#endif
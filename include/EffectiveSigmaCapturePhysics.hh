#ifndef EffectiveSigmaCapturePhysics_h
#define EffectiveSigmaCapturePhysics_h 1

#include "G4VPhysicsConstructor.hh"
#include "globals.hh"

class DetectorConstruction;

class EffectiveSigmaCapturePhysics : public G4VPhysicsConstructor
{
public:
  explicit EffectiveSigmaCapturePhysics(DetectorConstruction *detector);
  ~EffectiveSigmaCapturePhysics() override = default;

  void ConstructParticle() override;
  void ConstructProcess() override;

private:
  DetectorConstruction *fDetector;
};

#endif
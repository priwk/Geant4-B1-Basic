#include "EffectiveSigmaCapturePhysics.hh"

#include "DetectorConstruction.hh"
#include "EffectiveSigmaCaptureProcess.hh"

#include "G4Neutron.hh"
#include "G4ProcessManager.hh"
#include "G4ProcessVector.hh"
#include "G4VProcess.hh"
#include "G4SystemOfUnits.hh"
#include "G4ios.hh"

EffectiveSigmaCapturePhysics::EffectiveSigmaCapturePhysics(DetectorConstruction *detector)
    : G4VPhysicsConstructor("EffectiveSigmaCapturePhysics"),
      fDetector(detector)
{
}

void EffectiveSigmaCapturePhysics::ConstructParticle()
{
  // neutron 已由参考物理表构造，这里无需额外定义
}

void EffectiveSigmaCapturePhysics::ConstructProcess()
{
  auto *neutron = G4Neutron::Neutron();
  auto *pm = neutron->GetProcessManager();
  if (!pm)
    return;

  // --------------------------------------------------
  // 0. 如果没有手动指定 Sigma_eff，则保留 Geant4 默认 nCapture
  // --------------------------------------------------
  if (!fDetector || !fDetector->UseManualSigmaEff())
  {
    G4cout << "[EffectiveSigmaCapturePhysics] Manual Sigma_eff not set. "
           << "Keep Geant4 default neutron capture." << G4endl;
    return;
  }

  const G4double sigmaEff = fDetector->GetManualSigmaEff();

  if (sigmaEff <= 0.0)
  {
    G4cout << "[EffectiveSigmaCapturePhysics] Invalid manual Sigma_eff. "
           << "Keep Geant4 default neutron capture." << G4endl;
    return;
  }

  // --------------------------------------------------
  // 1. 先移除默认的 neutron capture，避免双重计数
  // --------------------------------------------------
  G4VProcess *defaultCapture = nullptr;

  G4ProcessVector *pv = pm->GetProcessList();
  const G4int nProc = pm->GetProcessListLength();

  for (G4int i = 0; i < nProc; ++i)
  {
    G4VProcess *proc = (*pv)[i];
    if (!proc)
      continue;

    const G4String pName = proc->GetProcessName();
    if (pName == "nCapture" ||
        pName == "HadronCapture" ||
        pName == "neutronCapture" ||
        pName == "nCaptureHP")
    {
      defaultCapture = proc;
      break;
    }
  }

  if (defaultCapture)
  {
    pm->RemoveProcess(defaultCapture);
    G4cout << "[EffectiveSigmaCapturePhysics] Removed default neutron process: "
           << defaultCapture->GetProcessName() << G4endl;
  }
  else
  {
    G4cout << "[EffectiveSigmaCapturePhysics] Warning: default nCapture not found."
           << G4endl;
  }

  // --------------------------------------------------
  // 2. 添加你的等效宏观吸收过程
  // --------------------------------------------------
  auto *effCapture = new EffectiveSigmaCaptureProcess(fDetector, sigmaEff);
  pm->AddDiscreteProcess(effCapture);

  G4cout << "[EffectiveSigmaCapturePhysics] Added process: "
         << effCapture->GetProcessName()
         << " with Sigma_eff = " << sigmaEff * cm << " /cm" << G4endl;
}
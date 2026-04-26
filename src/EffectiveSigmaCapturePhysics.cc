#include "EffectiveSigmaCapturePhysics.hh"

#include "DetectorConstruction.hh"
#include "EffectiveSigmaCaptureProcess.hh"

#include "G4Neutron.hh"
#include "G4ProcessManager.hh"
#include "G4ProcessVector.hh"
#include "G4VProcess.hh"
#include "G4SystemOfUnits.hh"
#include "G4ios.hh"

#include <vector>

EffectiveSigmaCapturePhysics::EffectiveSigmaCapturePhysics(DetectorConstruction *detector)
    : G4VPhysicsConstructor("EffectiveSigmaCapturePhysics"),
      fDetector(detector)
{
}

void EffectiveSigmaCapturePhysics::ConstructParticle()
{
  // neutron 已由参考物理表构造，这里无需额外定义。
}

void EffectiveSigmaCapturePhysics::ConstructProcess()
{
  auto *neutron = G4Neutron::Neutron();
  auto *pm = neutron->GetProcessManager();
  if (!pm)
    return;

  // --------------------------------------------------
  // 0. 没有手动指定 Sigma_eff 时，保持默认 Geant4 模式。
  //    注意：这保持了原始约定：不写 /det/setSigmaEffPerCm 就是默认模式。
  // --------------------------------------------------
  if (!fDetector || !fDetector->UseManualSigmaEff())
  {
    G4cout << "[EffectiveSigmaCapturePhysics] Manual Sigma_eff not set. "
           << "Keep Geant4 default neutron capture/inelastic processes." << G4endl;
    return;
  }

  const G4double sigmaEff = fDetector->GetManualSigmaEff();
  if (sigmaEff <= 0.0)
  {
    G4cout << "[EffectiveSigmaCapturePhysics] Invalid manual Sigma_eff. "
           << "Keep Geant4 default neutron capture/inelastic processes." << G4endl;
    return;
  }

  // --------------------------------------------------
  // 1. 手动 Sigma_eff 模式下，移除默认 neutron absorption/conversion 相关过程，
  //    避免 Geant4 默认 B10(n,alpha)Li7 与 EffectiveSigmaCapture 叠加。
  // --------------------------------------------------
  std::vector<G4VProcess *> processesToRemove;
  EffectiveSigmaCaptureProcess *existingEffective = nullptr;

  G4ProcessVector *pv = pm->GetProcessList();
  const G4int nProc = pm->GetProcessListLength();

  for (G4int i = 0; i < nProc; ++i)
  {
    G4VProcess *proc = (*pv)[i];
    if (!proc)
      continue;

    const G4String pName = proc->GetProcessName();

    if (pName == "EffectiveSigmaCapture")
    {
      existingEffective = dynamic_cast<EffectiveSigmaCaptureProcess *>(proc);
      continue;
    }

    if (pName == "nCapture" ||
        pName == "HadronCapture" ||
        pName == "neutronCapture" ||
        pName == "nCaptureHP" ||
        pName == "neutronInelastic" ||
        pName == "neutronInelasticHP")
    {
      processesToRemove.push_back(proc);
    }
  }

  for (auto *proc : processesToRemove)
  {
    if (!proc)
      continue;

    const G4String pName = proc->GetProcessName();
    pm->RemoveProcess(proc);
    G4cout << "[EffectiveSigmaCapturePhysics] Removed default neutron process: "
           << pName << G4endl;
  }

  if (processesToRemove.empty())
  {
    G4cout << "[EffectiveSigmaCapturePhysics] Warning: no default neutron capture/inelastic process was removed."
           << G4endl;
  }

  // --------------------------------------------------
  // 2. 添加或复用等效宏观吸收过程。
  //    真正抽样时 EffectiveSigmaCaptureProcess::GetMeanFreePath()
  //    会动态读取 DetectorConstruction 当前的 Sigma_eff。
  // --------------------------------------------------
  if (existingEffective)
  {
    existingEffective->SetSigmaEff(sigmaEff);
    G4cout << "[EffectiveSigmaCapturePhysics] Reuse process: "
           << existingEffective->GetProcessName()
           << " with Sigma_eff = " << sigmaEff * cm << " /cm" << G4endl;
    return;
  }

  auto *effCapture = new EffectiveSigmaCaptureProcess(fDetector, sigmaEff);
  pm->AddDiscreteProcess(effCapture);

  G4cout << "[EffectiveSigmaCapturePhysics] Added process: "
         << effCapture->GetProcessName()
         << " with Sigma_eff = " << sigmaEff * cm << " /cm" << G4endl;
}

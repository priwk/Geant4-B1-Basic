#include "SteppingAction.hh"
#include "EventAction.hh"
#include "DetectorConstruction.hh"

#include "G4Step.hh"
#include "G4Track.hh"
#include "G4StepPoint.hh"
#include "G4VPhysicalVolume.hh"
#include "G4RunManager.hh"
#include "G4ParticleDefinition.hh"
#include "G4SystemOfUnits.hh"
#include "G4String.hh"
#include "G4ThreeVector.hh"

#include <vector>

// 构造函数
SteppingAction::SteppingAction(EventAction *eventAction)
    : G4UserSteppingAction(),
      fEventAction(eventAction),
      fDetector(0)
{
    fDetector = (DetectorConstruction *)G4RunManager::GetRunManager()->GetUserDetectorConstruction();
}

// 析构函数
SteppingAction::~SteppingAction()
{
}

// 用户步进动作
void SteppingAction::UserSteppingAction(const G4Step *step)
{
    G4Track *track = step->GetTrack();
    G4StepPoint *prePoint = step->GetPreStepPoint();
    G4StepPoint *postPoint = step->GetPostStepPoint();

    if (!track || !prePoint || !postPoint)
    {
        return;
    }

    G4VPhysicalVolume *preVolume = prePoint->GetPhysicalVolume();
    if (!preVolume)
    {
        return;
    }

    G4String volumeName = preVolume->GetName();

    // ---------------------------------------------------------
    // 1. 保留你原有的 alpha / Li7 步长累计
    // ---------------------------------------------------------
    if (track->GetParentID() == 1 && volumeName == "Film")
    {
        G4String particleName = track->GetDefinition()->GetParticleName();
        G4double stepLength = step->GetStepLength();

        if (particleName == "alpha")
        {
            fEventAction->AddAlphaTrackLen(stepLength);
        }
        else if (particleName == "Li7")
        {
            fEventAction->AddLi7TrackLen(stepLength);
        }
    }

    // ---------------------------------------------------------
    // 2. 只记录每个事件第一次俘获
    // ---------------------------------------------------------
    if (fEventAction->HasCapture())
    {
        return;
    }

    // 只看中子
    if (track->GetDefinition()->GetParticleName() != "neutron")
    {
        return;
    }

    // 只看 Film 中发生的步
    if (volumeName != "Film")
    {
        return;
    }

    // 当前步产生的次级粒子
    const std::vector<const G4Track *> *secondaryTracks = step->GetSecondaryInCurrentStep();
    if (!secondaryTracks)
    {
        return;
    }

    G4bool hasAlpha = false;
    G4bool hasLi7 = false;

    for (size_t i = 0; i < secondaryTracks->size(); ++i)
    {
        const G4Track *secondary = (*secondaryTracks)[i];
        if (!secondary)
        {
            continue;
        }

        G4ParticleDefinition *secDef = secondary->GetDefinition();
        G4String secName = secDef->GetParticleName();

        // alpha 判定
        if (secName == "alpha" || (secDef->GetAtomicNumber() == 2 && secDef->GetAtomicMass() == 4))
        {
            hasAlpha = true;
        }

        // Li7 判定（比只写 "Li7" 更稳）
        if (secDef->GetAtomicNumber() == 3 && secDef->GetAtomicMass() == 7)
        {
            hasLi7 = true;
        }
    }

    // 必须同时有 alpha 和 Li7，才认为是 10B(n,alpha)7Li
    if (!(hasAlpha && hasLi7))
    {
        return;
    }

    G4ThreeVector capturePos = postPoint->GetPosition();

    G4double x = capturePos.x();
    G4double y = capturePos.y();
    G4double z = capturePos.z();
    G4double depth = fDetector->GetFilmFrontZ() - z;

    fEventAction->SetCaptureInfo(
        fDetector->GetBNWt(),
        fDetector->GetZnSWt(),
        x,
        y,
        z,
        depth);

    G4cout << "Capture recorded at "
           << x / um << ", "
           << y / um << ", "
           << z / um << " um"
           << " | depth = " << depth / um << " um"
           << G4endl;
}
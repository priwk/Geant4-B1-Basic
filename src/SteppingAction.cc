#include "SteppingAction.hh"
#include "EventAction.hh"
#include "DetectorConstruction.hh"
#include "AnalysisConfig.hh"

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

SteppingAction::SteppingAction(EventAction *eventAction,
                               const AnalysisConfig *config)
    : G4UserSteppingAction(),
      fEventAction(eventAction),
      fDetector(0),
      fAnalysisConfig(config)
{
    fDetector =
        (DetectorConstruction *)G4RunManager::GetRunManager()->GetUserDetectorConstruction();
}

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
    G4bool inFilm = (volumeName == "Film");

    if (!inFilm)
    {
        return;
    }

    G4String particleName = track->GetDefinition()->GetParticleName();

    // ---------------------------------------------------------
    // 1. α / Li7 路径长度累计
    // ---------------------------------------------------------
    if (fAnalysisConfig && fAnalysisConfig->enableTrackLen)
    {
        if (track->GetParentID() == 1)
        {
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
    }

    // ---------------------------------------------------------
    // 2. 沉积能量累计
    // ---------------------------------------------------------
    if (fAnalysisConfig && fAnalysisConfig->enableEdep)
    {
        G4double edep = step->GetTotalEnergyDeposit();
        if (edep > 0.0)
        {
            fEventAction->AddEdep(edep);
        }
    }

    // ---------------------------------------------------------
    // 3. 反应位置记录：只记录一次 10B(n,alpha)7Li
    // ---------------------------------------------------------
    if (!(fAnalysisConfig && fAnalysisConfig->enableReactionPosition))
    {
        return;
    }

    if (fEventAction->HasCapture())
    {
        return;
    }

    if (particleName != "neutron")
    {
        return;
    }

    const std::vector<const G4Track *> *secondaryTracks =
        step->GetSecondaryInCurrentStep();
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

        if (secName == "alpha" ||
            (secDef->GetAtomicNumber() == 2 && secDef->GetAtomicMass() == 4))
        {
            hasAlpha = true;
        }

        if (secDef->GetAtomicNumber() == 3 && secDef->GetAtomicMass() == 7)
        {
            hasLi7 = true;
        }
    }

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
}
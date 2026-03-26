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
#include "G4Run.hh"
#include "G4Event.hh"

#include <vector>

// CSV静态成员定义
std::ofstream SteppingAction::fStepCSVFile;
G4bool SteppingAction::fStepCSVInitialized = false;

// 构造函数
SteppingAction::SteppingAction(EventAction *eventAction,
                               const AnalysisConfig *config)
    : G4UserSteppingAction(),
      fEventAction(eventAction),
      fDetector(0),
      fAnalysisConfig(config)
{
    fDetector =
        (DetectorConstruction *)G4RunManager::GetRunManager()->GetUserDetectorConstruction();

    G4cout << "enableAlphaLiStepCSV = "
           << (fAnalysisConfig ? fAnalysisConfig->enableAlphaLiStepCSV : -1)
           << G4endl;

    if (fAnalysisConfig &&
        fAnalysisConfig->enableAlphaLiStepCSV &&
        !fStepCSVInitialized)
    {
        fStepCSVFile.open("alpha_li_steps.csv");

        if (fStepCSVFile.is_open())
        {
            fStepCSVFile
                << "runID,eventID,trackID,stepID,particle,"
                << "bn_wt,zns_wt,thickness_um,"
                << "capture_x_um,capture_y_um,capture_z_um,capture_depth_um,"
                << "x_pre_um,y_pre_um,z_pre_um,"
                << "x_post_um,y_post_um,z_post_um,"
                << "x_mid_um,y_mid_um,z_mid_um,depth_mid_um,"
                << "step_len_um,edep_keV,"
                << "ekin_pre_keV,ekin_post_keV"
                << G4endl;

            fStepCSVInitialized = true;
        }
        else
        {
            G4cerr << "Error: cannot open alpha_li_steps.csv" << G4endl;
        }
    }
}

SteppingAction::~SteppingAction()
{
    if (fStepCSVFile.is_open())
    {
        fStepCSVFile.close();
    }
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
    if (fAnalysisConfig &&
        (fAnalysisConfig->enableEdep || fAnalysisConfig->enableLightYield))
    {
        G4double edep = step->GetTotalEnergyDeposit();
        if (edep > 0.0)
        {
            fEventAction->AddEdep(edep);
        }
    }

    // ---------------------------------------------------------
    // 2.5 α / Li7 step 级源项输出
    // ---------------------------------------------------------
    if (fAnalysisConfig && fAnalysisConfig->enableAlphaLiStepCSV)
    {
        G4ParticleDefinition *pdef = track->GetDefinition();
        G4bool isAlpha = (particleName == "alpha") ||
                         (pdef->GetAtomicNumber() == 2 && pdef->GetAtomicMass() == 4);

        G4bool isLi7 = (particleName == "Li7") ||
                       (pdef->GetAtomicNumber() == 3 && pdef->GetAtomicMass() == 7);

        if (isAlpha || isLi7)
        {
            if (!fAnalysisConfig->stepCSVOnlyPrimaryCaptureProducts ||
                track->GetParentID() == 1)
            {
                G4double edep = step->GetTotalEnergyDeposit();

                if (!fAnalysisConfig->stepCSVOnlyWithEdep || edep > 0.0)
                {
                    G4ThreeVector prePos = prePoint->GetPosition();
                    G4ThreeVector postPos = postPoint->GetPosition();
                    G4ThreeVector midPos = 0.5 * (prePos + postPos);

                    G4ThreeVector vertexPos = track->GetVertexPosition();
                    G4double captureDepth = fDetector->GetFilmFrontZ() - vertexPos.z();

                    G4double depthMid = fDetector->GetFilmFrontZ() - midPos.z();
                    G4double stepLen = step->GetStepLength();

                    G4double ekinPre = prePoint->GetKineticEnergy();
                    G4double ekinPost = postPoint->GetKineticEnergy();

                    const G4Run *currentRun =
                        G4RunManager::GetRunManager()->GetCurrentRun();
                    G4int runID = (currentRun ? currentRun->GetRunID() : -1);

                    const G4Event *currentEvent = G4RunManager::GetRunManager()->GetCurrentEvent();
                    G4int eventID = (currentEvent ? currentEvent->GetEventID() : -1);

                    if (fStepCSVFile.is_open())
                    {
                        fStepCSVFile
                            << runID << ","
                            << eventID << ","
                            << track->GetTrackID() << ","
                            << track->GetCurrentStepNumber() << ","
                            << (isAlpha ? "alpha" : "Li7") << ","
                            << fDetector->GetBNWt() << ","
                            << fDetector->GetZnSWt() << ","
                            << fDetector->GetFilmThickness() / um << ","
                            << vertexPos.x() / um << ","
                            << vertexPos.y() / um << ","
                            << vertexPos.z() / um << ","
                            << captureDepth / um << ","
                            << prePos.x() / um << ","
                            << prePos.y() / um << ","
                            << prePos.z() / um << ","
                            << postPos.x() / um << ","
                            << postPos.y() / um << ","
                            << postPos.z() / um << ","
                            << midPos.x() / um << ","
                            << midPos.y() / um << ","
                            << midPos.z() / um << ","
                            << depthMid / um << ","
                            << stepLen / um << ","
                            << edep / keV << ","
                            << ekinPre / keV << ","
                            << ekinPost / keV
                            << G4endl;
                    }
                    else
                    {
                        G4cerr << "Error: alpha_li_steps.csv is not open." << G4endl;
                    }
                }
            }
        }
    }

    // ---------------------------------------------------------
    // 3. 透过判定：主中子从 Film 后表面穿出
    // ---------------------------------------------------------
    if (fAnalysisConfig && fAnalysisConfig->enableAttenuation)
    {
        if (track->GetParentID() == 0 && particleName == "neutron")
        {
            G4VPhysicalVolume *postVolume = postPoint->GetPhysicalVolume();
            G4String postVolumeName = (postVolume ? postVolume->GetName() : "");

            // 当前这一步从 Film 走到了 Film 外
            if (postVolumeName != "Film")
            {
                // 由于源从 +z 打向 -z，后表面在更小的 z 位置
                G4double backZ = fDetector->GetFilmFrontZ() - fDetector->GetFilmThickness();

                if (postPoint->GetPosition().z() <= backZ)
                {
                    fEventAction->SetTransmitted();
                }
            }
        }
    }

    // ---------------------------------------------------------
    // 4. 反应位置记录：只记录一次 10B(n,alpha)7Li
    // ---------------------------------------------------------
    G4bool needCaptureTag =
        (fAnalysisConfig &&
         (fAnalysisConfig->enableReactionPosition ||
          (fAnalysisConfig->enableLightYield &&
           fAnalysisConfig->lightOnlyForCaptureEvents)));
    if (!needCaptureTag)
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
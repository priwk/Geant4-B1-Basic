#include "SteppingAction.hh"
#include "EventAction.hh"
#include "RunAction.hh"
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
#include "G4StepStatus.hh"
#include "G4VProcess.hh"

#include "G4OpticalPhoton.hh"
#include "G4OpBoundaryProcess.hh"
#include "G4ProcessManager.hh"
#include "G4ProcessVector.hh"

#include <vector>
#include <fstream>
#include <filesystem>
#include <string>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace
{
    std::string FormatRatioTag(G4double bnPart, G4double znsPart)
    {
        auto trimNumber = [](G4double x)
        {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(3) << x;
            std::string s = oss.str();
            while (!s.empty() && s.back() == '0')
                s.pop_back();
            if (!s.empty() && s.back() == '.')
                s.pop_back();
            return s;
        };

        return trimNumber(bnPart) + "-" + trimNumber(znsPart);
    }

    std::ofstream gStepCSVFile;
    std::string gStepCSVKey;

    G4bool EnsureStepCSVForLabel(G4int label, const std::string &ratioTag)
    {
        namespace fs = std::filesystem;

        const std::string key = ratioTag + "/" + std::to_string(label);

        if (gStepCSVFile.is_open() && gStepCSVKey == key)
        {
            return true;
        }

        if (gStepCSVFile.is_open())
        {
            gStepCSVFile.close();
        }

        fs::path outDir = fs::current_path().parent_path() / "Data" / ratioTag / "alpha_li_steps";

        std::error_code ec;
        fs::create_directories(outDir, ec);
        if (ec)
        {
            G4cerr << "Error: cannot create directory "
                   << outDir.string()
                   << " | " << ec.message() << G4endl;
            return false;
        }

        fs::path outPath =
            outDir / (std::to_string(label) + "_alpha_li_steps.csv");

        const bool fileExists = fs::exists(outPath);

        gStepCSVFile.open(outPath.string(), std::ios::out | std::ios::app);
        if (!gStepCSVFile.is_open())
        {
            G4cerr << "Error: cannot open " << outPath.string() << G4endl;
            return false;
        }

        gStepCSVKey = key;

        if (!fileExists)
        {
            gStepCSVFile
                << "source_label,eventID,trackID,stepID,particle,"
                << "bn_wt,zns_wt,thickness_um,"
                << "capture_x_um,capture_y_um,capture_z_um,capture_depth_um,"
                << "x_pre_um,y_pre_um,z_pre_um,"
                << "x_post_um,y_post_um,z_post_um,"
                << "x_mid_um,y_mid_um,z_mid_um,depth_mid_um,"
                << "step_len_um,edep_keV,"
                << "ekin_pre_keV,ekin_post_keV"
                << G4endl;
        }

        return true;
    }

    void CloseStepCSV()
    {
        if (gStepCSVFile.is_open())
        {
            gStepCSVFile.close();
        }
        gStepCSVKey.clear();
    }

    // 获取光子边界过程
    G4OpBoundaryProcess *GetOpBoundaryProcess()
    {
        static G4OpBoundaryProcess *boundary = nullptr;
        if (boundary)
        {
            return boundary;
        }

        auto *pm = G4OpticalPhoton::OpticalPhoton()->GetProcessManager();
        if (!pm)
        {
            return nullptr;
        }

        auto *pv = pm->GetProcessList();
        const G4int nProc = pm->GetProcessListLength();

        for (G4int i = 0; i < nProc; ++i)
        {
            auto *proc = (*pv)[i];
            boundary = dynamic_cast<G4OpBoundaryProcess *>(proc);
            if (boundary)
            {
                return boundary;
            }
        }

        return nullptr;
    }
}

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
}

SteppingAction::~SteppingAction()
{
    CloseStepCSV();
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
    // 0. optical photon 读出统计：从 Film 背面(-z 面)出射到 World
    // ---------------------------------------------------------
    if (particleName == "opticalphoton")
    {
        G4VPhysicalVolume *postVolume = postPoint->GetPhysicalVolume();
        G4String postVolumeName = (postVolume ? postVolume->GetName() : "");

        if (postPoint->GetStepStatus() == fGeomBoundary)
        {
            if (volumeName == "Film" && postVolumeName == "World")
            {
                G4double backZ = fDetector->GetFilmBackZ();
                G4double zPost = postPoint->GetPosition().z();

                if (zPost <= backZ + 1e-9 * mm)
                {
                    G4OpBoundaryProcess *boundary = GetOpBoundaryProcess();

                    if (boundary)
                    {
                        const auto status = boundary->GetStatus();

                        const G4bool reallyEscaped =
                            (status == Transmission ||
                             status == FresnelRefraction);

                        if (reallyEscaped)
                        {
                            G4ThreeVector pos = postPoint->GetPosition();
                            fEventAction->AddDetectedPhoton(pos.x(), pos.y());

                            // 既然你只关心“到达读出面”的计数，
                            // 这里可以直接杀掉，避免后续无意义追踪
                            track->SetTrackStatus(fStopAndKill);
                        }
                    }
                }
            }
        }

        return;
    }

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

                    G4int sourceLabel =
                        static_cast<G4int>(std::lround(fDetector->GetFilmThickness() / um));

                    std::string ratioTag =
                        FormatRatioTag(fDetector->GetBNWt(), fDetector->GetZnSWt());

                    if (EnsureStepCSVForLabel(sourceLabel, ratioTag))
                    {
                        gStepCSVFile
                            << sourceLabel << ","
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
                        G4cerr << "Error: per-label alpha_li_steps CSV is not open." << G4endl;
                    }
                }
            }
        }
    }

    // ---------------------------------------------------------
    // 2.75 主中子在 Film 内的路径长度累计（用于 Sigma_eff = N / ΣL）
    // ---------------------------------------------------------
    if (track->GetParentID() == 0 && particleName == "neutron")
    {
        G4double stepLength = step->GetStepLength();
        if (stepLength > 0.0 && volumeName == "Film")
        {
            if (const auto *runAction =
                    static_cast<const RunAction *>(
                        G4RunManager::GetRunManager()->GetUserRunAction()))
            {
                const_cast<RunAction *>(runAction)->AddNeutronTrackLength(stepLength);
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
            // 严谨判定 1：只有粒子当前步正好踩在几何边界上，才进行穿透判断
            if (postPoint->GetStepStatus() == fGeomBoundary)
            {
                G4VPhysicalVolume *postVolume = postPoint->GetPhysicalVolume();
                G4String postVolumeName = (postVolume ? postVolume->GetName() : "");

                // 严谨判定 2：确实是从 Film 内部走到了外面
                if (volumeName == "Film" && postVolumeName != "Film")
                {
                    // 直接调用现成的后表面 Z 坐标，并加上 1e-6 mm (1纳米) 的浮点数容差
                    // 彻底解决 230 um 这种无法被二进制精确表示的坐标截断问题
                    G4double backZ = fDetector->GetFilmBackZ();

                    if (postPoint->GetPosition().z() <= backZ + 1e-6 * mm)
                    {
                        fEventAction->SetTransmitted();
                    }
                }
            }
        }
    }

    // ---------------------------------------------------------
    // 4. 识别一次俘获
    //    4.1 若使用 EffectiveSigmaCapture，则直接以该过程发生点作为等效俘获点
    //    4.2 否则保留原有 10B(n,alpha)7Li 次级粒子识别逻辑
    // ---------------------------------------------------------
    if (fEventAction->HasCapture())
    {
        return;
    }

    if (particleName != "neutron")
    {
        return;
    }

    // ---------- 4.1 优先识别自定义等效吸收过程 ----------
    const G4VProcess *proc = postPoint->GetProcessDefinedStep();
    if (proc && proc->GetProcessName() == "EffectiveSigmaCapture")
    {
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

        return;
    }

    // ---------- 4.2 仍保留原始 alpha + Li7 识别 ----------
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
#include "PrimaryGeneratorAction.hh"
#include "EventAction.hh"
#include "AnalysisConfig.hh"
#include "DetectorConstruction.hh"

#include "G4Event.hh"
#include "G4PrimaryVertex.hh"
#include "G4ParticleTable.hh"
#include "G4ParticleDefinition.hh"
#include "G4GeneralParticleSource.hh"
#include "G4SystemOfUnits.hh"
#include "G4ios.hh"
#include "Randomize.hh"
#include "G4RunManager.hh"

#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <cmath>
#include <algorithm>
#include <filesystem>

// ============================================================
// 一个 step 源项记录，存储了上一阶段的粒子信息
// ============================================================

namespace
{
  G4ThreeVector SampleIsotropicDirection() // 数学上实现4π各向同性发射
  {
    const G4double cosTheta = 2.0 * G4UniformRand() - 1.0;
    const G4double sinTheta = std::sqrt(std::max(0.0, 1.0 - cosTheta * cosTheta));
    const G4double phi = CLHEP::twopi * G4UniformRand();

    return G4ThreeVector(
        sinTheta * std::cos(phi),
        sinTheta * std::sin(phi),
        cosTheta);
  }

  G4ThreeVector SamplePointOnSegment(const StepSourceRecord &rec) // 线源抽样
  {
    const G4double u = G4UniformRand();

    const G4double x = rec.xPre + u * (rec.xPost - rec.xPre);
    const G4double y = rec.yPre + u * (rec.yPost - rec.yPre);
    const G4double z = rec.zPre + u * (rec.zPost - rec.zPre);

    return G4ThreeVector(x * um, y * um, z * um);
  }

  G4ThreeVector MidPoint(const StepSourceRecord &rec) // 线段中点
  {
    return G4ThreeVector(rec.xMid * um, rec.yMid * um, rec.zMid * um);
  }
}

// ============================================================
// 构造函数
// ============================================================
PrimaryGeneratorAction::PrimaryGeneratorAction(EventAction *eventAction,
                                               const AnalysisConfig *config)
    : G4VUserPrimaryGeneratorAction(),
      fParticleGun(nullptr),
      fEventAction(eventAction),
      fAnalysisConfig(config),
      fReplayInitialized(false),
      fReplayEventCursor(0),
      fLoadedReplayThicknessUm(-1)
{
  fParticleGun = new G4GeneralParticleSource();
}

// ============================================================
// 析构函数
// ============================================================
PrimaryGeneratorAction::~PrimaryGeneratorAction()
{
  delete fParticleGun;
}

// 重置重放缓存
void PrimaryGeneratorAction::ResetReplayCache()
{
  fReplayInitialized = false;
  fReplayEventCursor = 0;
  fReplayEvents.clear();
  fLoadedReplayThicknessUm = -1;
}

// 获取当前厚度标签
G4int PrimaryGeneratorAction::GetCurrentThicknessLabelUm() const
{
  auto *runManager = G4RunManager::GetRunManager();
  if (!runManager)
  {
    G4cerr << "Error: G4RunManager is null, cannot get detector." << G4endl;
    return -1;
  }

  auto *det = dynamic_cast<const DetectorConstruction *>(
      runManager->GetUserDetectorConstruction());

  if (!det)
  {
    G4cerr << "Error: cannot cast detector to DetectorConstruction." << G4endl;
    return -1;
  }

  const G4double thickness = det->GetFilmThickness();
  if (thickness <= 0.0)
  {
    G4cerr << "Error: invalid film thickness = " << thickness / um
           << " um." << G4endl;
    return -1;
  }

  return static_cast<G4int>(std::lround(thickness / um));
}

// ============================================================
// 读取 alpha_li_steps.csv
// 按 eventID 分组
// ============================================================
void PrimaryGeneratorAction::LoadReplayCSV()
{
  if (fReplayInitialized)
  {
    return;
  }

  fReplayEvents.clear();
  fReplayEventCursor = 0;

  const G4int thicknessLabelUm = GetCurrentThicknessLabelUm();
  if (thicknessLabelUm <= 0)
  {
    G4cerr << "Error: invalid current thickness label, cannot load replay CSV."
           << G4endl;
    return;
  }

  G4cout << "Replay target thickness = "
         << thicknessLabelUm << " um" << G4endl;

  namespace fs = std::filesystem;
  fs::path replayPath =
      fs::current_path().parent_path() / "Data" / "alpha_li_steps" /
      (std::to_string(thicknessLabelUm) + "_alpha_li_steps.csv");

  std::ifstream fin(replayPath.string());
  if (!fin.is_open())
  {
    G4cerr << "Error: cannot open " << replayPath.string()
           << " for replay." << G4endl;
    return;
  }

  std::string line;
  if (!std::getline(fin, line))
  {
    G4cerr << "Error: " << replayPath.string()
           << " is empty." << G4endl;
    return;
  }

  std::map<G4int, std::vector<StepSourceRecord>> grouped;
  G4double fileThicknessUm = -1.0;
  G4int fileSourceLabel = -1;

  while (std::getline(fin, line))
  {
    if (line.empty())
      continue;

    std::stringstream ss(line);
    std::string cell;
    std::vector<std::string> cols;

    while (std::getline(ss, cell, ','))
    {
      cols.push_back(cell);
    }

    if (cols.size() < 26)
    {
      G4cerr << "Warning: malformed replay row skipped: " << line << G4endl;
      continue;
    }

    StepSourceRecord rec;

    try
    {
      rec.sourceLabel = std::stoi(cols[0]);
      rec.eventID = std::stoi(cols[1]);
      rec.trackID = std::stoi(cols[2]);
      rec.stepID = std::stoi(cols[3]);
      rec.particle = cols[4].c_str();

      rec.bnWt = std::stod(cols[5]);
      rec.znsWt = std::stod(cols[6]);
      rec.thicknessUm = std::stod(cols[7]);

      rec.captureX = std::stod(cols[8]);
      rec.captureY = std::stod(cols[9]);
      rec.captureZ = std::stod(cols[10]);
      rec.captureDepth = std::stod(cols[11]);

      rec.xPre = std::stod(cols[12]);
      rec.yPre = std::stod(cols[13]);
      rec.zPre = std::stod(cols[14]);

      rec.xPost = std::stod(cols[15]);
      rec.yPost = std::stod(cols[16]);
      rec.zPost = std::stod(cols[17]);

      rec.xMid = std::stod(cols[18]);
      rec.yMid = std::stod(cols[19]);
      rec.zMid = std::stod(cols[20]);
      rec.depthMid = std::stod(cols[21]);

      rec.stepLenUm = std::stod(cols[22]);
      rec.edepKeV = std::stod(cols[23]);
      rec.ekinPreKeV = std::stod(cols[24]);
      rec.ekinPostKeV = std::stod(cols[25]);
    }
    catch (...)
    {
      G4cerr << "Warning: failed to parse replay row, skipped." << G4endl;
      continue;
    }

    if (fileSourceLabel < 0)
      fileSourceLabel = rec.sourceLabel;
    if (fileThicknessUm < 0.0)
      fileThicknessUm = rec.thicknessUm;

    grouped[rec.eventID].push_back(rec);
  }

  for (auto &kv : grouped)
  {
    auto &steps = kv.second;

    std::sort(steps.begin(), steps.end(),
              [](const StepSourceRecord &a, const StepSourceRecord &b)
              {
                if (a.trackID != b.trackID)
                  return a.trackID < b.trackID;
                return a.stepID < b.stepID;
              });

    fReplayEvents.push_back(steps);
  }

  std::sort(fReplayEvents.begin(), fReplayEvents.end(),
            [](const std::vector<StepSourceRecord> &a,
               const std::vector<StepSourceRecord> &b)
            {
              if (a.empty() || b.empty())
                return a.size() < b.size();
              return a.front().eventID < b.front().eventID;
            });

  if (fileThicknessUm > 0.0)
  {
    const G4int fileThicknessLabel =
        static_cast<G4int>(std::lround(fileThicknessUm));

    if (fileThicknessLabel != thicknessLabelUm)
    {
      G4cerr << "Warning: current detector thickness = "
             << thicknessLabelUm << " um, but replay file content says "
             << fileThicknessUm << " um." << G4endl;
    }
  }

  fReplayInitialized = true;
  fLoadedReplayThicknessUm = thicknessLabelUm;

  G4cout << "Replay loaded: " << fReplayEvents.size()
         << " events from " << replayPath.string()
         << " | source_label in file = " << fileSourceLabel
         << " | source thickness in file = " << fileThicknessUm << " um"
         << G4endl;
}

// ============================================================
// 生成主粒子
// ============================================================
void PrimaryGeneratorAction::GeneratePrimaries(G4Event *anEvent)
{

  // -----------------------------
  // 普通模式：保留你原来的 GPS 中子入射
  // -----------------------------
  if (!fAnalysisConfig || !fAnalysisConfig->enableSourceReplay)
  {
    fParticleGun->GeneratePrimaryVertex(anEvent);

    G4PrimaryVertex *vertex = anEvent->GetPrimaryVertex();
    if (vertex && fEventAction)
    {
      fEventAction->SetSourcePosition(vertex->GetX0(), vertex->GetY0());
    }
    return;
  }

  // -----------------------------
  // 第二阶段：CSV replay 模式
  // -----------------------------
  const G4int currentThicknessUm = GetCurrentThicknessLabelUm();
  if (currentThicknessUm <= 0)
  {
    G4cerr << "Warning: invalid detector thickness in replay mode." << G4endl;
    return;
  }

  if (fReplayInitialized && currentThicknessUm != fLoadedReplayThicknessUm)
  {
    G4cout << "Replay thickness changed from "
           << fLoadedReplayThicknessUm << " um to "
           << currentThicknessUm << " um, reload replay source." << G4endl;
    ResetReplayCache();
  }

  if (!fReplayInitialized)
  {
    LoadReplayCSV();
  }

  if (fReplayEvents.empty())
  {
    G4cerr << "Warning: replay mode enabled, but no replay events loaded." << G4endl;
    return;
  }

  // 检查游标是否越界
  if (fReplayEventCursor >= fReplayEvents.size())
  {
    if (fAnalysisConfig->replayLoopEvents)
    {
      fReplayEventCursor = 0;
    }
    else
    {
      G4cout << "Replay finished: no more events to replay. Abort current run."
             << G4endl;

      auto *runManager = G4RunManager::GetRunManager();
      if (runManager)
      {
        runManager->AbortRun(false);
      }
      return;
    }
  }

  // 获取当前步骤
  const auto &steps = fReplayEvents[fReplayEventCursor];
  ++fReplayEventCursor;

  if (steps.empty())
  {
    return;
  }

  // 查找光子粒子
  auto *opticalPhoton =
      G4ParticleTable::GetParticleTable()->FindParticle("opticalphoton");

  if (!opticalPhoton)
  {
    G4cerr << "Error: opticalphoton definition not found." << G4endl;
    return;
  }

  G4int replayPhotonCount = 0;
  G4int totalBudget = 0;
  G4bool sourceTagged = false;

  // 遍历当前步骤的所有记录
  for (const auto &rec : steps)
  {
    if (rec.edepKeV <= 0.0)
    {
      continue;
    }

    G4int nThisStep = fAnalysisConfig->replayPhotonsPerStep;

    if (fAnalysisConfig->replayUseEdepWeight)
    {
      const G4double weighted =
          fAnalysisConfig->replayScale * rec.edepKeV;
      nThisStep = std::max(1, static_cast<G4int>(std::round(weighted)));
    }

    if (fAnalysisConfig->replayMaxPhotonsPerEvent > 0)
    {
      const G4int remain =
          fAnalysisConfig->replayMaxPhotonsPerEvent - totalBudget;
      if (remain <= 0)
      {
        break;
      }
      nThisStep = std::min(nThisStep, remain);
    }

    for (G4int i = 0; i < nThisStep; ++i)
    {
      G4ThreeVector pos =
          fAnalysisConfig->replayUseLineSource ? SamplePointOnSegment(rec)
                                               : MidPoint(rec);

      G4ThreeVector dir = SampleIsotropicDirection();

      auto *vertex = new G4PrimaryVertex(pos, 0.0);
      auto *particle = new G4PrimaryParticle(opticalPhoton);
      particle->SetMomentumDirection(dir);
      particle->SetKineticEnergy(fAnalysisConfig->replayPhotonEnergy_eV * eV);

      // 给 optical photon 设置一个与传播方向垂直的随机偏振
      G4ThreeVector e1 = dir.orthogonal().unit();
      G4ThreeVector e2 = dir.cross(e1).unit();
      G4double psi = CLHEP::twopi * G4UniformRand();
      G4ThreeVector pol = std::cos(psi) * e1 + std::sin(psi) * e2;

      particle->SetPolarization(pol.x(), pol.y(), pol.z());

      vertex->SetPrimary(particle);
      anEvent->AddPrimaryVertex(vertex);

      ++replayPhotonCount;
      ++totalBudget;
    }

    if (fEventAction && !sourceTagged)
    {
      fEventAction->SetSourcePosition(rec.xMid * um, rec.yMid * um);
      fEventAction->SetCaptureInfo(rec.bnWt,
                                   rec.znsWt,
                                   rec.captureX * um,
                                   rec.captureY * um,
                                   rec.captureZ * um,
                                   rec.captureDepth * um);
      sourceTagged = true;
    }
  }

  if (fEventAction)
  {
    fEventAction->SetReplayPhotonCount(replayPhotonCount);
  }
}
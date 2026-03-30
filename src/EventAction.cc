#include "EventAction.hh"

#include "RunAction.hh"
#include "AnalysisConfig.hh"
#include "DetectorConstruction.hh"

#include "G4Event.hh"
#include "G4SystemOfUnits.hh"
#include "G4ios.hh"
#include "G4RunManager.hh"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <string>
#include <algorithm>

// =========================
// 静态成员定义
// 说明：fLightCSVFile / fLightCSVInitialized 仅为兼容当前头文件，
// 本版已不再使用 light_events.csv
// =========================
std::ofstream EventAction::fCSVFile;
G4bool EventAction::fCSVInitialized = false;

std::ofstream EventAction::fEdepCSVFile;
G4bool EventAction::fEdepCSVInitialized = false;

// =========================
// 匿名命名空间：辅助函数
// =========================
namespace
{
  std::ofstream gCaptureCSVFile;
  std::string gCaptureCSVKey;

  std::ofstream gEdepCSVFile;
  std::string gEdepCSVKey;

  std::ofstream gOpticalCSVFile;
  std::string gOpticalCSVKey;

  std::filesystem::path GetDataDir()
  {
    namespace fs = std::filesystem;

    fs::path dataDir = fs::current_path().parent_path() / "Data";

    std::error_code ec;
    fs::create_directories(dataDir, ec);
    if (ec)
    {
      G4cerr << "Error: cannot create directory "
             << dataDir.string()
             << " | " << ec.message() << G4endl;
    }

    return dataDir;
  }

  // 计算 RMS
  G4double SafeRMS(G4double sum, G4double sum2, G4int n)
  {
    if (n <= 0)
      return 0.0;

    const G4double mean = sum / n;
    const G4double var = sum2 / n - mean * mean;
    return (var > 0.0) ? std::sqrt(var) : 0.0;
  }

  // 捕获位置CSV
  G4bool EnsureCapturePositionCSVForThickness(G4int thicknessLabel)
  {
    namespace fs = std::filesystem;

    const std::string key = std::to_string(thicknessLabel);

    if (gCaptureCSVFile.is_open() && gCaptureCSVKey == key)
    {
      return true;
    }

    if (gCaptureCSVFile.is_open())
    {
      gCaptureCSVFile.close();
    }

    fs::path outDir = GetDataDir() / "neutron_capture_positions";

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
        outDir / (std::to_string(thicknessLabel) +
                  "_neutron_capture_positions.csv");

    const bool fileExists = fs::exists(outPath);

    gCaptureCSVFile.open(outPath.string(), std::ios::out | std::ios::app);
    if (!gCaptureCSVFile.is_open())
    {
      G4cerr << "Error: cannot open " << outPath.string() << G4endl;
      return false;
    }

    gCaptureCSVKey = key;

    if (!fileExists)
    {
      gCaptureCSVFile
          << "eventID,thickness_um,bn_wt,zns_wt,"
          << "capture_x_um,capture_y_um,"
          << "corr_x_um,corr_y_um,depth_um"
          << G4endl;
    }

    return true;
  }

  // 能量沉积CSV
  G4bool EnsureEdepCSVForThickness(G4int thicknessLabel)
  {
    namespace fs = std::filesystem;

    const std::string key = std::to_string(thicknessLabel);

    if (gEdepCSVFile.is_open() && gEdepCSVKey == key)
    {
      return true;
    }

    if (gEdepCSVFile.is_open())
    {
      gEdepCSVFile.close();
    }

    fs::path outDir = GetDataDir() / "energy_deposition";

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
        outDir / (std::to_string(thicknessLabel) +
                  "_energy_deposition_events.csv");

    const bool fileExists = fs::exists(outPath);

    gEdepCSVFile.open(outPath.string(), std::ios::out | std::ios::app);
    if (!gEdepCSVFile.is_open())
    {
      G4cerr << "Error: cannot open " << outPath.string() << G4endl;
      return false;
    }

    gEdepCSVKey = key;

    if (!fileExists)
    {
      gEdepCSVFile
          << "eventID,thickness_um,edep_keV"
          << G4endl;
    }

    return true;
  }

  // 光学读取CSV
  G4bool EnsureOpticalCSVForThickness(G4int thicknessLabel)
  {
    namespace fs = std::filesystem;

    const std::string key = std::to_string(thicknessLabel);

    if (gOpticalCSVFile.is_open() && gOpticalCSVKey == key)
    {
      return true;
    }

    if (gOpticalCSVFile.is_open())
    {
      gOpticalCSVFile.close();
    }

    fs::path outDir = GetDataDir() / "optical_readout";

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
        outDir / (std::to_string(thicknessLabel) +
                  "_optical_readout_events.csv");

    const bool fileExists = fs::exists(outPath);

    gOpticalCSVFile.open(outPath.string(), std::ios::out | std::ios::app);
    if (!gOpticalCSVFile.is_open())
    {
      G4cerr << "Error: cannot open " << outPath.string() << G4endl;
      return false;
    }

    gOpticalCSVKey = key;

    if (!fileExists)
    {
      gOpticalCSVFile
          << "eventID,thickness_um,"
          << "bn_wt,zns_wt,"
          << "replay_photons,escaped_photons,escape_efficiency,"
          << "spot_mean_x_um,spot_mean_y_um,"
          << "spot_rms_x_um,spot_rms_y_um,"
          << "has_capture,source_x_um,source_y_um,"
          << "capture_x_um,capture_y_um,depth_um"
          << G4endl;
    }

    return true;
  }

  void CloseCapturePositionCSV()
  {
    if (gCaptureCSVFile.is_open())
    {
      gCaptureCSVFile.close();
    }
    gCaptureCSVKey.clear();
  }

  void CloseEdepCSV()
  {
    if (gEdepCSVFile.is_open())
    {
      gEdepCSVFile.close();
    }
    gEdepCSVKey.clear();
  }

  void CloseOpticalCSV()
  {
    if (gOpticalCSVFile.is_open())
    {
      gOpticalCSVFile.close();
    }
    gOpticalCSVKey.clear();
  }
}

// =========================
// 构造函数
// =========================
EventAction::EventAction(RunAction *runAction, const AnalysisConfig *config)
    : G4UserEventAction(),
      fRunAction(runAction),
      fAnalysisConfig(config),
      fAlphaTrackLen(0.0),
      fLi7TrackLen(0.0),
      fEdep(0.0),
      fGeneratedPhotons(0.0),
      fSourceX(0.0),
      fSourceY(0.0),
      fHasCapture(false),
      fHasTransmit(false),
      fBNWt(0.0),
      fZnSWt(0.0),
      fCaptureX(0.0),
      fCaptureY(0.0),
      fCaptureZ(0.0),
      fDepth(0.0),
      fReplayPhotonCount(0),
      fDetectedPhotonCount(0),
      fDetectedSumX(0.0),
      fDetectedSumY(0.0),
      fDetectedSumX2(0.0),
      fDetectedSumY2(0.0)
{
}

// =========================
// 析构函数
// =========================
EventAction::~EventAction()
{
  CloseCapturePositionCSV();
  CloseEdepCSV();
  CloseOpticalCSV();
}

// =========================
// 每个事件开始：清零
// =========================
void EventAction::BeginOfEventAction(const G4Event *)
{
  fAlphaTrackLen = 0.0;
  fLi7TrackLen = 0.0;
  fEdep = 0.0;
  fGeneratedPhotons = 0.0;

  fHasTransmit = false;

  fDetectedPhotonCount = 0;
  fDetectedSumX = 0.0;
  fDetectedSumY = 0.0;
  fDetectedSumX2 = 0.0;
  fDetectedSumY2 = 0.0;

  if (fRunAction)
  {
    fRunAction->CountIncident();
  }
}

// =========================
// 每个事件结束：汇总并输出
// =========================
void EventAction::EndOfEventAction(const G4Event *event)
{
  G4bool doPrint = false;

  // -------------------------
  // 获取厚度
  // -------------------------
  const DetectorConstruction *detector =
      static_cast<const DetectorConstruction *>(
          G4RunManager::GetRunManager()->GetUserDetectorConstruction());

  G4double thicknessUm = -1.0;
  if (detector)
  {
    thicknessUm = detector->GetFilmThickness() / um;
  }

  const G4int thicknessLabel =
      static_cast<G4int>(std::lround(thicknessUm));

  // -------------------------
  // 是否打印
  // -------------------------
  if (fAnalysisConfig && fAnalysisConfig->enableVerboseEventPrint)
  {
    if (fAnalysisConfig->verboseEveryNEvents <= 1)
    {
      doPrint = true;
    }
    else if (event->GetEventID() % fAnalysisConfig->verboseEveryNEvents == 0)
    {
      doPrint = true;
    }
  }

  // -------------------------
  // 兼容保留：生成光子数统计
  // light_events.csv 已删除，但 GetGeneratedPhotons() 仍可能被别处使用
  // -------------------------
  const G4bool hasReplayStats =
      (fReplayPhotonCount > 0 || fDetectedPhotonCount > 0);

  if (hasReplayStats)
  {
    fGeneratedPhotons = static_cast<G4double>(fReplayPhotonCount);
  }
  else
  {
    G4bool countLightThisEvent = true;
    if (fAnalysisConfig &&
        fAnalysisConfig->enableLightYield &&
        fAnalysisConfig->lightOnlyForCaptureEvents)
    {
      countLightThisEvent = fHasCapture;
    }

    fGeneratedPhotons = 0.0;

    if (fAnalysisConfig &&
        fAnalysisConfig->enableLightYield &&
        countLightThisEvent)
    {
      fGeneratedPhotons =
          (fEdep / MeV) * fAnalysisConfig->lightYieldPhotonsPerMeV;
    }
  }

  // -------------------------
  // 第二阶段：读出面统计量
  // -------------------------
  G4double detectEfficiency = 0.0;
  G4double meanX = 0.0;
  G4double meanY = 0.0;
  G4double rmsX = 0.0;
  G4double rmsY = 0.0;

  if (fReplayPhotonCount > 0)
  {
    detectEfficiency =
        static_cast<G4double>(fDetectedPhotonCount) /
        static_cast<G4double>(fReplayPhotonCount);
  }

  if (fDetectedPhotonCount > 0)
  {
    meanX = fDetectedSumX / fDetectedPhotonCount;
    meanY = fDetectedSumY / fDetectedPhotonCount;
    rmsX = SafeRMS(fDetectedSumX, fDetectedSumX2, fDetectedPhotonCount);
    rmsY = SafeRMS(fDetectedSumY, fDetectedSumY2, fDetectedPhotonCount);
  }

  // -------------------------
  // 控制台打印
  // -------------------------
  if (doPrint)
  {
    G4cout << "Event " << event->GetEventID();

    if (fAnalysisConfig && fAnalysisConfig->enableTrackLen)
    {
      G4cout
          << " | alpha track length = " << fAlphaTrackLen / um << " um"
          << " | Li7 track length = " << fLi7TrackLen / um << " um";
    }

    if (fAnalysisConfig && fAnalysisConfig->enableEdep)
    {
      G4cout << " | edep = " << fEdep / keV << " keV";
    }

    if (hasReplayStats)
    {
      G4cout
          << " | replay photons = " << fReplayPhotonCount
          << " | detected photons = " << fDetectedPhotonCount
          << " | detect eff = " << detectEfficiency
          << " | spot RMS = (" << rmsX / um << ", " << rmsY / um << ") um";
    }

    if (fAnalysisConfig &&
        fAnalysisConfig->enableReactionPosition &&
        fHasCapture)
    {
      G4cout
          << " | capture at ("
          << fCaptureX / um << ", "
          << fCaptureY / um << ") um"
          << " | depth = " << fDepth / um << " um";
    }

    G4cout << G4endl;
  }

  // =========================
  // 1) neutron_capture_positions/<thickness>_neutron_capture_positions.csv
  // =========================
  if (fAnalysisConfig &&
      fAnalysisConfig->enableReactionPosition &&
      fHasCapture)
  {
    const G4double corrX = fCaptureX - fSourceX;
    const G4double corrY = fCaptureY - fSourceY;

    if (EnsureCapturePositionCSVForThickness(thicknessLabel))
    {
      gCaptureCSVFile
          << event->GetEventID() << ","
          << thicknessUm << ","
          << fBNWt << ","
          << fZnSWt << ","
          << fCaptureX / um << ","
          << fCaptureY / um << ","
          << corrX / um << ","
          << corrY / um << ","
          << fDepth / um
          << G4endl;
    }
    else
    {
      G4cerr << "Error: neutron capture position CSV is not open at event "
             << event->GetEventID() << G4endl;
    }
  }

  // =========================
  // 2) energy_deposition/<thickness>_energy_deposition_events.csv
  // =========================
  if (fAnalysisConfig &&
      fAnalysisConfig->enableEdep &&
      fHasCapture)
  {
    if (EnsureEdepCSVForThickness(thicknessLabel))
    {
      gEdepCSVFile
          << event->GetEventID() << ","
          << thicknessUm << ","
          << fEdep / keV
          << G4endl;
    }
    else
    {
      G4cerr << "Error: energy deposition CSV is not open at event "
             << event->GetEventID() << G4endl;
    }
  }

  // =========================
  // 3) optical_readout/<thickness>_optical_readout_events.csv
  // =========================
  if (hasReplayStats)
  {
    if (EnsureOpticalCSVForThickness(thicknessLabel))
    {
      gOpticalCSVFile
          << event->GetEventID() << ","
          << thicknessUm << ","
          << fBNWt << ","
          << fZnSWt << ","
          << fReplayPhotonCount << ","
          << fDetectedPhotonCount << ","
          << detectEfficiency << ","
          << meanX / um << ","
          << meanY / um << ","
          << rmsX / um << ","
          << rmsY / um << ","
          << (fHasCapture ? 1 : 0) << ","
          << fSourceX / um << ","
          << fSourceY / um << ","
          << fCaptureX / um << ","
          << fCaptureY / um << ","
          << fDepth / um
          << G4endl;
    }
    else
    {
      G4cerr << "Error: optical readout CSV is not open at event "
             << event->GetEventID() << G4endl;
    }
  }

  // =========================
  // RunAction 统计
  // =========================
  if (fRunAction)
  {
    if (fHasCapture)
    {
      fRunAction->CountCapture();
    }

    if (fHasTransmit)
    {
      fRunAction->CountTransmit();
    }
  }

  // =========================
  // 事件尾清零
  // =========================
  fSourceX = 0.0;
  fSourceY = 0.0;

  fHasCapture = false;

  fBNWt = 0.0;
  fZnSWt = 0.0;
  fCaptureX = 0.0;
  fCaptureY = 0.0;
  fCaptureZ = 0.0;
  fDepth = 0.0;

  fReplayPhotonCount = 0;
}

// =========================
// 累计 α 轨迹长度
// =========================
void EventAction::AddAlphaTrackLen(G4double len)
{
  fAlphaTrackLen += len;
}

// =========================
// 累计 Li7 轨迹长度
// =========================
void EventAction::AddLi7TrackLen(G4double len)
{
  fLi7TrackLen += len;
}

// =========================
// 累计沉积能量
// =========================
void EventAction::AddEdep(G4double edep)
{
  fEdep += edep;
}

// =========================
// 设置源位置
// =========================
void EventAction::SetSourcePosition(G4double x, G4double y)
{
  fSourceX = x;
  fSourceY = y;
}

// =========================
// 设置俘获信息（同一事件只记录第一次）
// =========================
void EventAction::SetCaptureInfo(G4double bnWt,
                                 G4double znsWt,
                                 G4double x,
                                 G4double y,
                                 G4double z,
                                 G4double depth)
{
  if (fHasCapture)
  {
    return;
  }

  fHasCapture = true;
  fBNWt = bnWt;
  fZnSWt = znsWt;
  fCaptureX = x;
  fCaptureY = y;
  fCaptureZ = z;
  fDepth = depth;
}

// =========================
// 第二阶段：设置本事件重放的光子数
// =========================
void EventAction::SetReplayPhotonCount(G4int n)
{
  fReplayPhotonCount = n;
}

// =========================
// 第二阶段：记录一个成功从 Film 背面透射到 World 的光子
// =========================
void EventAction::AddDetectedPhoton(G4double x, G4double y)
{
  ++fDetectedPhotonCount;
  fDetectedSumX += x;
  fDetectedSumY += y;
  fDetectedSumX2 += x * x;
  fDetectedSumY2 += y * y;
}

// =========================
// 是否有俘获事件
// =========================
G4bool EventAction::HasCapture() const
{
  return fHasCapture;
}

// =========================
// 设置穿透信息
// =========================
void EventAction::SetTransmitted()
{
  fHasTransmit = true;
}

// =========================
// 是否穿透
// =========================
G4bool EventAction::HasTransmit() const
{
  return fHasTransmit;
}

// =========================
// 获取 α 粒子轨迹长度
// =========================
G4double EventAction::GetAlphaTrackLen() const
{
  return fAlphaTrackLen;
}

// =========================
// 获取 Li7 粒子轨迹长度
// =========================
G4double EventAction::GetLi7TrackLen() const
{
  return fLi7TrackLen;
}

// =========================
// 获取沉积能量
// =========================
G4double EventAction::GetEdep() const
{
  return fEdep;
}

// =========================
// 获取生成光子数
// =========================
G4double EventAction::GetGeneratedPhotons() const
{
  return fGeneratedPhotons;
}

// =========================
// 获取光子重放数
// =========================
G4double EventAction::GetReplayPhotonCount() const
{
  return fReplayPhotonCount;
}

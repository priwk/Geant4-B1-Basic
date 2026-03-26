#include "EventAction.hh"

#include "RunAction.hh"
#include "AnalysisConfig.hh"
#include "DetectorConstruction.hh"

#include "G4Event.hh"
#include "G4Run.hh"
#include "G4SystemOfUnits.hh"
#include "G4ios.hh"
#include "G4RunManager.hh"

#include <iomanip>

// 静态成员定义
// 反应位置 CSV
std::ofstream EventAction::fCSVFile;
G4bool EventAction::fCSVInitialized = false;

// 能量沉积 CSV
std::ofstream EventAction::fEdepCSVFile;
G4bool EventAction::fEdepCSVInitialized = false;

// 发光源项 CSV
std::ofstream EventAction::fLightCSVFile;
G4bool EventAction::fLightCSVInitialized = false;

// 构造函数
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
      fDepth(0.0)
{
  // 反应位置 CSV
  if (fAnalysisConfig &&
      fAnalysisConfig->enableReactionPosition &&
      !fCSVInitialized)
  {
    fCSVFile.open("capture_events.csv");

    if (fCSVFile.is_open())
    {
      fCSVFile << "runID,eventID,thickness_um,bn_wt,zns_wt,x_um,y_um,corr_x_um,corr_y_um,depth_um"
               << G4endl;
      fCSVInitialized = true;
    }
    else
    {
      G4cerr << "Error: cannot open capture_events.csv" << G4endl;
    }
  }

  // 能量沉积 CSV
  if (fAnalysisConfig &&
      fAnalysisConfig->enableEdep &&
      !fEdepCSVInitialized)
  {
    fEdepCSVFile.open("edep_events.csv");

    if (fEdepCSVFile.is_open())
    {
      fEdepCSVFile << "runID,eventID,thickness_um,edep_keV" << G4endl;
      fEdepCSVInitialized = true;
    }
    else
    {
      G4cerr << "Error: cannot open edep_events.csv" << G4endl;
    }
  }

  // light_events.csv
  if (fAnalysisConfig &&
      fAnalysisConfig->enableLightYield &&
      !fLightCSVInitialized)
  {
    fLightCSVFile.open("light_events.csv");

    if (fLightCSVFile.is_open())
    {
      fLightCSVFile
          << "runID,eventID,thickness_um,"
          << "edep_keV,light_yield_photons,"
          << "has_capture,bn_wt,zns_wt,"
          << "source_x_um,source_y_um,"
          << "capture_x_um,capture_y_um,depth_um"
          << G4endl;

      fLightCSVInitialized = true;
    }
    else
    {
      G4cerr << "Error: cannot open light_events.csv" << G4endl;
    }
  }
}

// 析构函数
EventAction::~EventAction()
{
  if (fCSVFile.is_open())
  {
    fCSVFile.close();
  }

  if (fEdepCSVFile.is_open())
  {
    fEdepCSVFile.close();
  }

  if (fLightCSVFile.is_open())
  {
    fLightCSVFile.close();
  }
}

// 事件开始时重置
void EventAction::BeginOfEventAction(const G4Event *)
{
  fAlphaTrackLen = 0.0;
  fLi7TrackLen = 0.0;
  fEdep = 0.0;
  fGeneratedPhotons = 0.0;

  fHasCapture = false;
  fHasTransmit = false;

  fBNWt = 0.0;
  fZnSWt = 0.0;
  fCaptureX = 0.0;
  fCaptureY = 0.0;
  fCaptureZ = 0.0;
  fDepth = 0.0;

  if (fRunAction)
  {
    fRunAction->CountIncident();
  }
}

// 事件结束时
void EventAction::EndOfEventAction(const G4Event *event)
{
  G4bool doPrint = false;

  // 获取探测器（厚度）信息
  const DetectorConstruction *detector =
      static_cast<const DetectorConstruction *>(
          G4RunManager::GetRunManager()->GetUserDetectorConstruction());

  G4double thicknessUm = -1.0;
  if (detector)
  {
    thicknessUm = detector->GetFilmThickness() / um;
  }

  // 获取run信息
  const G4Run *currentRun = G4RunManager::GetRunManager()->GetCurrentRun();
  G4int runID = -1;
  if (currentRun)
  {
    runID = currentRun->GetRunID();
  }

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

  // 光产额前提是否俘获
  G4bool countLightThisEvent = true;
  if (fAnalysisConfig &&
      fAnalysisConfig->enableLightYield &&
      fAnalysisConfig->lightOnlyForCaptureEvents)
  {
    countLightThisEvent = fHasCapture;
  }

  fGeneratedPhotons = 0.0; // 没开启 enableLightYield 光产额保持为0

  // 计算光产额
  if (fAnalysisConfig &&
      fAnalysisConfig->enableLightYield &&
      countLightThisEvent)
  {
    fGeneratedPhotons =
        (fEdep / MeV) * fAnalysisConfig->lightYieldPhotonsPerMeV;
  }

  if (doPrint)
  {
    G4cout << "Event " << event->GetEventID();

    if (fAnalysisConfig->enableTrackLen)
    {
      G4cout
          << " | alpha track length = " << fAlphaTrackLen / um << " um"
          << " | Li7 track length = " << fLi7TrackLen / um << " um";
    }

    if (fAnalysisConfig->enableEdep)
    {
      G4cout << " | edep = " << fEdep / keV << " keV";
    }

    if (fAnalysisConfig && fAnalysisConfig->enableLightYield)
    {
      G4cout << " | light yield = " << fGeneratedPhotons << " photons";
    }

    if (fAnalysisConfig->enableReactionPosition && fHasCapture)
    {
      G4cout
          << " | capture at ("
          << fCaptureX / um << ", "
          << fCaptureY / um << ") um"
          << " | depth = " << fDepth / um << " um";
    }

    G4cout << G4endl;
  }

  // capture_events.csv
  if (fAnalysisConfig &&
      fAnalysisConfig->enableReactionPosition &&
      fHasCapture)
  {
    G4double corrX = fCaptureX - fSourceX;
    G4double corrY = fCaptureY - fSourceY;

    if (fCSVFile.is_open())
    {
      fCSVFile
          << runID << ","
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
      G4cerr << "Error: capture_events.csv is not open at event "
             << event->GetEventID() << G4endl;
    }
  }

  // edep_events.csv
  if (fAnalysisConfig &&
      fAnalysisConfig->enableEdep &&
      fHasCapture)
  {
    if (fEdepCSVFile.is_open())
    {
      fEdepCSVFile
          << runID << ","
          << event->GetEventID() << ","
          << thicknessUm << ","
          << fEdep / keV
          << G4endl;
    }
    else
    {
      G4cerr << "Error: edep_events.csv is not open at event "
             << event->GetEventID() << G4endl;
    }
  }

  // light_events.csv
  G4bool writeLightRow = false;

  if (fAnalysisConfig && fAnalysisConfig->enableLightYield)
  {
    if (fAnalysisConfig->lightOnlyForCaptureEvents)
    {
      writeLightRow = fHasCapture;
    }
    else
    {
      writeLightRow = true;
    }
  }

  if (writeLightRow)
  {
    if (fLightCSVFile.is_open())
    {
      fLightCSVFile
          << runID << ","
          << event->GetEventID() << ","
          << thicknessUm << ","
          << fEdep / keV << ","
          << fGeneratedPhotons << ","
          << (fHasCapture ? 1 : 0) << ","
          << fBNWt << ","
          << fZnSWt << ","
          << fSourceX / um << ","
          << fSourceY / um << ","
          << fCaptureX / um << ","
          << fCaptureY / um << ","
          << fDepth / um
          << G4endl;
    }
    else
    {
      G4cerr << "Error: light_events.csv is not open at event "
             << event->GetEventID() << G4endl;
    }
  }

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
}

// 添加 α 粒子轨迹长度
void EventAction::AddAlphaTrackLen(G4double len)
{
  fAlphaTrackLen += len;
}

// 添加 Li7 粒子轨迹长度
void EventAction::AddLi7TrackLen(G4double len)
{
  fLi7TrackLen += len;
}

// 添加沉积能量
void EventAction::AddEdep(G4double edep)
{
  fEdep += edep;
}

// 设置源位置
void EventAction::SetSourcePosition(G4double x, G4double y)
{
  fSourceX = x;
  fSourceY = y;
}

// 设置一次俘获信息（同一事件只记录第一次）
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

G4bool EventAction::HasCapture() const
{
  return fHasCapture;
}

void EventAction::SetTransmitted()
{
  fHasTransmit = true;
}

G4bool EventAction::HasTransmit() const
{
  return fHasTransmit;
}

G4double EventAction::GetAlphaTrackLen() const
{
  return fAlphaTrackLen;
}

G4double EventAction::GetLi7TrackLen() const
{
  return fLi7TrackLen;
}

G4double EventAction::GetEdep() const
{
  return fEdep;
}

G4double EventAction::GetGeneratedPhotons() const
{
  return fGeneratedPhotons;
}
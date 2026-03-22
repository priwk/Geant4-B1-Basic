#include "EventAction.hh"

#include "RunAction.hh"
#include "AnalysisConfig.hh"

#include "G4Event.hh"
#include "G4SystemOfUnits.hh"
#include "G4ios.hh"

#include <iomanip>

// 静态成员定义
std::ofstream EventAction::fCSVFile;
G4bool EventAction::fCSVInitialized = false;

// 构造函数
EventAction::EventAction(RunAction *runAction, const AnalysisConfig *config)
    : G4UserEventAction(),
      fRunAction(runAction),
      fAnalysisConfig(config),
      fAlphaTrackLen(0.0),
      fLi7TrackLen(0.0),
      fEdep(0.0),
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
  // 只有启用反应位置记录时才初始化 CSV
  if (fAnalysisConfig &&
      fAnalysisConfig->enableReactionPosition &&
      !fCSVInitialized)
  {
    fCSVFile.open("capture_events.csv");

    if (fCSVFile.is_open())
    {
      fCSVFile << "eventID,bn_wt,zns_wt,x_um,y_um,corr_x_um,corr_y_um,depth_um,alpha_len_um,li7_len_um,edep_keV"
               << G4endl;
      fCSVInitialized = true;
    }
    else
    {
      G4cerr << "Error: cannot open capture_events.csv" << G4endl;
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
}

// 事件开始时重置
void EventAction::BeginOfEventAction(const G4Event *)
{
  fAlphaTrackLen = 0.0;
  fLi7TrackLen = 0.0;
  fEdep = 0.0;

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

  // 只对“发生俘获的事件”写一行
  if (fAnalysisConfig &&
      fAnalysisConfig->enableReactionPosition &&
      fHasCapture)
  {
    G4double corrX = fCaptureX - fSourceX;
    G4double corrY = fCaptureY - fSourceY;

    if (fCSVFile.is_open())
    {
      fCSVFile
          << event->GetEventID() << ","
          << fBNWt << ","
          << fZnSWt << ","
          << fCaptureX / um << ","
          << fCaptureY / um << ","
          << corrX / um << ","
          << corrY / um << ","
          << fDepth / um << ","
          << fAlphaTrackLen / um << ","
          << fLi7TrackLen / um << ","
          << fEdep / keV
          << G4endl;
    }
    else
    {
      G4cerr << "Error: capture_events.csv is not open at event "
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
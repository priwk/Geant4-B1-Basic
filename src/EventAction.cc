#include "EventAction.hh"

#include "G4Event.hh"
#include "G4SystemOfUnits.hh"
#include "G4ios.hh"

#include <iomanip>

// 静态成员定义
std::ofstream EventAction::fCSVFile;
G4bool EventAction::fCSVInitialized = false;

// 构造函数
EventAction::EventAction(RunAction *)
    : G4UserEventAction(),
      fAlphaTrackLen(0.0),
      fLi7TrackLen(0.0),
      fHasCapture(false),
      fBNWt(0.0),
      fZnSWt(0.0),
      fCaptureX(0.0),
      fCaptureY(0.0),
      fCaptureZ(0.0),
      fDepth(0.0)
{
  if (!fCSVInitialized)
  {
    fCSVFile.open("capture_events.csv");

    if (fCSVFile.is_open())
    {
      fCSVFile << "eventID,bn_wt,zns_wt,x_um,y_um,z_um,depth_um" << G4endl;
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
}

// 事件开始时
void EventAction::BeginOfEventAction(const G4Event *)
{
  fAlphaTrackLen = 0.0;
  fLi7TrackLen = 0.0;

  fHasCapture = false;
  fBNWt = 0.0;
  fZnSWt = 0.0;
  fCaptureX = 0.0;
  fCaptureY = 0.0;
  fCaptureZ = 0.0;
  fDepth = 0.0;
}

// 事件结束时
void EventAction::EndOfEventAction(const G4Event *event)
{
  G4cout
      << "Event " << event->GetEventID()
      << " | alpha track length = " << fAlphaTrackLen / mm << " mm"
      << " | Li7 track length = " << fLi7TrackLen / mm << " mm";

  if (fHasCapture)
  {
    G4cout
        << " | capture at ("
        << fCaptureX / um << ", "
        << fCaptureY / um << ", "
        << fCaptureZ / um << ") um"
        << " | depth = " << fDepth / um << " um";

    if (fCSVFile.is_open())
    {
      fCSVFile
          << event->GetEventID() << ","
          << fBNWt << ","
          << fZnSWt << ","
          << fCaptureX / um << ","
          << fCaptureY / um << ","
          << fCaptureZ / um << ","
          << fDepth / um
          << G4endl;
    }
  }

  G4cout << G4endl;
}

// 添加 α 粒子轨迹长度
void EventAction::AddAlphaTrackLen(G4double len)
{
  fAlphaTrackLen += len;
}

// 添加 Li7 轨迹长度
void EventAction::AddLi7TrackLen(G4double len)
{
  fLi7TrackLen += len;
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
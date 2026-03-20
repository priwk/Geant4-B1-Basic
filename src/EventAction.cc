#include "EventAction.hh"
#include "G4Event.hh"
#include "G4SystemOfUnits.hh"
#include "G4ios.hh"

// 构造函数
EventAction::EventAction(RunAction *)
    : G4UserEventAction(),
      fAlphaTrackLen(0.0),
      fLi7TrackLen(0.0)
{
}

// 析构函数
EventAction::~EventAction() {}

// 事件开始时
void EventAction::BeginOfEventAction(const G4Event *)
{
  fAlphaTrackLen = 0.0;
  fLi7TrackLen = 0.0;
}

// 事件结束时
void EventAction::EndOfEventAction(const G4Event *event)
{
  G4cout
      << "Event " << event->GetEventID()
      << " | alpha track length = " << fAlphaTrackLen / mm << " mm"
      << " | Li7 track length = " << fLi7TrackLen / mm << " mm"
      << G4endl;
}

// 添加α粒子轨迹长度
void EventAction::AddAlphaTrackLen(G4double len)
{
  fAlphaTrackLen += len;
}

// 添加Li7轨迹长度
void EventAction::AddLi7TrackLen(G4double len)
{
  fLi7TrackLen += len;
}
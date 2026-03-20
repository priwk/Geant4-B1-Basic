#ifndef EventAction_h
#define EventAction_h 1

#include "G4UserEventAction.hh"
#include "globals.hh"

class RunAction;

class EventAction : public G4UserEventAction
{
public:
  // 构造函数
  EventAction(RunAction *runAction);
  virtual ~EventAction();

  // 事件开始时
  virtual void BeginOfEventAction(const G4Event *event);
  virtual void EndOfEventAction(const G4Event *event);

  // 添加轨迹长度
  void AddAlphaTrackLen(G4double len);
  void AddLi7TrackLen(G4double len);

private:
  // 轨迹长度
  G4double fAlphaTrackLen;
  G4double fLi7TrackLen;
};

#endif

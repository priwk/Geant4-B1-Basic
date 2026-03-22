#ifndef PrimaryGeneratorAction_h
#define PrimaryGeneratorAction_h 1

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "G4GeneralParticleSource.hh"
#include "globals.hh"

class G4Event;
class EventAction;

class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction
{
public:
  PrimaryGeneratorAction(EventAction *eventAction);
  ~PrimaryGeneratorAction();

public:
  // 生成粒子事件
  virtual void GeneratePrimaries(G4Event *);

private:
  // 粒子源
  G4GeneralParticleSource *fParticleGun;

  // 指向 EventAction，用于保存本事件的源位置
  EventAction *fEventAction;
};

#endif
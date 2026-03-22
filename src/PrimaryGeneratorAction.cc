#include "PrimaryGeneratorAction.hh"
#include "EventAction.hh"
#include "G4Event.hh"
#include "G4PrimaryVertex.hh"
#include "G4ParticleTable.hh"
#include "G4IonTable.hh"
#include "G4ParticleDefinition.hh"
#include "G4Geantino.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"

PrimaryGeneratorAction::PrimaryGeneratorAction(EventAction *eventAction)
    : G4VUserPrimaryGeneratorAction(),
      fParticleGun(nullptr),
      fEventAction(eventAction)
{
  // 创建粒子源
  fParticleGun = new G4GeneralParticleSource();
}

PrimaryGeneratorAction::~PrimaryGeneratorAction()
{
  delete fParticleGun;
}

void PrimaryGeneratorAction::GeneratePrimaries(G4Event *anEvent)
{
  // 生成粒子事件
  fParticleGun->GeneratePrimaryVertex(anEvent);

  // 读取源位置，并传给 EventAction
  G4PrimaryVertex *vertex = anEvent->GetPrimaryVertex();
  if (vertex && fEventAction)
  {
    G4double x = vertex->GetX0();
    G4double y = vertex->GetY0();

    fEventAction->SetSourcePosition(x, y);
  }
}
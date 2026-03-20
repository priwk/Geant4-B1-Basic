#include "PrimaryGeneratorAction.hh"
#include "G4Event.hh"
#include "G4ParticleTable.hh"
#include "G4IonTable.hh"
#include "G4ParticleDefinition.hh"
#include "G4Geantino.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"

PrimaryGeneratorAction::PrimaryGeneratorAction()
    : G4VUserPrimaryGeneratorAction(),
      fParticleGun(nullptr)
{
  fParticleGun = new G4GeneralParticleSource();

  /*
  G4int n_particle = 1;
  fParticleGun = new G4ParticleGun(n_particle);

  G4ParticleTable *particleTable = G4ParticleTable::GetParticleTable();
  G4ParticleDefinition *particle = particleTable->FindParticle("neutron");
  fParticleGun->SetParticleDefinition(particle);

  // 热中子能量
  fParticleGun->SetParticleEnergy(0.0253 * eV);

  // 设置粒子动量方向
  fParticleGun->SetParticleMomentumDirection(G4ThreeVector(0., 0., -1.));
  */
}

PrimaryGeneratorAction::~PrimaryGeneratorAction()
{
  delete fParticleGun;
}

void PrimaryGeneratorAction::GeneratePrimaries(G4Event *anEvent)
{
  /*
  // 设置粒子位置
  fParticleGun->SetParticlePosition(G4ThreeVector(0, 0, 4 * cm));
  */

  // 生成粒子事件
  fParticleGun->GeneratePrimaryVertex(anEvent);
  
}

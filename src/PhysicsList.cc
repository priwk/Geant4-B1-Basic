#include "PhysicsList.hh"
#include "DetectorConstruction.hh"
#include "EffectiveSigmaCapturePhysics.hh"

#include "G4ParticleDefinition.hh"
#include "G4ProcessManager.hh"
#include "G4ParticleTypes.hh"
#include "G4ParticleTable.hh"
#include "G4EmStandardPhysics.hh"
#include "G4DecayPhysics.hh"
#include "G4RadioactiveDecayPhysics.hh"
#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"

#include "G4MesonConstructor.hh"
#include "G4Proton.hh"
#include "G4AntiProton.hh"
#include "G4Neutron.hh"
#include "G4AntiNeutron.hh"
#include "G4IonConstructor.hh"
#include "G4ShortLivedConstructor.hh"

#include "G4HadronElasticPhysicsHP.hh"
#include "G4HadronPhysicsQGSP_BIC_HP.hh"
#include "G4OpticalPhysics.hh"
#include "G4ThermalNeutrons.hh"
#include "G4IonPhysicsPHP.hh"

PhysicsList::PhysicsList(DetectorConstruction *detector)
    : G4VModularPhysicsList(),
      fDetector(detector)
{
  SetVerboseLevel(1);

  // 电磁
  RegisterPhysics(new G4EmStandardPhysics());

  // 衰变
  RegisterPhysics(new G4DecayPhysics);
  RegisterPhysics(new G4RadioactiveDecayPhysics);

  // 热中子 / 低能中子 hadronic
  RegisterPhysics(new G4HadronElasticPhysicsHP());
  RegisterPhysics(new G4HadronPhysicsQGSP_BIC_HP());

  // 开启热中子晶格散射模型，以及高精度离子（7Li, Alpha）物理过程
  RegisterPhysics(new G4ThermalNeutrons(0));
  RegisterPhysics(new G4IonPhysicsPHP());

  // 光学：厚度扫描 / source generation 阶段先不要开
  // RegisterPhysics(new G4OpticalPhysics());

  // 自定义：用等效宏观吸收系数替代 Film 中的默认 neutron capture
  RegisterPhysics(new EffectiveSigmaCapturePhysics(fDetector));
}

PhysicsList::~PhysicsList()
{
}

void PhysicsList::SetCuts()
{
  G4VUserPhysicsList::SetCuts();
}
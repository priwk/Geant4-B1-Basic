#include "PhysicsList.hh"

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

#include "G4OpticalPhysics.hh"

PhysicsList::PhysicsList() : G4VModularPhysicsList()
{
  SetVerboseLevel(1);

  RegisterPhysics(new G4EmStandardPhysics());     // 电磁过程
  RegisterPhysics(new G4DecayPhysics);            // 衰变物理过程
  RegisterPhysics(new G4RadioactiveDecayPhysics); // 放射性衰变物理过程
  RegisterPhysics(new G4OpticalPhysics());        // 光学过程
}

PhysicsList::~PhysicsList()
{
}

void PhysicsList::SetCuts()
{
  G4VUserPhysicsList::SetCuts();
}

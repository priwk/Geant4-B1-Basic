#ifndef PhysicsList_h
#define PhysicsList_h 1

#include "G4VUserPhysicsList.hh"
#include "globals.hh"
#include "G4VModularPhysicsList.hh"

class DetectorConstruction;

class PhysicsList : public G4VModularPhysicsList
{
public:
  PhysicsList(DetectorConstruction *detector);
  ~PhysicsList();

protected:
  virtual void SetCuts();

private:
  DetectorConstruction *fDetector;
};

#endif
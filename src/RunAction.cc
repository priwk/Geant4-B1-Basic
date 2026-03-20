#include "RunAction.hh"
#include "G4ThreeVector.hh"
#include "DetectorConstruction.hh"

#include "G4Run.hh"
#include "G4ios.hh"
#include "G4RunManager.hh"
#include "G4UnitsTable.hh"
#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"
#include "G4LogicalVolume.hh"
#include "G4PhysicalVolumeStore.hh"

RunAction::RunAction()
{
}

RunAction::~RunAction()
{
}

void RunAction::BeginOfRunAction(const G4Run *aRun)
{
}

void RunAction::EndOfRunAction(const G4Run *aRun)
{
    G4cout << "### Run " << aRun->GetRunID() << " Stop." << G4endl;
}

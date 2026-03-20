/*
#include "SteppingAction.hh"
#include "EventAction.hh"
#include "DetectorConstruction.hh"

#include "G4Step.hh"
#include "G4Event.hh"
#include "G4RunManager.hh"
#include "G4LogicalVolume.hh"
#include "G4SystemOfUnits.hh"

#include <fstream>
#include <iostream>
#include <string>
using namespace std;

SteppingAction::SteppingAction(EventAction *eventAction)
    : G4UserSteppingAction(),
      fEventAction(eventAction)
{
}

SteppingAction::~SteppingAction()
{
}

//________________________________________________
//
void SteppingAction::UserSteppingAction(const G4Step *step)
{

    G4int copyno = step->GetPreStepPoint()->GetTouchableHandle()->GetVolume()->GetCopyNo();
    G4double energy = step->GetTotalEnergyDeposit() / MeV;
    G4int evtID = G4EventManager::GetEventManager()->GetConstCurrentEvent()->GetEventID();
    // G4int PID1 = step->GetTrack()->GetParentID();
    // G4int PID2 = step->GetTrack()->GetParentID(0);

    if (copyno != 0)
    {
        // fEventAction->AddEdep(energy)
    }
}
*/

#include "SteppingAction.hh"
#include "EventAction.hh"

#include "G4Step.hh"
#include "G4Track.hh"
#include "G4StepPoint.hh"
#include "G4VPhysicalVolume.hh"

// 构造函数
SteppingAction::SteppingAction(EventAction *eventAction)
    : G4UserSteppingAction(),
      fEventAction(eventAction)
{
}

// 析构函数
SteppingAction::~SteppingAction() {}

// 用户步进动作
void SteppingAction::UserSteppingAction(const G4Step *step)
{
    G4Track *track = step->GetTrack();
    G4String particleName = track->GetDefinition()->GetParticleName();
    G4double stepLength = step->GetStepLength();

    // 只统计第一代次级粒子（由主中子直接产生）
    if (track->GetParentID() != 1)
        return;

    // 只统计 Film 内的步长
    auto prePoint = step->GetPreStepPoint();
    if (!prePoint)
        return;

    auto volume = prePoint->GetPhysicalVolume();
    if (!volume)
        return;

    G4String volumeName = volume->GetName();
    if (volumeName != "Film")
        return;

    // 累加轨迹长度
    if (particleName == "alpha")
    {
        fEventAction->AddAlphaTrackLen(stepLength);
    }
    else if (particleName == "Li7")
    {
        fEventAction->AddLi7TrackLen(stepLength);
    }
}
#ifndef EventAction_h
#define EventAction_h 1

#include "G4UserEventAction.hh"
#include "globals.hh"
#include <fstream>

class RunAction;

class EventAction : public G4UserEventAction
{
public:
  EventAction(RunAction *runAction);
  virtual ~EventAction();

  virtual void BeginOfEventAction(const G4Event *event);
  virtual void EndOfEventAction(const G4Event *event);

  void AddAlphaTrackLen(G4double len);
  void AddLi7TrackLen(G4double len);

  void SetCaptureInfo(G4double bnWt,
                      G4double znsWt,
                      G4double x,
                      G4double y,
                      G4double z,
                      G4double depth);

  G4bool HasCapture() const;

private:
  G4double fAlphaTrackLen;
  G4double fLi7TrackLen;

  G4bool fHasCapture;
  G4double fBNWt;
  G4double fZnSWt;
  G4double fCaptureX;
  G4double fCaptureY;
  G4double fCaptureZ;
  G4double fDepth;

  static std::ofstream fCSVFile;
  static G4bool fCSVInitialized;
};

#endif
#ifndef DetectorConstruction_h
#define DetectorConstruction_h 1

#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"
#include "G4Cache.hh"

class G4VPhysicalVolume;
class G4LogicalVolume;
class G4Material;

class DetectorConstruction : public G4VUserDetectorConstruction
{
public:
  DetectorConstruction();
  virtual ~DetectorConstruction();

  virtual G4VPhysicalVolume *Construct();

  G4LogicalVolume *GetScoringVolume() const { return fScoringVolume; }

  G4double GetFilmThickness() const { return fFilmThickness; }
  G4double GetFilmCenterZ() const { return fFilmCenterZ; }
  G4double GetFilmFrontZ() const { return fFilmFrontZ; }

  G4double GetBNWt() const { return fBNWt; }
  G4double GetZnSWt() const { return fZnSWt; }

protected:
  G4LogicalVolume *fScoringVolume;

  G4double fFilmThickness;
  G4double fFilmCenterZ;
  G4double fFilmFrontZ;

  G4double fBNWt;
  G4double fZnSWt;
};

#endif
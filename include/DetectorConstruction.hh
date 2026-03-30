#ifndef DetectorConstruction_h
#define DetectorConstruction_h 1

#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"
#include "G4Cache.hh"

class G4VPhysicalVolume;
class G4LogicalVolume;
class G4Material;
class G4GenericMessenger;

class DetectorConstruction : public G4VUserDetectorConstruction
{
public:
  DetectorConstruction();
  virtual ~DetectorConstruction();

  virtual G4VPhysicalVolume *Construct();

  void DefineMaterials();

  G4LogicalVolume *GetScoringVolume() const { return fScoringVolume; }

  G4Material *fVacuum;
  G4Material *fMat10BN;
  G4Material *fMatZnS;
  G4Material *fMatZnSAg;
  G4Material *fMatBN_ZnS;

  G4double GetFilmThickness() const { return fFilmThickness; }
  G4double GetFilmCenterZ() const { return fFilmCenterZ; }
  G4double GetFilmFrontZ() const { return fFilmFrontZ; }
  G4double GetFilmBackZ() const { return fFilmBackZ; }

  void SetFilmThickness(G4double t) { fFilmThicknessInput = t; }

  G4double GetBNWt() const { return fBNWt; }
  G4double GetZnSWt() const { return fZnSWt; }

protected:
  G4GenericMessenger *fMessenger;

  G4LogicalVolume *fScoringVolume;

  G4double fFilmThickness;
  G4double fFilmCenterZ;
  G4double fFilmFrontZ;
  G4double fFilmBackZ;

  G4double fFilmThicknessInput;

  G4double fBNWt;
  G4double fZnSWt;
};

#endif
#include "DetectorConstruction.hh"

#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4VisAttributes.hh"
#include "G4Colour.hh"
#include "G4Material.hh"
#include "G4Element.hh"
#include "G4Isotope.hh"

DetectorConstruction::DetectorConstruction()
    : G4VUserDetectorConstruction(),
      fScoringVolume(nullptr),
      fFilmThickness(0.0),
      fFilmCenterZ(0.0),
      fFilmFrontZ(0.0),
      fBNWt(0.0),
      fZnSWt(0.0)
{
}

DetectorConstruction::~DetectorConstruction()
{
}

G4VPhysicalVolume *DetectorConstruction::Construct()
{
    G4NistManager *nist = G4NistManager::Instance();

    // =========================
    // 1. 定义真空世界
    // =========================
    static constexpr G4double universe_mean_density = 1.e-25 * g / cm3;
    G4double pressure = 3.e-18 * pascal;
    G4double temperature = 2.73 * kelvin;

    G4Material *Vacuum = new G4Material(
        "Galactic",
        1.,
        1.008 * g / mole,
        universe_mean_density,
        kStateGas,
        temperature,
        pressure);

    G4double worldXY = 6 * cm;
    G4double worldZ = 8 * cm;

    G4Box *solidWorld = new G4Box("World", 0.5 * worldXY, 0.5 * worldXY, 0.5 * worldZ);
    G4LogicalVolume *logicWorld = new G4LogicalVolume(solidWorld, Vacuum, "World");
    G4VPhysicalVolume *physWorld = new G4PVPlacement(
        nullptr,
        G4ThreeVector(),
        logicWorld,
        "World",
        nullptr,
        false,
        0);

    // =========================
    // 2. 定义 10B 富集元素
    // =========================
    G4Isotope *B10 = new G4Isotope("B10", 5, 10, 10.0129370 * g / mole);

    G4Element *elB10 = new G4Element("EnrichedBoron10", "B10", 1);
    elB10->AddIsotope(B10, 100. * perCent);

    G4Element *elN = nist->FindOrBuildElement("N");
    G4Element *elZn = nist->FindOrBuildElement("Zn");
    G4Element *elS = nist->FindOrBuildElement("S");

    // =========================
    // 3. 定义 10BN 和 ZnS
    // =========================
    G4double densityBN = 2.1 * g / cm3;
    G4Material *mat10BN = new G4Material("B10N", densityBN, 2);
    mat10BN->AddElement(elB10, 1);
    mat10BN->AddElement(elN, 1);

    G4double densityZnS = 4.09 * g / cm3;
    G4Material *matZnS = new G4Material("ZnS", densityZnS, 2);
    matZnS->AddElement(elZn, 1);
    matZnS->AddElement(elS, 1);

    // =========================
    // 4. 定义混合物：10BN + ZnS
    // =========================
    fBNWt = 50.0;
    fZnSWt = 50.0;

    G4double densityMix = 3.0 * g / cm3;
    G4Material *matBN_ZnS = new G4Material("B10BN_ZnS_Mix", densityMix, 2);
    matBN_ZnS->AddMaterial(mat10BN, fBNWt * perCent);
    matBN_ZnS->AddMaterial(matZnS, fZnSWt * perCent);

    // =========================
    // 5. 定义薄膜（Film）
    // =========================
    G4double filmXY = 5 * cm;
    G4double filmT = 1 * cm;

    G4ThreeVector filmPos = G4ThreeVector(0, 0, 0);

    G4Box *solidFilm = new G4Box("Film", 0.5 * filmXY, 0.5 * filmXY, 0.5 * filmT);
    G4LogicalVolume *logicFilm = new G4LogicalVolume(solidFilm, matBN_ZnS, "Film");

    new G4PVPlacement(
        nullptr,
        filmPos,
        logicFilm,
        "Film",
        logicWorld,
        false,
        0);

    // 保存后续统计所需信息
    fScoringVolume = logicFilm;
    fFilmThickness = filmT;
    fFilmCenterZ = filmPos.z();
    fFilmFrontZ = fFilmCenterZ + 0.5 * fFilmThickness;

    // =========================
    // 6. 可视化属性
    // =========================
    logicWorld->SetVisAttributes(G4VisAttributes::GetInvisible());

    G4VisAttributes *filmVisAtt = new G4VisAttributes(G4Colour(1.0, 1.0, 0.0, 0.5));
    filmVisAtt->SetForceSolid(true);
    logicFilm->SetVisAttributes(filmVisAtt);

    return physWorld;
}
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

#include "G4GenericMessenger.hh"

#include "G4MaterialPropertiesTable.hh"
#include "G4OpticalParameters.hh"

DetectorConstruction::DetectorConstruction()
    : G4VUserDetectorConstruction(),
      fMessenger(nullptr),
      fScoringVolume(nullptr),
      fVacuum(nullptr),
      fMat10BN(nullptr),
      fMatZnS(nullptr),
      fMatZnSAg(nullptr),
      fMatBN_ZnS(nullptr),
      fFilmThickness(0.0),
      fFilmCenterZ(0.0),
      fFilmFrontZ(0.0),
      fFilmBackZ(0.0),
      fFilmThicknessInput(2000 * um),
      fBNWt(0.0),
      fZnSWt(0.0)
{
    // 创建命令解析器
    fMessenger = new G4GenericMessenger(this, "/det/", "Detector control");

    auto &thickCmd =
        fMessenger->DeclareMethodWithUnit("setThickness", "um",
                                          &DetectorConstruction::SetFilmThickness,
                                          "Set film thickness.");
    thickCmd.SetParameterName("thickness", false);
    thickCmd.SetRange("thickness>0.");
    thickCmd.SetStates(G4State_PreInit, G4State_Idle);
}

// 析构函数
DetectorConstruction::~DetectorConstruction()
{
    delete fMessenger;
}

void DetectorConstruction::DefineMaterials()
{
    if (fVacuum && fMat10BN && fMatZnS && fMatZnSAg && fMatBN_ZnS)
    {
        return;
    }

    G4NistManager *nist = G4NistManager::Instance();

    // 1. 真空世界材料
    static constexpr G4double universe_mean_density = 1.e-25 * g / cm3;
    const G4double pressure = 3.e-18 * pascal;
    const G4double temperature = 2.73 * kelvin;

    if (!fVacuum)
    {
        fVacuum = new G4Material(
            "Galactic",
            1.,
            1.008 * g / mole,
            universe_mean_density,
            kStateGas,
            temperature,
            pressure);
    }

    // 2. 定义 10B 富集元素
    G4Isotope *B10 = new G4Isotope("B10", 5, 10, 10.0129370 * g / mole);
    G4Isotope *B11 = new G4Isotope("B11", 5, 11, 11.009305 * g / mole);

    G4Element *elB_enriched = new G4Element("EnrichedBoron", "B", 2);
    elB_enriched->AddIsotope(B10, 99.17 * perCent);
    elB_enriched->AddIsotope(B11, 0.83 * perCent);

    G4Element *elN = nist->FindOrBuildElement("N");
    G4Element *elZn = nist->FindOrBuildElement("Zn");
    G4Element *elS = nist->FindOrBuildElement("S");
    G4Element *elAg = nist->FindOrBuildElement("Ag");

    // 3. 定义 10BN、ZnS 和 ZnS:Ag
    if (!fMat10BN)
    {
        const G4double densityBN = 2.1 * g / cm3;
        fMat10BN = new G4Material("B10N", densityBN, 2);
        fMat10BN->AddElement(elB_enriched, 1);
        fMat10BN->AddElement(elN, 1);
    }

    if (!fMatZnS)
    {
        const G4double densityZnS = 4.09 * g / cm3;
        fMatZnS = new G4Material("ZnS", densityZnS, 2);
        fMatZnS->AddElement(elZn, 1);
        fMatZnS->AddElement(elS, 1);
    }

    if (!fMatZnSAg)
    {
        const G4double densityZnSAg = 4.0925 * g / cm3;
        fMatZnSAg = new G4Material("ZnS_Ag", densityZnSAg, 2);
        fMatZnSAg->AddMaterial(fMatZnS, 99.9 * perCent);
        fMatZnSAg->AddElement(elAg, 0.1 * perCent);
    }

    // 4. 定义混合物：10BN + ZnS:Ag
    fBNWt = 50.0;
    fZnSWt = 50.0;

    if (!fMatBN_ZnS)
    {
        const G4double densityMix = 2.94 * g / cm3;
        fMatBN_ZnS = new G4Material("B10BN_ZnS_Mix", densityMix, 2);
        fMatBN_ZnS->AddMaterial(fMat10BN, fBNWt * perCent);
        fMatBN_ZnS->AddMaterial(fMatZnSAg, fZnSWt * perCent);
    }

    // 5. 最简光学属性：World 和 Film
    const G4int nEntries = 2;
    G4double photonEnergy[nEntries] = {2.5 * eV, 3.2 * eV};

    // World / Vacuum
    G4double rindexVacuum[nEntries] = {1.0, 1.0};
    auto *mptVacuum = new G4MaterialPropertiesTable();
    mptVacuum->AddProperty("RINDEX", photonEnergy, rindexVacuum, nEntries);
    fVacuum->SetMaterialPropertiesTable(mptVacuum);

    // Film / B10BN_ZnS_Mix
    G4double rindexFilm[nEntries] = {1.98, 1.98};
    G4double absFilm[nEntries] = {1.5 * mm, 1.5 * mm};

    auto *mptFilm = new G4MaterialPropertiesTable();
    mptFilm->AddProperty("RINDEX", photonEnergy, rindexFilm, nEntries);
    mptFilm->AddProperty("ABSLENGTH", photonEnergy, absFilm, nEntries);
    fMatBN_ZnS->SetMaterialPropertiesTable(mptFilm);
}

// 构建函数
G4VPhysicalVolume *DetectorConstruction::Construct()
{

    DefineMaterials(); // 材料已定义

    G4double worldXY = 6 * cm;
    G4double worldZ = 8 * cm;

    G4Box *solidWorld = new G4Box("World", 0.5 * worldXY, 0.5 * worldXY, 0.5 * worldZ);
    G4LogicalVolume *logicWorld = new G4LogicalVolume(solidWorld, fVacuum, "World");
    G4VPhysicalVolume *physWorld = new G4PVPlacement(
        nullptr,
        G4ThreeVector(),
        logicWorld,
        "World",
        nullptr,
        false,
        0);

    // =========================
    // 定义薄膜（Film）
    // =========================
    G4double filmXY = 5 * cm;
    G4double filmT = fFilmThicknessInput;

    G4ThreeVector filmPos = G4ThreeVector(0, 0, 0);

    G4Box *solidFilm = new G4Box("Film", 0.5 * filmXY, 0.5 * filmXY, 0.5 * filmT);
    G4LogicalVolume *logicFilm = new G4LogicalVolume(solidFilm, fMatBN_ZnS, "Film");

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
    fFilmFrontZ = fFilmCenterZ + 0.5 * fFilmThickness; // +z 面，入射面
    fFilmBackZ = fFilmCenterZ - 0.5 * fFilmThickness;  // -z 面，出射面

    // =========================
    // 6. 可视化属性
    // =========================
    logicWorld->SetVisAttributes(G4VisAttributes::GetInvisible());

    G4VisAttributes *filmVisAtt = new G4VisAttributes(G4Colour(0.9, 0.9, 0.3, 0.4));
    filmVisAtt->SetForceSolid(true);
    logicFilm->SetVisAttributes(filmVisAtt);

    return physWorld;
}
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

DetectorConstruction::DetectorConstruction()
    : G4VUserDetectorConstruction(),
      fMessenger(nullptr),
      fScoringVolume(nullptr),
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

// 构建函数
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
    G4Isotope *B11 = new G4Isotope("B11", 5, 11, 11.009305 * g / mole);

    G4Element *elB_enriched = new G4Element("EnrichedBoron", "B", 2);
    elB_enriched->AddIsotope(B10, 99.17 * perCent);
    elB_enriched->AddIsotope(B11, 0.83 * perCent);

    G4Element *elN = nist->FindOrBuildElement("N");
    G4Element *elZn = nist->FindOrBuildElement("Zn");
    G4Element *elS = nist->FindOrBuildElement("S");

    // =========================
    // 3. 定义 10BN、ZnS 和 ZnS:Ag
    // =========================
    G4double densityBN = 2.1 * g / cm3;
    G4Material *mat10BN = new G4Material("B10N", densityBN, 2);
    mat10BN->AddElement(elB_enriched, 1);
    mat10BN->AddElement(elN, 1);

    G4double densityZnS = 4.09 * g / cm3;
    G4Material *matZnS = new G4Material("ZnS", densityZnS, 2);
    matZnS->AddElement(elZn, 1);
    matZnS->AddElement(elS, 1);

    // Ag 元素
    G4Element *elAg = nist->FindOrBuildElement("Ag");

    // ZnS:Ag，按 ZnS 相内掺银 0.1 wt%
    G4double densityZnSAg = 4.0925 * g / cm3; // 估算值，也可直接继续用 4.09
    G4Material *matZnSAg = new G4Material("ZnS_Ag", densityZnSAg, 2);
    matZnSAg->AddMaterial(matZnS, 99.9 * perCent);
    matZnSAg->AddElement(elAg, 0.1 * perCent);

    // =========================
    // 4. 定义混合物：10BN + ZnS:Ag
    // =========================
    fBNWt = 50.0;
    fZnSWt = 50.0;

    G4double densityMix = 2.94 * g / cm3; // 按你当前 BN=2.1、ZnS:Ag 估算后的更合理值
    G4Material *matBN_ZnS = new G4Material("B10BN_ZnS_Mix", densityMix, 2);
    matBN_ZnS->AddMaterial(mat10BN, fBNWt * perCent);
    matBN_ZnS->AddMaterial(matZnSAg, fZnSWt * perCent);

    // =========================
    // 5. 定义薄膜（Film）
    // =========================
    G4double filmXY = 5 * cm;
    G4double filmT = fFilmThicknessInput;

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
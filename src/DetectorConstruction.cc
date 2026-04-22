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

#include <algorithm>
#include <cmath>
#include <string>

namespace
{
    // ============================================================
    // 便捷填写区（也可在宏里改）
    // ------------------------------------------------------------
    // 1) BN : ZnS(Ag) 只写“份数”，程序会自动归一化
    //    例如：
    //      1 : 2   -> fBNWt = 1.0, fZnSWt = 2.0
    //      1 : 1.5 -> fBNWt = 1.0, fZnSWt = 1.5
    //      2 : 1   -> fBNWt = 2.0, fZnSWt = 1.0
    //
    // 2) solid / binder / air 也写“份数”或“质量百分比”都可以，
    //    程序会自动按三者总和归一化。
    // ============================================================
    G4double gSolidPart = 64.0;  // BN + ZnS(Ag) 总固相
    G4double gBinderPart = 14.4; // binder
    G4double gAirPart = 21.6;    // air / void -0.60

    inline G4double SafePositive(G4double x, G4double fallback)
    {
        return (x > 0.0) ? x : fallback;
    }
}

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
      fBNWt(1.0), // BN 份数（不是百分比）
      fZnSWt(2.0) // ZnS(Ag) 份数（不是百分比）
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

    // 直接在宏里改 BN:ZnS(Ag) 份数，更方便
    auto &bnPartCmd = fMessenger->DeclareProperty(
        "bnPart", fBNWt,
        "BN part in BN:ZnS(Ag). Example: /det/bnPart 1");
    bnPartCmd.SetParameterName("bnPart", false);
    bnPartCmd.SetRange("bnPart>0.");

    auto &znsPartCmd = fMessenger->DeclareProperty(
        "znsPart", fZnSWt,
        "ZnS(Ag) part in BN:ZnS(Ag). Example: /det/znsPart 2");
    znsPartCmd.SetParameterName("znsPart", false);
    znsPartCmd.SetRange("znsPart>0.");

    auto &solidCmd = fMessenger->DeclareProperty(
        "solidPart", gSolidPart,
        "Total solid part = BN + ZnS(Ag) in the homogeneous film.");
    solidCmd.SetParameterName("solidPart", false);
    solidCmd.SetRange("solidPart>=0.");

    auto &binderCmd = fMessenger->DeclareProperty(
        "binderPart", gBinderPart,
        "Binder part in the homogeneous film.");
    binderCmd.SetParameterName("binderPart", false);
    binderCmd.SetRange("binderPart>=0.");

    auto &airCmd = fMessenger->DeclareProperty(
        "airPart", gAirPart,
        "Air/void part in the homogeneous film.");
    airCmd.SetParameterName("airPart", false);
    airCmd.SetRange("airPart>=0.");
}

// 析构函数
DetectorConstruction::~DetectorConstruction()
{
    delete fMessenger;
}

void DetectorConstruction::DefineMaterials()
{
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
    G4Isotope *B10 = G4Isotope::GetIsotope("B10", false);
    if (!B10)
    {
        B10 = new G4Isotope("B10", 5, 10, 10.0129370 * g / mole);
    }

    G4Isotope *B11 = G4Isotope::GetIsotope("B11", false);
    if (!B11)
    {
        B11 = new G4Isotope("B11", 5, 11, 11.009305 * g / mole);
    }

    G4Element *elB_enriched = G4Element::GetElement("EnrichedBoron", false);
    if (!elB_enriched)
    {
        elB_enriched = new G4Element("EnrichedBoron", "B", 2); // B丰度修改
        elB_enriched->AddIsotope(B10, 19.78 * perCent);
        elB_enriched->AddIsotope(B11, 80.22 * perCent);
    }

    G4Element *elN = nist->FindOrBuildElement("N");
    G4Element *elZn = nist->FindOrBuildElement("Zn");
    G4Element *elS = nist->FindOrBuildElement("S");
    G4Element *elAg = nist->FindOrBuildElement("Ag");

    // 3. 定义 10BN、ZnS 和 ZnS:Ag
    if (!fMat10BN)
    {
        const G4double densityBN = 2.10 * g / cm3;
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
        const G4double densityZnSAg = 4.09 * g / cm3;
        fMatZnSAg = new G4Material("ZnS_Ag", densityZnSAg, 2);
        fMatZnSAg->AddMaterial(fMatZnS, 0.999); // 99.9 wt%
        fMatZnSAg->AddElement(elAg, 0.001);     // 0.1 wt%
    }

    // 4. 其他均相组分：binder + air
    //    binder 这里先用PMMA近似；如果你有更合适的粘结剂，可直接换这里。
    G4Material *matBinder = nist->FindOrBuildMaterial("G4_PLEXIGLASS");
    // G4Material *matAir = nist->FindOrBuildMaterial("G4_AIR");

    // ------------------------------------------------------------
    // 最终薄膜 = 实体骨架(BN + ZnS(Ag) + binder) + 孔隙(void)
    // 注意：airPart 不再作为“质量组分”，而是孔隙体积分数的份数
    // ------------------------------------------------------------
    const G4double bnPart = SafePositive(fBNWt, 1.0);
    const G4double znsPart = SafePositive(fZnSWt, 1.0);

    const G4double solidPart = std::max(0.0, gSolidPart);
    const G4double binderPart = std::max(0.0, gBinderPart);
    const G4double airPart = std::max(0.0, gAirPart);

    // 总份数：solid + binder + air(void)
    const G4double totalPart = solidPart + binderPart + airPart;
    const G4double safeTotal = (totalPart > 0.0) ? totalPart : 1.0;

    // 孔隙体积分数
    const G4double phiVoid = airPart / safeTotal;

    // 非孔隙骨架 = solid + binder
    const G4double skeletonPart = solidPart + binderPart;
    const G4double safeSkeletonPart = (skeletonPart > 0.0) ? skeletonPart : 1.0;

    // 在骨架内部，solid 与 binder 的质量分数
    const G4double solidMassFracInSkeleton = solidPart / safeSkeletonPart;
    const G4double binderMassFracInSkeleton = binderPart / safeSkeletonPart;

    // 在 solid 内部，BN 与 ZnS(Ag) 的质量分数
    const G4double totalSolidInnerPart = bnPart + znsPart;
    const G4double safeSolidInnerPart = (totalSolidInnerPart > 0.0) ? totalSolidInnerPart : 1.0;

    const G4double bnFracInSolid = bnPart / safeSolidInnerPart;
    const G4double znsFracInSolid = znsPart / safeSolidInnerPart;

    // 骨架内部最终三组分质量分数
    const G4double wBN_skeleton = solidMassFracInSkeleton * bnFracInSolid;
    const G4double wZnSAg_skeleton = solidMassFracInSkeleton * znsFracInSolid;
    const G4double wBinder_skeleton = binderMassFracInSkeleton;

    // 三组分密度
    const G4double rhoBN = fMat10BN->GetDensity();
    const G4double rhoZnSAg = fMatZnSAg->GetDensity();
    const G4double rhoBinder = matBinder->GetDensity();

    // 先算“无孔隙骨架”的密度
    const G4double denomSkeleton =
        (wBN_skeleton / rhoBN) +
        (wZnSAg_skeleton / rhoZnSAg) +
        (wBinder_skeleton / rhoBinder);

    const G4double rhoSkeleton =
        (denomSkeleton > 0.0) ? (1.0 / denomSkeleton) : (1.0 * g / cm3);

    // 再乘以 (1 - 孔隙率)，得到最终有效密度
    const G4double densityMix = (1.0 - phiVoid) * rhoSkeleton;

    // 每次重建几何时，都新建一个最终混合材料，避免沿用旧比例
    static int mixBuildIndex = 0;
    const std::string mixName = "B10BN_ZnSAg_Binder_Air_HomogeneousMix_" +
                                std::to_string(++mixBuildIndex);

    fMatBN_ZnS = new G4Material(mixName, densityMix, 3);
    fMatBN_ZnS->AddMaterial(fMat10BN, wBN_skeleton);
    fMatBN_ZnS->AddMaterial(fMatZnSAg, wZnSAg_skeleton);
    fMatBN_ZnS->AddMaterial(matBinder, wBinder_skeleton);

    // 5. 最简光学属性：World 和 Film
    const G4int nEntries = 2;
    G4double photonEnergy[nEntries] = {2.5 * eV, 3.2 * eV};

    // World / Vacuum
    if (!fVacuum->GetMaterialPropertiesTable())
    {
        G4double rindexVacuum[nEntries] = {1.0, 1.0};
        auto *mptVacuum = new G4MaterialPropertiesTable();
        mptVacuum->AddProperty("RINDEX", photonEnergy, rindexVacuum, nEntries);
        fVacuum->SetMaterialPropertiesTable(mptVacuum);
    }

    // Film / homogeneous mixture
    G4double rindexFilm[nEntries] = {2.18, 2.18};
    G4double absFilm[nEntries] = {12.0 * um, 12.0 * um};

    auto *mptFilm = new G4MaterialPropertiesTable();
    mptFilm->AddProperty("RINDEX", photonEnergy, rindexFilm, nEntries);
    mptFilm->AddProperty("ABSLENGTH", photonEnergy, absFilm, nEntries);
    fMatBN_ZnS->SetMaterialPropertiesTable(mptFilm);

    // 打印当前均相混合配比，便于核查
    G4cout << "\n[DetectorConstruction] Homogeneous film material rebuilt:" << G4endl;
    G4cout << "  BN : ZnS(Ag) parts        = " << bnPart << " : " << znsPart << G4endl;
    G4cout << "  [solid | binder | air]   = "
           << solidPart << " | " << binderPart << " | " << airPart << G4endl;
    G4cout << "\n[DetectorConstruction] Homogeneous film material rebuilt:" << G4endl;
    G4cout << "  BN : ZnS(Ag) parts        = " << bnPart << " : " << znsPart << G4endl;
    G4cout << "  [solid | binder | air]   = "
           << solidPart << " | " << binderPart << " | " << airPart << G4endl;
    G4cout << "  Skeleton mass fractions  = "
           << "BN=" << 100.0 * wBN_skeleton << " wt%, "
           << "ZnS(Ag)=" << 100.0 * wZnSAg_skeleton << " wt%, "
           << "binder=" << 100.0 * wBinder_skeleton << " wt%" << G4endl;
    G4cout << "  Void volume fraction     = " << 100.0 * phiVoid << " vol%" << G4endl;
    G4cout << "  Skeleton density         = " << rhoSkeleton / (g / cm3) << " g/cm3" << G4endl;
    G4cout << "  Effective density        = " << densityMix / (g / cm3) << " g/cm3" << G4endl
           << G4endl;
}

// 构建函数
G4VPhysicalVolume *DetectorConstruction::Construct()
{
    DefineMaterials();

    const G4double worldXY = 6 * cm;
    const G4double worldZ = 8 * cm;

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
    // 定义均相薄膜（Film）
    // =========================
    const G4double filmXY = 5 * cm;
    const G4double filmT = fFilmThicknessInput;
    const G4ThreeVector filmPos(0, 0, 0);

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
    // 铝板
    // =========================

    // 定义 0.5mm 铝板 (Aluminum Substrate)，放置在入射面 (+z 侧)
    G4Material *matAl = G4NistManager::Instance()->FindOrBuildMaterial("G4_Al");
    const G4double alXY = filmXY; // 假设铝板长宽与薄膜一致
    const G4double alT = 0.5 * mm;

    // 铝板中心 Z 坐标 = 薄膜中心 + 半个薄膜厚度 + 半个铝板厚度
    const G4double alPosZ = fFilmCenterZ + 0.5 * fFilmThickness + 0.5 * alT;

    G4Box *solidAl = new G4Box("AlPlate", 0.5 * alXY, 0.5 * alXY, 0.5 * alT);
    G4LogicalVolume *logicAl = new G4LogicalVolume(solidAl, matAl, "AlPlate");

    new G4PVPlacement(
        nullptr,
        G4ThreeVector(0, 0, alPosZ),
        logicAl,
        "AlPlate",
        logicWorld,
        false,
        0);

    // =========================
    // 可视化属性
    // =========================
    logicWorld->SetVisAttributes(G4VisAttributes::GetInvisible());

    G4VisAttributes *filmVisAtt = new G4VisAttributes(G4Colour(0.9, 0.9, 0.3, 0.4));
    filmVisAtt->SetForceSolid(true);
    logicFilm->SetVisAttributes(filmVisAtt);

    // 为铝板设置银灰色半透明的可视化属性
    G4VisAttributes *alVisAtt = new G4VisAttributes(G4Colour(0.7, 0.7, 0.7, 0.5));
    alVisAtt->SetForceSolid(true);
    logicAl->SetVisAttributes(alVisAtt);

    return physWorld;
}

#include "RunAction.hh"
#include "DetectorConstruction.hh"

#include "G4Run.hh"
#include "G4RunManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4ios.hh"

// --- 新增：用于提取截面数据的头文件 ---
#include "G4HadronicProcessStore.hh"
#include "G4Neutron.hh"
#include "G4Material.hh"
#include "G4ProcessManager.hh"
#include "G4VProcess.hh"
// -------------------------------------

#include <fstream>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <string>

RunAction::RunAction(const AnalysisConfig *config)
    : G4UserRunAction(),
      fAnalysisConfig(config),
      fNIncident(0),
      fNCapture(0),
      fNTransmit(0),
      fTotalNeutronTrackLength(0.0)
{
}

RunAction::~RunAction()
{
}

// 开始时
void RunAction::BeginOfRunAction(const G4Run *aRun)
{
    fNIncident = 0;
    fNCapture = 0;
    fNTransmit = 0;
    fTotalNeutronTrackLength = 0.0;

    G4cout << "### Run " << aRun->GetRunID() << " Start." << G4endl;

    if (fAnalysisConfig)
    {
        G4cout << "  enableReactionPosition = " << fAnalysisConfig->enableReactionPosition << G4endl;
        G4cout << "  enableEdep             = " << fAnalysisConfig->enableEdep << G4endl;
        G4cout << "  enableTrackLen         = " << fAnalysisConfig->enableTrackLen << G4endl;
        G4cout << "  enableAttenuation      = " << fAnalysisConfig->enableAttenuation << G4endl;
    }

    // --- 提取截面数据准备 ---
    G4HadronicProcessStore *hadStore = G4HadronicProcessStore::Instance();
    G4ParticleDefinition *neutron = G4Neutron::Neutron();

    // 1. 获取物理过程管理器，同时寻找 (n,alpha) 和 (n,gamma) 过程
    G4ProcessManager *pManager = neutron->GetProcessManager();
    G4VProcess *procAlpha = nullptr; // 对应 Inelastic，主导 B10 吸收
    G4VProcess *procGamma = nullptr; // 对应 Capture，微小的辐射俘获

    if (pManager)
    {
        G4ProcessVector *pList = pManager->GetProcessList();
        for (std::size_t i = 0; i < pList->size(); ++i)
        {
            G4String pName = (*pList)[i]->GetProcessName();

            // 抓取 (n, alpha) 非弹性过程
            if (pName == "neutronInelastic" || pName == "neutronInelasticHP")
            {
                procAlpha = (*pList)[i];
            }
            // 抓取 (n, gamma) 辐射俘获过程
            if (pName == "nCapture" || pName == "HadronCapture" || pName == "neutronCapture" || pName == "nCaptureHP")
            {
                procGamma = (*pList)[i];
            }
        }
    }

    // 2. 获取你当前运行的混合薄膜材料
    const DetectorConstruction *detector =
        static_cast<const DetectorConstruction *>(
            G4RunManager::GetRunManager()->GetUserDetectorConstruction());

    G4Material *mat = nullptr;
    if (detector && detector->GetScoringVolume())
    {
        mat = detector->GetScoringVolume()->GetMaterial();
    }

    // 3. 打印对比表格
    if (mat)
    {
        G4cout << "\n=================================================================================" << G4endl;
        G4cout << "Neutron Cross Section Analysis for Material: " << mat->GetName() << G4endl;
        G4cout << "Effective Density: " << mat->GetDensity() / (g / cm3) << " g/cm3" << G4endl;
        G4cout << "---------------------------------------------------------------------------------" << G4endl;
        G4cout << "Energy\t\t(n, alpha) Macroscopic\tMeanFreePath\t|  (n, gamma) Macroscopic\t|  Total Absorption" << G4endl;
        G4cout << "      \t\t[Inelastic] (cm^-1)   \t[alpha] (um)\t|  [Capture]  (cm^-1)\t|  [Inelastic+Capture] (cm^-1)" << G4endl;
        G4cout << "---------------------------------------------------------------------------------" << G4endl;

        std::vector<G4double> energies = {
            0.0253 * eV,
            0.1 * eV,
            1.0 * eV,
            10.0 * eV,
            1.0 * keV,
            1.0 * MeV};

        for (G4double E : energies)
        {
            // 计算 (n, alpha) 数据
            G4double macXsAlpha = 0.0;
            if (procAlpha)
            {
                macXsAlpha = hadStore->GetCrossSectionPerVolume(neutron, E, procAlpha, mat);
            }
            G4double mfpAlpha = (macXsAlpha > 0.0) ? (1.0 / macXsAlpha) : -1.0;

            // 计算 (n, gamma) 数据
            G4double macXsGamma = 0.0;
            if (procGamma)
            {
                macXsGamma = hadStore->GetCrossSectionPerVolume(neutron, E, procGamma, mat);
            }

            // 总吸收截面（按你当前项目口径）
            G4double macXsTotalAbs = macXsAlpha + macXsGamma;

            // 格式化输出
            G4cout << std::fixed << std::setprecision(4)
                   << E / eV << " eV\t"
                   << macXsAlpha * cm << "\t\t\t"
                   << (mfpAlpha > 0 ? mfpAlpha / um : -1.0) << "\t\t|  "
                   << macXsGamma * cm << "\t\t|  "
                   << macXsTotalAbs * cm
                   << G4endl;
        }
        G4cout << "=================================================================================\n"
               << G4endl;
    }
    else
    {
        G4cout << "\n[Warning] Scoring volume material not found for cross section printing." << G4endl;
    }
}

// 结束时
void RunAction::EndOfRunAction(const G4Run *aRun)
{
    G4int nOther = fNIncident - fNCapture - fNTransmit;
    if (nOther < 0)
    {
        nOther = 0;
    }

    G4double captureEff = 0.0;
    G4double transmitEff = 0.0;
    G4double otherEff = 0.0;
    G4double attenuation = 0.0;
    G4double sigmaEff = 0.0;
    if (fTotalNeutronTrackLength > 0.0)
    {
        sigmaEff = static_cast<G4double>(fNCapture) / fTotalNeutronTrackLength;
    }

    if (fNIncident > 0)
    {
        captureEff = static_cast<G4double>(fNCapture) / fNIncident;
        transmitEff = static_cast<G4double>(fNTransmit) / fNIncident;
        otherEff = static_cast<G4double>(nOther) / fNIncident;
        attenuation = 1.0 - transmitEff;
    }

    G4cout << "### Run " << aRun->GetRunID() << " Stop." << G4endl;
    G4cout << "  N_incident             = " << fNIncident << G4endl;
    G4cout << "  N_capture              = " << fNCapture << G4endl;
    G4cout << "  N_transmit             = " << fNTransmit << G4endl;
    G4cout << "  total_neutron_track_um = " << (fTotalNeutronTrackLength / um) << G4endl;
    G4cout << "  sigma_eff_per_um       = " << (sigmaEff * um) << G4endl;
    G4cout << "  sigma_eff_per_cm       = " << (sigmaEff * cm) << G4endl;
    G4cout << "  capture_eff            = " << captureEff << G4endl;
    G4cout << "  transmit_eff           = " << transmitEff << G4endl;
    G4cout << "  other_eff              = " << otherEff << G4endl;
    G4cout << "  attenuation            = " << attenuation << G4endl;

    if (fAnalysisConfig && fAnalysisConfig->enableAttenuation)
    {
        const DetectorConstruction *detector =
            static_cast<const DetectorConstruction *>(
                G4RunManager::GetRunManager()->GetUserDetectorConstruction());

        if (!detector)
        {
            G4cerr << "Error: cannot get DetectorConstruction in RunAction." << G4endl;
            return;
        }

        const G4double thicknessUm = detector->GetFilmThickness() / um;

        namespace fs = std::filesystem;

        auto trimNumber = [](G4double x)
        {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(3) << x;
            std::string s = oss.str();
            while (!s.empty() && s.back() == '0')
                s.pop_back();
            if (!s.empty() && s.back() == '.')
                s.pop_back();
            return s;
        };

        const std::string ratioTag =
            trimNumber(detector->GetBNWt()) + "-" + trimNumber(detector->GetZnSWt());

        fs::path outDir =
            fs::current_path().parent_path() / "Data" / ratioTag / "neutron_transport_summary";

        std::error_code ec;
        fs::create_directories(outDir, ec);
        if (ec)
        {
            G4cerr << "Error: cannot create directory "
                   << outDir.string()
                   << " | " << ec.message() << G4endl;
            return;
        }

        fs::path outPath = outDir / "neutron_transport_summary.csv";

        G4bool needHeader = false;
        {
            std::ifstream checkFile(outPath.string());
            if (!checkFile.good() ||
                checkFile.peek() == std::ifstream::traits_type::eof())
            {
                needHeader = true;
            }
        }

        std::ofstream outFile(outPath.string(), std::ios::app);
        if (!outFile.is_open())
        {
            G4cerr << "Error: cannot open " << outPath.string() << G4endl;
            return;
        }

        if (needHeader)
        {
            outFile << "thickness_um,"
                    << "n_incident,"
                    << "n_capture,"
                    << "n_transmit,"
                    << "total_neutron_track_length_um,"
                    << "sigma_eff_per_um,"
                    << "sigma_eff_per_cm,"
                    << "capture_efficiency,"
                    << "transmission_efficiency,"
                    << "other_loss_efficiency,"
                    << "attenuation"
                    << G4endl;
        }

        outFile << thicknessUm << ","
                << fNIncident << ","
                << fNCapture << ","
                << fNTransmit << ","
                << (fTotalNeutronTrackLength / um) << ","
                << (sigmaEff * um) << ","
                << (sigmaEff * cm) << ","
                << captureEff << ","
                << transmitEff << ","
                << otherEff << ","
                << attenuation
                << G4endl;
    }
}

const AnalysisConfig *RunAction::GetAnalysisConfig() const
{
    return fAnalysisConfig;
}

void RunAction::CountIncident()
{
    ++fNIncident;
}

void RunAction::CountCapture()
{
    ++fNCapture;
}

void RunAction::CountTransmit()
{
    ++fNTransmit;
}

void RunAction::AddNeutronTrackLength(G4double len)
{
    if (len > 0.0)
    {
        fTotalNeutronTrackLength += len;
    }
}

G4double RunAction::GetTotalNeutronTrackLength() const
{
    return fTotalNeutronTrackLength;
}

G4double RunAction::GetSigmaEff() const
{
    if (fTotalNeutronTrackLength <= 0.0)
    {
        return 0.0;
    }

    return static_cast<G4double>(fNCapture) / fTotalNeutronTrackLength;
}
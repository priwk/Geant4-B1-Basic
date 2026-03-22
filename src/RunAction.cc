#include "RunAction.hh"
#include "DetectorConstruction.hh"

#include "G4Run.hh"
#include "G4RunManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4ios.hh"

#include <fstream>

RunAction::RunAction(const AnalysisConfig *config)
    : G4UserRunAction(),
      fAnalysisConfig(config),
      fNIncident(0),
      fNCapture(0),
      fNTransmit(0)
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

    G4cout << "### Run " << aRun->GetRunID() << " Start." << G4endl;

    if (fAnalysisConfig)
    {
        G4cout << "  enableReactionPosition = " << fAnalysisConfig->enableReactionPosition << G4endl;
        G4cout << "  enableEdep             = " << fAnalysisConfig->enableEdep << G4endl;
        G4cout << "  enableTrackLen         = " << fAnalysisConfig->enableTrackLen << G4endl;
        G4cout << "  enableAttenuation      = " << fAnalysisConfig->enableAttenuation << G4endl;
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

    if (fNIncident > 0)
    {
        captureEff = static_cast<G4double>(fNCapture) / fNIncident;
        transmitEff = static_cast<G4double>(fNTransmit) / fNIncident;
        otherEff = static_cast<G4double>(nOther) / fNIncident;
        attenuation = 1.0 - transmitEff;
    }

    G4cout << "### Run " << aRun->GetRunID() << " Stop." << G4endl;
    G4cout << "  N_incident   = " << fNIncident << G4endl;
    G4cout << "  N_capture    = " << fNCapture << G4endl;
    G4cout << "  N_transmit   = " << fNTransmit << G4endl;
    G4cout << "  capture_eff  = " << captureEff << G4endl;
    G4cout << "  transmit_eff = " << transmitEff << G4endl;
    G4cout << "  other_eff    = " << otherEff << G4endl;
    G4cout << "  attenuation  = " << attenuation << G4endl;

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

        G4double thicknessUm = detector->GetFilmThickness() / um;

        // 判断文件是否为空，若为空则先写表头
        G4bool needHeader = false;
        {
            std::ifstream checkFile("capture_summary.csv");
            if (!checkFile.good() ||
                checkFile.peek() == std::ifstream::traits_type::eof())
            {
                needHeader = true;
            }
        }

        std::ofstream outFile("capture_summary.csv", std::ios::app);
        if (!outFile.is_open())
        {
            G4cerr << "Error: cannot open csv" << G4endl;
            return;
        }

        if (needHeader)
        {
            outFile << "thickness_um,"
                    << "n_incident,"
                    << "n_capture,"
                    << "n_transmit,"
                    << "capture_eff,"
                    << "transmit_eff,"
                    << "other_eff,"
                    << "attenuation"
                    << G4endl;
        }

        outFile << thicknessUm << ","
                << fNIncident << ","
                << fNCapture << ","
                << fNTransmit << ","
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
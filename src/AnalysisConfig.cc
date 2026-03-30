#include "AnalysisConfig.hh"

#include "G4GenericMessenger.hh"
#include "G4ios.hh"

#include <algorithm>
#include <cctype>
#include <string>

namespace
{
  std::string ToUpperCopy(const G4String &s)
  {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c)
                   { return static_cast<char>(std::toupper(c)); });
    return out;
  }

  const char *PresetName(AnalysisConfig::Preset p)
  {
    switch (p)
    {
    case AnalysisConfig::THICKNESS_SCREENING:
      return "THICKNESS_SCREENING";
    case AnalysisConfig::SOURCE_GENERATION:
      return "SOURCE_GENERATION";
    case AnalysisConfig::REPLAY_DEBUG:
      return "REPLAY_DEBUG";
    case AnalysisConfig::REPLAY_PRODUCTION:
      return "REPLAY_PRODUCTION";
    case AnalysisConfig::CUSTOM:
      return "CUSTOM";
    default:
      return "UNKNOWN";
    }
  }
}

AnalysisConfig::AnalysisConfig()
    : fMessenger(nullptr)
{
  SetDefaults();

  fMessenger = new G4GenericMessenger(this, "/cfg/", "Analysis/replay config");

  auto &presetCmd =
      fMessenger->DeclareMethod("usePreset",
                                &AnalysisConfig::UsePresetByName,
                                "Use preset: THICKNESS_SCREENING, SOURCE_GENERATION, REPLAY_DEBUG, REPLAY_PRODUCTION, CUSTOM");
  presetCmd.SetParameterName("preset", false);
  presetCmd.SetStates(G4State_PreInit, G4State_Idle);

  auto &scaleCmd =
      fMessenger->DeclareMethod("setReplayScale",
                                &AnalysisConfig::SetReplayScale,
                                "Set replay scale factor (>=0).");
  scaleCmd.SetParameterName("scale", false);
  scaleCmd.SetRange("scale>=0.");
  scaleCmd.SetStates(G4State_PreInit, G4State_Idle);

  auto &lineCmd =
      fMessenger->DeclareMethod("setReplayUseLineSource",
                                &AnalysisConfig::SetReplayUseLineSource,
                                "Set replay line source mode: true/false.");
  lineCmd.SetParameterName("useLineSource", false);
  lineCmd.SetStates(G4State_PreInit, G4State_Idle);

  auto &edepWeightCmd =
      fMessenger->DeclareMethod("setReplayUseEdepWeight",
                                &AnalysisConfig::SetReplayUseEdepWeight,
                                "Set replay edep-weight mode: true/false.");
  edepWeightCmd.SetParameterName("useEdepWeight", false);
  edepWeightCmd.SetStates(G4State_PreInit, G4State_Idle);

  auto &maxPhotonCmd =
      fMessenger->DeclareMethod("setReplayMaxPhotonsPerEvent",
                                &AnalysisConfig::SetReplayMaxPhotonsPerEvent,
                                "Set replay max photons per event (0 = unlimited).");
  maxPhotonCmd.SetParameterName("maxPhotons", false);
  maxPhotonCmd.SetRange("maxPhotons>=0");
  maxPhotonCmd.SetStates(G4State_PreInit, G4State_Idle);

  auto &perStepCmd =
      fMessenger->DeclareMethod("setReplayPhotonsPerStep",
                                &AnalysisConfig::SetReplayPhotonsPerStep,
                                "Set replay photons per step (>=1).");
  perStepCmd.SetParameterName("photonsPerStep", false);
  perStepCmd.SetRange("photonsPerStep>=1");
  perStepCmd.SetStates(G4State_PreInit, G4State_Idle);

  auto &energyCmd =
      fMessenger->DeclareMethod("setReplayPhotonEnergyEV",
                                &AnalysisConfig::SetReplayPhotonEnergyEV,
                                "Set replay optical photon energy in eV (>0).");
  energyCmd.SetParameterName("energyEV", false);
  energyCmd.SetRange("energyEV>0.");
  energyCmd.SetStates(G4State_PreInit, G4State_Idle);

  auto &loopCmd =
      fMessenger->DeclareMethod("setReplayLoopEvents",
                                &AnalysisConfig::SetReplayLoopEvents,
                                "Set replay loop mode: true/false.");
  loopCmd.SetParameterName("loopEvents", false);
  loopCmd.SetStates(G4State_PreInit, G4State_Idle);

  auto &verboseEventCmd =
      fMessenger->DeclareMethod("setVerboseEventPrint",
                                &AnalysisConfig::SetVerboseEventPrint,
                                "Enable/disable verbose event print.");
  verboseEventCmd.SetParameterName("verboseEventPrint", false);
  verboseEventCmd.SetStates(G4State_PreInit, G4State_Idle);

  auto &verboseCaptureCmd =
      fMessenger->DeclareMethod("setVerboseCapturePrint",
                                &AnalysisConfig::SetVerboseCapturePrint,
                                "Enable/disable verbose capture print.");
  verboseCaptureCmd.SetParameterName("verboseCapturePrint", false);
  verboseCaptureCmd.SetStates(G4State_PreInit, G4State_Idle);

  auto &verboseEveryCmd =
      fMessenger->DeclareMethod("setVerboseEveryNEvents",
                                &AnalysisConfig::SetVerboseEveryNEvents,
                                "Set verbose print interval (>=1).");
  verboseEveryCmd.SetParameterName("N", false);
  verboseEveryCmd.SetRange("N>=1");
  verboseEveryCmd.SetStates(G4State_PreInit, G4State_Idle);

  G4cout << "[AnalysisConfig] initialized with defaults. "
         << "Use /cfg/usePreset ... before /run/initialize if needed."
         << G4endl;
}

AnalysisConfig::~AnalysisConfig()
{
  delete fMessenger;
}

void AnalysisConfig::SetDefaults()
{
  enableReactionPosition = false;
  enableEdep = false;
  enableTrackLen = false;
  enableAttenuation = false;

  enableLightYield = false;
  lightOnlyForCaptureEvents = false;
  lightYieldPhotonsPerMeV = 20000.0;

  enableAlphaLiStepCSV = false;
  stepCSVOnlyWithEdep = false;
  stepCSVOnlyPrimaryCaptureProducts = false;

  enableSourceReplay = false;
  // replayRunID = 0; // 兼容保留；当前通常不再依赖此项选文件
  replayUseLineSource = false;
  replayPhotonsPerStep = 1;
  replayScale = 1.0;
  replayUseEdepWeight = false;
  replayMaxPhotonsPerEvent = 1000;
  replayPhotonEnergy_eV = 2.95;
  replayLoopEvents = false;

  enableVerboseEventPrint = false;
  enableVerboseCapturePrint = false;
  verboseEveryNEvents = 2000;
}

void AnalysisConfig::ApplyPreset(Preset p)
{
  SetDefaults();

  switch (p)
  {
  case THICKNESS_SCREENING:
    enableReactionPosition = true;
    enableEdep = true;
    enableAttenuation = true;
    break;

  case SOURCE_GENERATION:
    enableAlphaLiStepCSV = true;
    stepCSVOnlyWithEdep = true;
    stepCSVOnlyPrimaryCaptureProducts = true;
    break;

  case REPLAY_DEBUG:
    enableSourceReplay = true;
    replayUseLineSource = false;
    replayUseEdepWeight = false;
    replayPhotonsPerStep = 1;
    replayScale = 1.0;
    replayMaxPhotonsPerEvent = 2000;
    replayPhotonEnergy_eV = 2.95;
    replayLoopEvents = false;
    break;

  case REPLAY_PRODUCTION:
    enableSourceReplay = true;
    replayUseLineSource = true;
    replayUseEdepWeight = true;
    replayPhotonsPerStep = 1;
    replayScale = 0.5;
    replayMaxPhotonsPerEvent = 20000;
    replayPhotonEnergy_eV = 2.95;
    replayLoopEvents = false;
    enableVerboseEventPrint = false;
    enableVerboseCapturePrint = false;
    verboseEveryNEvents = 2000;
    break;

  case CUSTOM:
    // 仅保留默认值，供用户后续用 /cfg/xxx 单独覆盖
    break;
  }

  G4cout << "[AnalysisConfig] preset applied: " << PresetName(p) << G4endl;
}

void AnalysisConfig::UsePresetByName(const G4String &name)
{
  const std::string key = ToUpperCopy(name);

  if (key == "THICKNESS_SCREENING")
  {
    ApplyPreset(THICKNESS_SCREENING);
  }
  else if (key == "SOURCE_GENERATION")
  {
    ApplyPreset(SOURCE_GENERATION);
  }
  else if (key == "REPLAY_DEBUG")
  {
    ApplyPreset(REPLAY_DEBUG);
  }
  else if (key == "REPLAY_PRODUCTION")
  {
    ApplyPreset(REPLAY_PRODUCTION);
  }
  else if (key == "CUSTOM")
  {
    ApplyPreset(CUSTOM);
  }
  else
  {
    G4cerr << "[AnalysisConfig] Unknown preset: " << name << G4endl;
    G4cerr << "[AnalysisConfig] Valid presets: "
           << "THICKNESS_SCREENING, SOURCE_GENERATION, "
           << "REPLAY_DEBUG, REPLAY_PRODUCTION, CUSTOM"
           << G4endl;
  }
}

void AnalysisConfig::SetReplayScale(G4double v)
{
  if (v < 0.0)
  {
    G4cerr << "[AnalysisConfig] replayScale must be >= 0." << G4endl;
    return;
  }

  replayScale = v;
  G4cout << "[AnalysisConfig] replayScale = " << replayScale << G4endl;
}

void AnalysisConfig::SetReplayUseLineSource(G4bool v)
{
  replayUseLineSource = v;
  G4cout << "[AnalysisConfig] replayUseLineSource = "
         << (replayUseLineSource ? "true" : "false") << G4endl;
}

void AnalysisConfig::SetReplayUseEdepWeight(G4bool v)
{
  replayUseEdepWeight = v;
  G4cout << "[AnalysisConfig] replayUseEdepWeight = "
         << (replayUseEdepWeight ? "true" : "false") << G4endl;
}

void AnalysisConfig::SetReplayMaxPhotonsPerEvent(G4int v)
{
  if (v < 0)
  {
    G4cerr << "[AnalysisConfig] replayMaxPhotonsPerEvent must be >= 0." << G4endl;
    return;
  }

  replayMaxPhotonsPerEvent = v;
  G4cout << "[AnalysisConfig] replayMaxPhotonsPerEvent = "
         << replayMaxPhotonsPerEvent << G4endl;
}

void AnalysisConfig::SetReplayPhotonsPerStep(G4int v)
{
  if (v < 1)
  {
    G4cerr << "[AnalysisConfig] replayPhotonsPerStep must be >= 1." << G4endl;
    return;
  }

  replayPhotonsPerStep = v;
  G4cout << "[AnalysisConfig] replayPhotonsPerStep = "
         << replayPhotonsPerStep << G4endl;
}

void AnalysisConfig::SetReplayPhotonEnergyEV(G4double v)
{
  if (v <= 0.0)
  {
    G4cerr << "[AnalysisConfig] replayPhotonEnergy_eV must be > 0." << G4endl;
    return;
  }

  replayPhotonEnergy_eV = v;
  G4cout << "[AnalysisConfig] replayPhotonEnergy_eV = "
         << replayPhotonEnergy_eV << " eV" << G4endl;
}

void AnalysisConfig::SetReplayLoopEvents(G4bool v)
{
  replayLoopEvents = v;
  G4cout << "[AnalysisConfig] replayLoopEvents = "
         << (replayLoopEvents ? "true" : "false") << G4endl;
}

void AnalysisConfig::SetVerboseEventPrint(G4bool v)
{
  enableVerboseEventPrint = v;
  G4cout << "[AnalysisConfig] enableVerboseEventPrint = "
         << (enableVerboseEventPrint ? "true" : "false") << G4endl;
}

void AnalysisConfig::SetVerboseCapturePrint(G4bool v)
{
  enableVerboseCapturePrint = v;
  G4cout << "[AnalysisConfig] enableVerboseCapturePrint = "
         << (enableVerboseCapturePrint ? "true" : "false") << G4endl;
}

void AnalysisConfig::SetVerboseEveryNEvents(G4int v)
{
  if (v < 1)
  {
    G4cerr << "[AnalysisConfig] verboseEveryNEvents must be >= 1." << G4endl;
    return;
  }

  verboseEveryNEvents = v;
  G4cout << "[AnalysisConfig] verboseEveryNEvents = "
         << verboseEveryNEvents << G4endl;
}
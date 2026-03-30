#ifndef AnalysisConfig_h
#define AnalysisConfig_h 1

#include "globals.hh"

class G4GenericMessenger;

/*
模式 A：厚度趋势预筛选
enableReactionPosition = true;
enableEdep = true;
enableAttenuation = true;
其余全关

模式 B：源项文件生成
enableAlphaLiStepCSV = true;
stepCSVOnlyWithEdep = true;
stepCSVOnlyPrimaryCaptureProducts = true;
其余全关

模式 C：最小 replay 调试
enableSourceReplay = true;
replayUseLineSource = false;
replayUseEdepWeight = false;
replayPhotonsPerStep = 1;
replayMaxPhotonsPerEvent = 2000;
其余全关

模式 D：正式 replay
enableSourceReplay = true;
replayUseLineSource = true;
replayUseEdepWeight = true;
replayScale = 小值；
replayMaxPhotonsPerEvent = 中等值；
其余一阶段输出全关
*/

class AnalysisConfig
{
public:
  enum Preset
  {
    THICKNESS_SCREENING, // A
    SOURCE_GENERATION,   // B
    REPLAY_DEBUG,        // C
    REPLAY_PRODUCTION,   // D
    CUSTOM               // 仅恢复默认值，便于后续用 /cfg/xxx 单独覆盖
  };

public:
  AnalysisConfig();
  ~AnalysisConfig();

  void SetDefaults();
  void ApplyPreset(Preset p);
  void UsePresetByName(const G4String &name);

  void SetReplayScale(G4double v);
  void SetReplayUseLineSource(G4bool v);
  void SetReplayUseEdepWeight(G4bool v);
  void SetReplayMaxPhotonsPerEvent(G4int v);
  void SetReplayPhotonsPerStep(G4int v);
  void SetReplayPhotonEnergyEV(G4double v);
  void SetReplayLoopEvents(G4bool v);

  void SetVerboseEventPrint(G4bool v);
  void SetVerboseCapturePrint(G4bool v);
  void SetVerboseEveryNEvents(G4int v);

public:
  bool enableReactionPosition = false; // 是否记录反应位置（即发生俘获的位置）
  bool enableEdep = false;             // 是否记录沉积能量
  bool enableTrackLen = false;         // 是否记录轨迹长度
  bool enableAttenuation = false;      // 是否启用透过判定

  bool enableLightYield = false;                  // 是否启用光产额
  bool lightOnlyForCaptureEvents = false;         // 光产额事件输出是否仅限发生俘获的事件
  double lightYieldPhotonsPerMeV = 20000.0;       // 有效光产额，单位 photons/MeV
  bool enableAlphaLiStepCSV = false;              // 是否输出 alpha / Li7 的 step 级源项数据
  bool stepCSVOnlyWithEdep = false;               // 是否只输出有能量沉积的 step
  bool stepCSVOnlyPrimaryCaptureProducts = false; // 是否仅记录俘获直接产生的 alpha / Li7

  bool enableSourceReplay = false; // 是否启用第二阶段：源重放
  // int replayRunID = 0;               // 兼容保留：当前按厚度自动读文件，通常不再使用此项
  bool replayUseLineSource = false;    // false=用 midPos 点源；true=用 pre/post 线源
  int replayPhotonsPerStep = 1;        // 每条 step 生成多少个 optical photons
  double replayScale = 1.0;            // 光子数缩放系数
  bool replayUseEdepWeight = false;    // 是否按 edep 加权
  int replayMaxPhotonsPerEvent = 0;    // 0=不限制；>0=事件级上限
  double replayPhotonEnergy_eV = 2.95; // replay 模式下 optical photon 的固定能量
  bool replayLoopEvents = false;       // 放完后是否循环

  bool enableVerboseEventPrint = false;   // 每个事件结束时是否打印
  bool enableVerboseCapturePrint = false; // 识别到俘获时是否立刻打印
  int verboseEveryNEvents = 2000;         // 每隔多少个事件打印一次

private:
  G4GenericMessenger *fMessenger = nullptr;
};

#endif
#ifndef AnalysisConfig_h
#define AnalysisConfig_h 1

class AnalysisConfig
{
public:
  AnalysisConfig() = default;
  ~AnalysisConfig() = default;

public:
  bool enableReactionPosition = false; // 是否记录反应位置（即发生俘获的位置）
  bool enableEdep = false;             // 是否记录沉积能量
  bool enableTrackLen = false;        // 是否记录轨迹长度
  bool enableAttenuation = false;     // 是否启用透过判定

  bool enableLightYield = true;                  // 是否启用“基于总沉积能量换算光产额”的统计
  bool lightOnlyForCaptureEvents = true;         // 光产额事件输出是否仅限发生俘获的事件
  double lightYieldPhotonsPerMeV = 50000.0;      // 有效光产额，单位 photons/MeV
  bool enableAlphaLiStepCSV = true;             // 是否输出 alpha / Li7 的 step 级源项数据
  bool stepCSVOnlyWithEdep = true;               // 是否只输出有能量沉积的 step
  bool stepCSVOnlyPrimaryCaptureProducts = true; // 是否仅记录俘获直接产生的 alpha / Li7

  bool enableVerboseEventPrint = true;    // 每个事件结束时是否打印
  bool enableVerboseCapturePrint = false; // 识别到俘获时是否立刻打印
  int verboseEveryNEvents = 2000;         // 每隔多少个事件打印一次
};

#endif
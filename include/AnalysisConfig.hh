#ifndef AnalysisConfig_h
#define AnalysisConfig_h 1

class AnalysisConfig
{
public:
  AnalysisConfig() = default;
  ~AnalysisConfig() = default;

public:
  bool enableReactionPosition = true; // 是否记录反应位置（即发生俘获的位置）
  bool enableEdep = true;              // 是否记录沉积能量
  bool enableTrackLen = true;          // 是否记录轨迹长度
  bool enableAttenuation = false;        // 是否启用透过判定

  bool enableVerboseEventPrint = true;    // 每个事件结束时是否打印
  bool enableVerboseCapturePrint = false; // 识别到俘获时是否立刻打印
  int verboseEveryNEvents = 2000;          // 每隔多少个事件打印一次
};

#endif
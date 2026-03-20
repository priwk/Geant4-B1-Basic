#ifndef AnalysisConfig_h
#define AnalysisConfig_h 1

class AnalysisConfig
{
public:
  AnalysisConfig() = default;
  ~AnalysisConfig() = default;

public:
  bool enableReactionPosition = true;
  bool enableEdep = true;
  bool enableTrackLen = true;
  bool enableAttenuation = true;

  bool enableVerboseEventPrint = true;    // 每个事件结束时是否打印
  bool enableVerboseCapturePrint = false; // 识别到俘获时是否立刻打印
  int verboseEveryNEvents = 200;          // 每隔多少个事件打印一次
};

#endif
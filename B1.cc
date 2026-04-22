#include "DetectorConstruction.hh" // 用户自定义：几何、材料、体积摆放
#include "ActionInitialization.hh" // 用户自定义：注册 PrimaryGeneratorAction、RunAction、EventAction 等

// 根据是否开启多线程，选择不同的 RunManager
#ifdef G4MULTITHREADED
#include "G4MTRunManager.hh" // 多线程运行管理器
#else
#include "G4RunManager.hh" // 单线程运行管理器
#endif

#include "G4UImanager.hh" // UI 命令管理器，用于执行宏命令
// #include "QBBC.hh"           // Geant4 自带的一个参考物理表（这里被注释备用）
#include "QGSP_BIC_HP.hh"    // Geant4 自带的一个参考物理表
#include "PhysicsList.hh"    // 用户自定义物理表
#include "G4VisExecutive.hh" // 可视化管理器
#include "G4UIExecutive.hh"  // 交互式 UI 会话（Qt/terminal 等）
#include "Randomize.hh"      // 随机数引擎相关

#include "G4OpticalPhysics.hh" // 光学物理过程

// 主函数

int main(int argc, char **argv)
{
  // =========================================================
  // 1. 判断程序是“交互模式”还是“批处理模式”
  // =========================================================
  //
  // argc == 1 说明启动程序时没有额外传入宏文件参数，
  // 这时通常进入交互模式，例如：
  //   ./B1
  //
  // 若 argc > 1，通常表示用户传入了一个宏文件，
  // 例如：
  //   ./B1 run.mac
  // 这时一般按批处理模式执行，不进入交互界面。

  G4UIExecutive *ui = 0;
  if (argc == 1)
  {
    ui = new G4UIExecutive(argc, argv); // 创建交互式 UI 会话
  }

  // =========================================================
  // 2. 设置随机数引擎
  // =========================================================
  //
  // Geant4 需要随机数来处理粒子输运、相互作用采样等过程。
  // 这里显式指定使用 CLHEP::RanecuEngine 作为随机数引擎。
  //
  // 说明：
  // - 不同随机数引擎会影响随机序列
  // - 在需要结果可复现时，还常常会进一步设置随机种子

  G4Random::setTheEngine(new CLHEP::RanecuEngine);

  // =========================================================
  // 3. 创建 RunManager
  // =========================================================
  //
  // RunManager 是 Geant4 程序的核心调度器，负责组织整个模拟流程：
  // - 初始化几何、材料、物理过程
  // - 管理事例（event）循环
  // - 调用用户定义的 action
  //
  // 如果编译时启用了多线程，则使用 G4MTRunManager；
  // 否则使用普通的 G4RunManager。

  /*
#ifdef G4MULTITHREADED
  G4MTRunManager *runManager = new G4MTRunManager;
  runManager->SetNumberOfThreads(1);
#else
  G4RunManager *runManager = new G4RunManager;
#endif
  */

  G4RunManager *runManager = new G4RunManager;

  // =========================================================
  // 4. 注册“必须的初始化类”
  // =========================================================
  //
  // 这三类通常是 Geant4 主程序最核心的注册内容：
  // 1) DetectorConstruction：定义几何与材料
  // 2) PhysicsList：定义物理过程
  // 3) ActionInitialization：定义用户动作类

  // ---------- 4.1 注册几何与材料 ----------
  //
  // DetectorConstruction 中通常包含：
  // - 世界体（World）
  // - 探测器本体
  // - 材料定义
  // - 几何摆放关系
  //
  // 没有它，程序就不知道“粒子在什么地方运动”
  runManager->SetUserInitialization(new DetectorConstruction());

  // ---------- 4.2 注册物理表 ----------
  //
  // 物理表决定粒子在材料中会发生哪些物理过程，
  // 例如：
  // - 电磁过程
  // - 衰变
  // - 中子散射、俘获
  // - 热中子相关高精度过程等
  //
  // 下面这段被注释的代码表示：原本也可以直接使用
  // Geant4 自带的 QBBC 物理表。
  //
  // 但当前真正生效的是下一行：
  //   runManager->SetUserInitialization(new PhysicsList());
  //
  // 也就是说，你现在使用的是“自己写的 PhysicsList”，
  // 而不是 Geant4 默认的 QBBC。
  // 这对中子模拟非常关键，因为最终是否有热中子俘获、
  // 是否有 10B(n,alpha)7Li、是否有 nKiller 等，
  // 都取决于 PhysicsList 里到底注册了什么。

  // 1. 实例化你自己写的 PhysicsList
  G4VUserPhysicsList *myPhysicsList = new PhysicsList();

  // 2. 将它注册给 runManager （也就是你问的这行代码的正确用法）
  runManager->SetUserInitialization(myPhysicsList);

  // ---------- 4.3 注册用户动作初始化 ----------
  //
  // ActionInitialization 用来集中注册各种用户动作类，例如：
  // - PrimaryGeneratorAction：定义初级粒子源
  // - RunAction：每个 run 的开始/结束操作
  // - EventAction：每个 event 的开始/结束操作
  // - SteppingAction：每一步输运时的操作
  //
  // 没有它，程序虽然有几何和物理过程，但不知道“谁来发粒子”
  runManager->SetUserInitialization(new ActionInitialization());

  // =========================================================
  // 5. 初始化可视化系统
  // =========================================================
  //
  // G4VisExecutive 是 Geant4 提供的可视化管理器，
  // 会根据当前环境自动选择可用的可视化后端。
  //
  // 常见的可视化驱动有：
  // - OGL / OGLSQt
  // - HepRep
  // - DAWN
  // - RayTracer
  //
  // visManager->Initialize() 之后，/vis/... 宏命令才能正常使用。

  G4VisManager *visManager = new G4VisExecutive;

  // 也可以传入字符串控制输出风格，例如 "Quiet"
  // G4VisManager* visManager = new G4VisExecutive("Quiet");

  visManager->Initialize();

  // =========================================================
  // 6. 获取 UI 命令管理器
  // =========================================================
  //
  // G4UImanager 负责接收并执行 Geant4 命令，
  // 无论这些命令来自：
  // - 宏文件（.mac）
  // - 交互界面手动输入
  //
  // 例如：
  // /run/initialize
  // /vis/open OGLSQt
  // /gps/particle neutron
  // /run/beamOn 10

  G4UImanager *UImanager = G4UImanager::GetUIpointer();

  // =========================================================
  // 7. 根据模式决定程序怎么运行
  // =========================================================
  //
  // 若 ui == nullptr，说明当前是“批处理模式”：
  // 用户在命令行传入了一个宏文件，例如：
  //   ./B1 run.mac
  //
  // 此时程序会直接执行这个宏文件，然后结束。
  //
  // 若 ui != nullptr，说明当前是“交互模式”：
  // 程序会先执行 init_vis.mac，再进入交互界面，
  // 用户可以在界面里继续输入命令。

  if (!ui)
  {
    // -------------------------
    // 7.1 批处理模式
    // -------------------------
    //
    // 将用户传入的宏文件名拼接成 Geant4 命令：
    //   /control/execute xxx.mac
    //
    // 例如 argv[1] 若为 run.mac，
    // 则最终执行：
    //   /control/execute run.mac

    G4String command = "/control/execute ";
    G4String fileName = argv[1];
    UImanager->ApplyCommand(command + fileName);
  }
  else
  {
    // -------------------------
    // 7.2 交互模式
    // -------------------------
    //
    // 启动时先执行 init_vis.mac。
    // 这个文件通常用来：
    // - 设置 verbose 等级
    // - /run/initialize
    // - 打开可视化窗口
    // - 执行 vis.mac
    //
    // 注意：
    // 这里只自动执行 init_vis.mac，
    // 并不会自动执行 run.mac，除非你在 init_vis.mac 中
    // 显式写了：
    //   /control/execute run.mac
    //
    // 因此，如果程序启动后只有几何没有轨迹，
    // 常见原因之一就是：run.mac 其实没有被执行。

    UImanager->ApplyCommand("/control/execute init_vis.mac");

    // 进入交互式会话
    // 之后用户可以继续在终端/Qt 界面中输入命令
    ui->SessionStart();

    // 交互会话结束后释放 UI 对象
    delete ui;
  }

  // =========================================================
  // 8. 程序结束前释放资源
  // =========================================================
  //
  // 注意：
  // DetectorConstruction / PhysicsList / ActionInitialization
  // 这些通过 SetUserInitialization() 注册进去的对象，
  // 生命周期由 runManager 接管，最终会由 runManager 负责删除。
  //
  // 因此 main() 中不需要、也不应该手动 delete 它们。

  delete visManager;
  delete runManager;
}
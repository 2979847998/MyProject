# MyProject — UE5 ARPG 玩法系统 Demo

基于 **Unreal Engine 5.7 + 纯 C++** 开发的单机 ARPG Demo,覆盖「战斗 → 养成 → 收集 → 设置」完整玩法闭环。全部玩法逻辑与 UI 框架使用 C++ 实现,采用组件化 + 委托广播架构,各系统挂载于 PlayerController 并相互解耦。

## 功能特性

- **战斗系统**:属性组件(HP/MP/攻击/防御,防御减伤公式)、Buff 系统(计时/永久、刷新/叠加/覆盖规则、乘算数值汇总)、技能系统(冷却、能量消耗与自然回复、上 Buff/即时效果、蓝图事件钩子)
- **背包系统**:格子堆叠 / 拆分 / 排序 / 交换 / 拾取高亮,分页 TileView,物品拖拽跟随鼠标
- **装备系统**:五槽位(武器/头盔/护甲/鞋子/饰品)穿戴与卸下、属性加成应用与失败回滚、武器 Actor 生成并挂载骨骼 Socket
- **抽卡系统**:单抽 / 十连
- **UI 框架**:五层 Z 序面板栈(HUD/面板/弹窗/Tooltip/顶层)、面板缓存复用、输入模式与暂停统一管理、纯 C++ WidgetTree 动态构建界面、通用确认弹窗
- **输入系统**:Enhanced Input 动态创建/克隆 IMC,运行时键位重绑(改键即时生效、同键冲突互斥)、鼠标点击移动
- **设置系统**:音量(音频总线)/ 分辨率 / 按键,持久化保存
- **存档系统**:背包 / 装备 / 设置三套 SaveGame,兼容旧版本存档数据迁移
- **数据驱动**:物品 / Buff / 技能以 Data Asset 目录配置,展示文本走 CSV 字符串表本地化

## 技术亮点

- **组件化架构**:属性 / Buff / 技能 / 背包 / 装备 / 抽卡 / UI 管理均为 ActorComponent 挂载于 PlayerController,彼此通过动态多播委托通信,UI 与玩法逻辑零直接依赖;
- **自研 UI 框架**:五层 Z 序面板栈 + 面板缓存复用 + 输入模式与暂停统一调度;装备面板直接用 WidgetTree 在 C++ 中动态构建,无需蓝图 WBP;
- **运行时键位重绑**:克隆资产 IMC 到运行时上下文并重建映射,设置页改键即时生效,并处理同键冲突互斥;
- **数据驱动 + 本地化**:物品 / Buff / 技能以 Data Asset 目录配置,展示文本走 CSV 字符串表;
- **内存安全**:跨对象动态委托在 NativeDestruct / EndPlay 显式解绑,静态目录缓存按强 / 弱指针选型,规避 GC 后悬垂访问;
- **性能治理**:无逻辑组件关闭 Tick、空闲自动停 Tick、UI 增量刷新与控件复用(消除每秒重建)、热路径缓存、存档写入零冗余读盘;全量编译 0 错误 0 警告。

## 核心代码导览

| 模块 | 代码位置 |
| --- | --- |
| 属性 / 受击 / 死亡 | [`AttributeComponent.cpp`](Source/MyProject/Private/AttributeComponent.cpp) · [`AMyEnemyBase.cpp`](Source/MyProject/Private/AMyEnemyBase.cpp) |
| Buff 系统(计时 / 叠加 / 数值汇总) | [`BuffsComponent.cpp`](Source/MyProject/Private/BuffsComponent.cpp) · [`BuffCatalog.cpp`](Source/MyProject/Private/BuffCatalog.cpp) |
| 技能系统(冷却 / 能量 / 效果) | [`SkillsComponent.cpp`](Source/MyProject/Private/SkillsComponent.cpp) · [`SkillCatalog.cpp`](Source/MyProject/Private/SkillCatalog.cpp) |
| 背包数据与存档 | [`InventoryData.cpp`](Source/MyProject/Private/InventoryData.cpp) · [`InventoryComponent.cpp`](Source/MyProject/Private/InventoryComponent.cpp) |
| 装备加成与挂载 | [`EquipmentComponent.cpp`](Source/MyProject/Private/EquipmentComponent.cpp) |
| 抽卡 | [`GachaComponent.cpp`](Source/MyProject/Private/GachaComponent.cpp) |
| UI 框架 / 面板栈管理 | [`UIManagerComponent.cpp`](Source/MyProject/Private/UIManagerComponent.cpp) · [`UIViewBase.cpp`](Source/MyProject/Private/UIViewBase.cpp) |
| HUD 与各界面控件 | [`UIHUDWidget.cpp`](Source/MyProject/Private/UIHUDWidget.cpp) · [`InventoryWidget.cpp`](Source/MyProject/Private/InventoryWidget.cpp) · [`EquipmentWidget.cpp`](Source/MyProject/Private/EquipmentWidget.cpp) |
| 输入 / 点击移动 | [`MyPlayerController.cpp`](Source/MyProject/Private/MyPlayerController.cpp) |
| 设置子系统 / 键位重绑 | [`USettingsSubsystem.cpp`](Source/MyProject/Private/USettingsSubsystem.cpp) |

## 环境要求

- Unreal Engine **5.7**(开发版本 5.7.4)
- Visual Studio 2022(Windows)

## 构建与运行

1. 右键 `MyProject.uproject` → **Generate Visual Studio project files** 生成解决方案(仓库不包含 `.sln`,它含本机绝对路径)
2. 编译:`MyProjectEditor` 目标(Win64 / Development Editor)。命令行方式:

   ```
   <引擎路径>\Engine\Build\BatchFiles\Build.bat MyProjectEditor Win64 Development -Project="<工程路径>\MyProject.uproject"
   ```

3. 双击 `MyProject.uproject` 打开编辑器,点击 **Play** 运行

## 操作说明(默认键,可在 设置 → 按键 中重绑)

| 按键 | 功能 |
| --- | --- |
| 鼠标左键 | 点击地面移动 / UI 点击 |
| `I` | 打开 / 关闭背包 |
| `E` | 打开 / 关闭装备面板 |
| `M` | 打开 / 关闭地图 |
| `G` | 打开 / 关闭抽卡 |
| `Esc` | 关闭当前面板 |

## 目录结构

```
MyProject/
├── Source/MyProject/     # 全部 C++ 代码:组件化玩法系统 + UMG UI 框架
├── Content/              # 地图、数据资产(物品/Buff/技能目录)、UI 与模型资源
├── Config/               # 项目配置(输入、渲染、引擎设置)
└── MyProject.uproject    # 工程文件
```

> `Binaries/`、`Intermediate/`、`Saved/`、`DerivedDataCache/` 等生成目录已加入 `.gitignore`,不随仓库分发。

## 说明

- 存档文件写入 `Saved/SaveGames/`(已忽略,不提交)
- 本工程为个人学习 / Demo 用途

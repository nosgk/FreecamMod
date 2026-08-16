# 艾尔登法环 自由视角模组（Freecam Mod）

> **中文汉化分支声明**
> 本仓库是 [Logersnamed/FreecamMod](https://github.com/Logersnamed/FreecamMod) 的**中文汉化分支**，仅在其基础上将模组图形界面（GUI）与文档翻译为简体中文，不改变原有功能逻辑。
> 汉化内容可能滞后于上游版本，如需最新功能请关注原仓库。向上游反馈问题请使用英文并提交至[原仓库 Issues](https://github.com/Logersnamed/FreecamMod/issues)。

一个在《艾尔登法环》中将摄像机与玩家角色分离的[模组](https://www.nexusmods.com/eldenring/mods/9420)。在自由视角模式下会暂时冻结玩家。
兼容 Seamless Coop（无缝联机）、ER Reforged。已在 EldenModLoader、modengine2、me3 上测试通过。

![Freecam 预览](https://github.com/user-attachments/assets/9b9569fb-8401-4d98-a025-8c6dc6ec98fa)

<img width="1920" height="1080" alt="2086FD~1" src="https://github.com/user-attachments/assets/cd09f0ce-e9cb-44be-943c-8df30cf57fe9" />

## 功能特性
- 独立冻结游戏、玩家与实体
- 更改天气 / 时间
- 传送至摄像机位置
- 基于关键帧的摄像机动画时间轴窗口
- 逐帧步进器
- 保存 / 读取状态，以及状态间的插值过渡
- 路径记录器
- 变速加速（Speedhack）
- 可调节视场角（FOV）
- 自动隐藏 HUD
- 禁用 / 启用玩家操作
- 支持手柄
- 通过配置文件自定义按键与设置（仅键盘）

## 操作方式
所有按键均可在 `config.ini` 中查看和修改。
全部可用按键的完整列表请查阅[文档（Wiki）](https://github.com/Logersnamed/FreecamMod/wiki)。
- **F1** – 开关自由视角
- **W / A / S / D** – 移动摄像机
- **Shift / 空格** – 升高 / 降低
- **鼠标滚轮** – 调节摄像机移动速度
- **Ctrl + 鼠标滚轮（或 + / -）** – 调节视场角（FOV）
- **鼠标左键** – 加速移动

## 安装步骤
1. 使用 [Anti-Cheat Toggler](https://www.nexusmods.com/eldenring/mods/90) 等工具关闭 Easy Anti-Cheat（EAC）。
2. 安装任意一款 DLL 模组加载器（例如 [EldenModLoader](https://www.nexusmods.com/eldenring/mods/117)、[me3](https://github.com/garyttierney/me3)）。
3. [下载](https://github.com/Logersnamed/FreecamMod/releases)最新版本，并解压 **Freecam.zip** 中的内容：
   - **EldenModLoader**：放入 `...\steamapps\common\ELDEN RING\Game\mods`
   - **me2 / me3**：在对应的 profile 配置文件中指定路径。
   - 其他加载器请参阅其各自的文档。
4. 首次运行游戏后，会在模组 DLL 旁生成 `Freecam` 配置文件夹。请将汉化分支附带的 `zh-CN.json`（中文界面映射表）与 `AlibabaHealthFont2.0CN-85B.ttf`（中文字体）放入该文件夹，重启游戏后界面即为中文。
   - 若缺少字体文件，将自动回退系统微软雅黑；若两者均缺失，界面保持英文，不影响模组运行。
5. 启动游戏。

## 编译
### 使用 CMake
```bash
git clone --recurse-submodule https://github.com/Logersnamed/FreecamMod.git
cd FreecamMod
```
配置项目。可以选择通过 DGAME_DIR 变量指定 DLL 输出目录：
```bash
cmake -S . -B build -G "Visual Studio 17 2022" [-DGAME_DIR="path/to/modflolder/"]
```
构建项目：
```bash
cmake --build build --config Release
```
编译生成的 DLL 位于：`build/Release/FreecamMod.dll`

## 致谢与参考
[EROverlay](https://github.com/koalabear420/EROverlay) – 参考并使用了部分代码
[EldenRing-PostureBarMod](https://github.com/Mordrog/EldenRing-PostureBarMod) – 参考
[DX12 ImGui Overlay](https://github.com/kacejot/dx12-imgui-overlay) – 参考
[Techiew ModUtils](https://github.com/techiew/EldenRingMods/blob/master/ModUtils.h) - 艾尔登法环模组工具库
[Techiew EldenRingMods](https://github.com/techiew/EldenRingMods) - 参考并使用了部分代码
[The Grand Archives](https://github.com/The-Grand-Archives/Elden-Ring-CT-TGA) - 联机修改表
[Elden Ring Ultimate Cheat Engine Table](https://www.nexusmods.com/eldenring/mods/48) - 联机修改表
[Universal-WndProc-Hook](https://github.com/M0rtale/Universal-WndProc-Hook) – WndProc 钩子库

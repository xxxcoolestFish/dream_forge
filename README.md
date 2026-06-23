<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-20-blue" alt="C++20">
  <img src="https://img.shields.io/badge/OpenGL-3.3-orange" alt="OpenGL 3.3">
  <img src="https://img.shields.io/badge/Lua-5.4-blueviolet" alt="Lua 5.4">
  <img src="https://img.shields.io/badge/platform-Windows-lightgrey" alt="Windows">
  <img src="https://img.shields.io/badge/license-MIT-green" alt="MIT">
</p>

# DreamForge

> LLM 驱动叙事 × AI 图像 → 可走入的 3D 世界

DreamForge 是一个 AI 驱动的互动叙事游戏引擎。它接收 LLM 生成的**故事线**和**图片**，通过神经网络提取图层，在 3D 透视空间中重建为**可走入、可交互**的场景——让 AI 不仅"讲故事"，更能"构建世界"。

```
LLM 输出 → 剧情 + 对话 + 场景图像
   ↓
神经网络图层提取 + 深度估计
   ↓
3D 透视空间重建（层间可穿行）
   ↓
玩家第一人称走入 → 与 NPC 对话 → 完成任务 → 剧情分支
```

---

## 特性

- **LLM 驱动叙事**：对话树 + 任务系统 + 剧情分支，全部可通过 AI 实时生成
- **3D 透视场景**：第一人称视角，透视投影、深度测试，近大远小
- **图层系统**：AI 图像自动分层 → 3D 空间排列，玩家可在层间穿行
- **ECS 架构**：基于 EnTT，12 种组件类型，组合优于继承
- **Lua 脚本**：sol2 绑定全部引擎 API，JSON + Lua 即可编写完整游戏
- **后处理特效**：Bloom 泛光、Vignette 暗角、特效链可扩展
- **粒子系统**：GPU 点精灵，萤火虫/尘埃/火花预设
- **ImGui 编辑器**：F1 切换，实体列表 + 属性检视器
- **音频引擎**：SoLoud/WinMM，BGM + SFX + 环境音

## 快速开始

### 构建

```bash
# 需要 Visual Studio 2022 + CMake 3.27+
cmake -B build
cmake --build build --config Debug
```

### 运行

```bash
./build/bin/Debug/sample_game.exe
```

### 操作

| 按键 | 功能 |
|------|------|
| WASD | 移动（相对视角方向） |
| 鼠标 | 旋转视角 |
| 空格 | 跳跃 |
| E | 与 NPC 对话 |
| 方向键 | 自由飞行浏览 |
| F1 | ImGui 编辑器 |
| ESC | 切换光标捕获 |

## 架构

```
DreamForge/
├── engine/               # C++20 引擎（静态库）
│   ├── core/             # 生命周期、游戏循环、事件总线
│   ├── ecs/              # EnTT 封装 + 系统
│   ├── render/           # OpenGL 3.3 渲染
│   │   ├── scene/        # 3D 透视相机 + 场景渲染器
│   │   ├── effects/      # 后处理特效链
│   │   ├── particles/    # 粒子系统
│   │   └── transitions/  # 屏幕转场
│   ├── input/            # 键盘/鼠标输入
│   ├── ui/               # 数据驱动 UI（FlexLayout）
│   ├── ai/               # ZeroMQ AI 客户端
│   ├── script/           # Lua 5.4 + sol2 绑定
│   ├── narrative/        # 对话树、任务、StoryFlags
│   ├── audio/            # SoLoud 音频引擎
│   └── tools/            # ImGui 编辑器
├── assets/               # 游戏资源
│   ├── scenes/           # .scene 场景文件
│   ├── dialogues/        # .json 对话树
│   ├── quests/           # .json 任务定义
│   ├── ui/               # .json UI 布局
│   └── scripts/          # .lua 脚本
├── sample/               # 示例游戏
└── docs/                 # 文档
```

## Lua API

```lua
-- 音频
engine.playSound("sfx/click.wav")
engine.playMusic("bgm/theme.ogg", 0.7, true)

-- 粒子
engine.spawnParticles("fireflies", 400, 300)

-- 后处理
engine.setBloom(true, 0.5)
engine.setVignette(true, 0.6)

-- 叙事
narrative.setFlag("chapter", 2)
narrative.startQuest("blacksmith_quest")
```

## 路线图

- [x] Phase 1: 基础框架（窗口、渲染、ECS）
- [x] Phase 2: AI 集成（ZeroMQ IPC + LLM Provider）
- [x] Phase 3: 场景管线（2.5D 视差渲染）
- [x] Phase 4: UI 系统（数据驱动控件树）
- [x] Phase 5: 叙事系统（Lua + 对话树 + 任务）
- [x] Phase 6: 工具打磨（音频、特效、粒子、编辑器）
- [x] Phase 7: 第一人称 3D 透视渲染
- [ ] Phase 8: 神经网络图层提取管线
- [ ] Phase 9: LLM 实时世界生成
- [ ] Phase 10: 完整示例游戏

## 技术栈

C++20 · EnTT · GLM · OpenGL 3.3 · GLFW · Lua 5.4 + sol2 · SoLoud · Dear ImGui · stb_image · spdlog · nlohmann/json · ZeroMQ

## License

MIT

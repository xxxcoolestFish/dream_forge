# AI Game Frame — 开发者指南

## 快速开始

```bash
# 1. 构建
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Debug

# 2. 运行
./build/bin/Debug/sample_game.exe

# 3. 打包
cmake --build build --target package
```

## 项目结构

```
AI_game_frame/
├── engine/           # C++ 引擎源码（静态库 engine.lib）
│   ├── core/         # 引擎初始化、游戏循环、事件总线
│   ├── ecs/          # EnTT 封装 + 系统
│   ├── render/       # OpenGL 3.3 渲染后端
│   ├── input/        # 输入系统
│   ├── ui/           # 数据驱动 UI 系统
│   ├── ai/           # ZeroMQ AI 客户端
│   ├── script/       # Lua 绑定（sol2）
│   ├── narrative/    # 叙事系统（对话树、任务、标记、条件）
│   ├── audio/        # SoLoud 音频引擎
│   ├── tools/        # ImGui 编辑器
│   └── render/effects/   # 后处理特效
│       particles/    # 粒子系统
│       transitions/  # 屏幕转场
├── sample/           # 示例游戏
├── assets/           # 游戏资源
│   ├── scenes/       # .scene 场景文件
│   ├── dialogues/    # .json 对话树
│   ├── quests/       # .json 任务定义
│   ├── ui/           # .json UI 布局
│   └── scripts/      # .lua 脚本
└── docs/             # 文档
```

## 引擎核心

### 初始化流程

```cpp
engine::EngineConfig config;
config.windowTitle  = "My Game";
config.windowWidth  = 1280;
config.windowHeight = 720;

engine::Engine engine;
engine.init(config);

// 创建实体
auto* world = engine.ecsWorld();
auto entity = world->createEntity();
world->addComponent<engine::ecs::Transform>(entity);
// ...

// 加载资源
engine.loadScene("assets/scenes/my_scene.scene");
engine.loadUI("assets/ui/my_hud.json");
engine.loadScript("assets/scripts/game.lua");

engine.run();
engine.shutdown();
```

### 主循环顺序

```
glfwPollEvents → input.poll
  → Editor.update (F1 toggle)
  → scriptEngine.onUpdate(dt)
  → questManager.update(dt)
  → particleSystem.update(dt)
  → transitionManager.update(dt)
  → audioEngine.update(dt)
  → world.updateSystems(dt)    // Movement → Interaction → Dialogue
  → dialogueUI.update(input)

  → render:
    bind FBO → clear
    → sceneRenderer.render() → FBO
    → spriteRenderer.render(world) → FBO
    → particleSystem.render() → FBO
    unbind FBO → clear backbuffer
    → effectChain.process(FBO texture)
    → uiRenderer.render()
    → editor.render() (if active)
    → transitionManager.render() (if active)
    → glfwSwapBuffers
```

## Lua API 参考

### 引擎 API（`engine` 表）

```lua
-- 音频
engine.playSound("assets/audio/click.wav", 1.0)  -- 播放音效
engine.playMusic("assets/audio/bgm.ogg", 0.7, true) -- 背景音乐
engine.stopMusic()
engine.setMasterVolume(0.8)

-- 粒子
engine.spawnParticles("fireflies", 400, 300)  -- 预设: fireflies/dust/sparkles

-- 后处理
engine.setBloom(true, 0.5)      -- 开启泛光，强度 0.5
engine.setBloom(false)          -- 关闭泛光
engine.setVignette(true, 0.6)   -- 开启暗角

-- 实体操作
engine.createEntity()           -- 创建实体，返回 ID
engine.getTransform(entityId)   -- 返回 {x, y, z}
engine.isKeyPressed(key)        -- 检查按键 (Keys.W/E/etc.)
```

### 叙事 API（`narrative` 表）

```lua
narrative.setFlag("chapter", 2)
narrative.getFlag("met_blacksmith")  -- 返回 bool
narrative.hasFlag("some_flag")
narrative.startQuest("quest_id")
```

## ECS 组件

| 组件 | 字段 |
|------|------|
| `Transform` | position(vec3), rotation(vec3), scale(vec3), depthLayer(float) |
| `Sprite` | tint(vec4), textureId(uint32), visible(bool) |
| `Stats` | values(map), maxValues(map) |
| `Interactive` | interactionType(string), hitPolygon |
| `DialogueSpeaker` | characterId, personalityPrompt |
| `DialogueState` | dialogueTreeId, currentNodeId, visitedNodes |
| `QuestProgress` | questId, status, objectives |
| `Player` | (tag, 无字段) |
| `ParticleEmitter` | presetName, active, emissionRate |

## 对话树 JSON 格式

```json
{
  "treeId": "npc_intro",
  "startNodeId": 1,
  "nodes": {
    "1": { "type": "condition", "condition": "flag:met_npc", "trueNode": 5, "falseNode": 2 },
    "2": { "type": "text", "speaker": "NPC", "text": "你好！", "next": 3 },
    "3": { "type": "choice", "options": [
      { "text": "选项A", "next": 4 },
      { "text": "选项B", "next": 6 }
    ]},
    "4": { "type": "set_flag", "flag": "met_npc", "value": true, "next": 5 },
    "5": { "type": "text", "speaker": "NPC", "text": "又见面了。", "next": "END" },
    "6": { "type": "ai_fallback", "context": { "npc": "npc_id" } }
  }
}
```

节点类型：`text`, `choice`, `condition`, `set_flag`, `ai_fallback`, `END`

## 渲染架构

- **OpenGL 3.3 Core Profile**，手动加载 GL 函数
- **纹理采样**：SpriteRenderer shader 支持 `uUseTexture` 开关
- **FBO**：场景渲染到 `RenderTarget` → 后处理特效链 → 全屏四边形 → UI 叠加
- **特效链**：Bloom（亮部提取+模糊+合成）→ Vignette（径向暗角）
- **粒子**：CPU 更新 + `GL_POINTS` 圆形点精灵（加法混合）
- **转场**：全屏着色四边形 FadeIn/FadeOut

## 快捷键

| 按键 | 功能 |
|------|------|
| WASD | 移动玩家 |
| 方向键 | 旋转相机（2.5D 视差） |
| E | 与 NPC 交互 |
| F1 | 切换 ImGui 编辑器 |
| ESC | 退出 |

## 编辑器

F1 打开 ImGui 编辑器，提供：
- **Entity List**：列出所有实体
- **Inspector**：查看/编辑 Transform、Sprite tint、Stats
- 编辑器模式下输入不传递给游戏

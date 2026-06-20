# AI驱动互动叙事游戏框架 — 架构设计

## Context

开发者希望构建一个桌面端游戏框架，核心创新在于：**AI生成的2D图像经过自动分割与深度估计后，被解析为分层可交互的2.5D游戏场景**。框架需要维护完整的游戏逻辑状态（物品、数值、关系、任务等），同时通过LLM驱动剧情、对话和支线任务的实时生成。

这是一个全新的项目（空目录）。第一阶段交付：**完整架构设计**。

### 用户确认的技术方向
- **平台**: 桌面端 (Windows/Mac)
- **图像生成**: API调用云端模型 (DALL-E / Stable Diffusion)
- **图像分割**: 本地GPU加速的轻量化模型 (ONNX Runtime)，**支持后续替换为自行训练的模型**
- **LLM**: 多模型支持，框架设计为模型无关
- **当前阶段**: 先做完整架构设计

---

## 一、总体架构

```
+------------------------------------------------------------------+
|                    开发者工具层 (World Editor, Scene Composer)     |
+------------------------------------------------------------------+
|                    脚本层 (Lua 5.4 — 游戏逻辑、对话树、任务)      |
+------------------------------------------------------------------+
|                    AI服务层 (Python 3.11 — 独立进程)               |
|  [LLM Gateway]  [Image Gen API]  [ONNX Segmentation]  [Prompt Mgr]|
+-------------------------- IPC (ZeroMQ) ---------------------------+
|                    核心引擎层 (C++20)                              |
|  [ECS World]  [Render Engine(2.5D)]  [UI System]  [Input]  [Audio]|
+------------------------------------------------------------------+
|                    平台抽象层 (bgfx, GLFW)                         |
+------------------------------------------------------------------+
|                    硬件/OS (Win10+, macOS12+, GPU)                 |
+------------------------------------------------------------------+
```

### 核心设计原则
- **数据驱动**: 所有行为定义在数据文件中，不硬编码
- **组合优于继承**: ECS架构，无深层类继承
- **关注点分离**: 逻辑(ECS)、渲染、AI通过事件总线解耦
- **异步优先**: AI操作(LLM/图像生成)天然慢，异步执行，渲染循环不受阻塞
- **模块可替换**: LLM提供商、UI控件、分割模型、渲染后端皆可插拔

---

## 二、模块架构

### 2.1 Logic Core (`logic/`) — 逻辑信息维护

**职责**: 游戏状态的唯一真相源。维护除画面/UI以外的所有游戏数据。

**内部结构**:
```
logic/
  ecs/
    world.h/cpp          — ECS World (基于EnTT)
    entity.h             — Entity = uint64_t
    component_types.h    — 所有组件的POD结构定义
    system.h/cpp         — System基类 + 系统依赖图
  systems/
    stat_system.cpp      — HP/XP/属性计算
    inventory_system.cpp — 物品拾取/装备/使用
    dialogue_system.cpp  — 对话状态机
    quest_system.cpp     — 任务追踪
    relationship_system.cpp — NPC关系图
    time_system.cpp      — 游戏内时间
    interaction_system.cpp  — 点击交互解析
  state/
    game_state.h/cpp     — 序列化/存档/状态差异
```

**核心组件目录**:

| 组件 | 说明 |
|------|------|
| `Transform` | 位置、旋转、缩放、深度层级 |
| `Sprite` | 纹理引用、色调、锚点、层级组 |
| `Interactive` | 点击检测多边形、交互类型、光标样式 |
| `Stats` | 键值对属性 (hp, xp, mp...) |
| `Inventory` | 物品Entity列表、最大槽位 |
| `Item` | 物品ID、名称、描述、装备槽、属性修正 |
| `DialogueSpeaker` | 角色ID、性格prompt、友好度、记忆标签 |
| `DialogueState` | 对话树ID、当前节点、访问历史、标记位 |
| `QuestProgress` | 任务ID、状态机、目标进度 |
| `Relationship` | NPC_ID → 好感度 (-1.0 ~ 1.0) |
| `AICharacterContext` | 背景故事、当前目标、情绪状态、知识域 |

### 2.2 Render Engine (`render/`) — 视觉效果

**职责**: 所有视觉输出 — 2.5D视差场景渲染 + UI渲染。

```
render/
  backend/
    render_backend.h     — 抽象渲染后端
    bgfx_backend.cpp     — bgfx实现 (DX11/Metal/Vulkan)
  scene/
    camera.h/cpp         — 2.5D相机 (水平/垂直旋转限制 ±15°/±5°)
    layer.h/cpp          — 视差层 (纹理、深度值、变换)
    scene.h/cpp          — 场景 = 有序层集合
    scene_renderer.cpp   — 深度排序 + 视差变换 + 绘制
  ui/
    widget.h/cpp         — 控件基类、控件树
    layout_engine.cpp    — Flexbox布局引擎
    theme.h/cpp          — 主题定义 (颜色/字体/度量/纹理)
    widgets/             — 内建控件 (Button, Text, Panel, Slider, ScrollView...)
    event_system.cpp     — UI事件分发
```

**2.5D视差渲染原理**:
- 每个场景层是一个带纹理的四边形(2三角形)
- 相机围绕场景中心旋转时，不同深度层产生不同的偏移量（视差因子）
- 深度因子 0.0(远景天空) ~ 1.0(前景)，近景偏移大、远景偏移小
- **不是真正的3D**：无Z-buffer、无透视投影矩阵，但偏移计算产生令人信服的深度错觉
- 相机旋转限制：水平±15°、垂直±5°

**渲染通道顺序**:
1. Background Plate (修复后的背景底图)
2. Layers (深度排序，远→近)
3. Foreground Effects (光效、粒子)
4. UI Overlay (对话框、HUD、光标)

### 2.3 AI Service (`ai_service/`) — AI集成层

**语言**: Python 3.11+ (ONNX Runtime / httpx / Pillow生态)

```
ai_service/
  llm/
    provider.py          — LLMProvider抽象基类
    claude_provider.py   — Anthropic SDK
    openai_provider.py   — OpenAI SDK
    ollama_provider.py   — 本地Ollama
    provider_registry.py — 动态发现
    context_builder.py   — 游戏状态 → LLM prompt (Jinja2模板)
    response_parser.py   — 结构化输出提取 (JSON mode + 回退)
    history_manager.py   — 对话历史 + token预算管理
  image_gen/
    base.py              — ImageGenProvider抽象基类
    dalle_provider.py    — DALL-E API
    sdxl_provider.py     — Stable Diffusion API
  segmentation/
    onnx_engine.py       — ONNX Runtime GPU推理
    sam_model.py         — SAM2 ONNX模型 (可替换为自训练模型)
    depth_estimator.py   — Depth-Anything-V2 深度估计
    mask_processor.py    — 遮罩→多边形简化、层提取
    entity_detector.py   — 交互对象识别
  gateway/
    ai_gateway.py        — 统一入口，协调LLM+图像+分割
    ipc_server.py        — ZeroMQ REP，接收C++引擎请求
```

**LLM Provider抽象接口**:
```python
class LLMProvider(ABC):
    async def complete(messages, config) -> LLMResponse
    async def complete_stream(messages, config) -> AsyncIterator[str]
    def count_tokens(text) -> int
    def max_context_tokens() -> int
```
通过ProviderRegistry，开发者只需修改配置文件即可切换Claude/OpenAI/Ollama，无需改动引擎代码。

**图像分割模型说明**:
- 默认集成SAM2-ONNX + Depth-Anything-V2-ONNX
- ONNX Runtime支持GPU加速 (DirectML on Windows, CoreML on Mac)
- **关键**: segmentation模块设计为模型无关，开发者可通过ONNX Runtime加载自己训练的轻量化模型进行替换
- 分割在开发阶段离线运行，不影响游戏运行时性能

### 2.4 Script Engine (`script/`)

**语言**: Lua 5.4 + sol2 (C++绑定)

开发者用Lua编写游戏逻辑。引擎通过sol2将ECS操作、渲染控制、UI操作、输入等暴露给Lua。

### 2.5 Input System + Resource Manager

- **Input**: 原始输入 → 动作映射 → 事件总线。支持按键重绑定。
- **Resource**: 异步加载、LRU缓存、生命周期管理（纹理/场景/脚本/音频/字体）。

---

## 三、核心数据流

### 3.1 帧级运行循环
```
Frame Start
  → [Input] 轮询输入
  → [Script] 执行活跃脚本
  → [ECS] 运行所有System (stat/inventory/dialogue/quest...)
  → [AI Service] 非阻塞轮询AI结果
  → [Render] begin → 场景渲染(视差) → UI渲染 → end
Frame End (目标60fps, 16.67ms预算)
```

### 3.2 AI对话交互流程
```
玩家点击NPC
  → [Input] ClickEvent
  → [InteractionSystem] 射线检测 → 找到最近可交互Entity
  → [Lua Script] 读取对话状态，调用AI Service
  → [ContextBuilder] 序列化游戏状态 → Jinja2模板 → LLM消息
  → [LLM Provider] 流式生成
  → [ResponseParser] 提取结构化对话JSON
  → [Lua Script] 更新ECS状态 → 触发ShowDialogueEvent
  → [UI] 显示对话文本/选项
```

### 3.3 图像→场景管线
```
开发者描述场景
  → [Image Gen API] 生成图像
  → [ONNX SAM2] 自动分割 → 遮罩列表 (15-50个)
  → [ONNX Depth] 深度估计 → 每像素深度图
  → [Mask-Depth关联] 每个遮罩赋予深度值
  → [Layer Extraction] 提取每层PNG + 背景修复(Inpainting)
  → [Entity Classification] 启发式分类 character/furniture/item/door...
  → [Interactive Boundary] 生成交互多边形 (扩展10%便于点击)
  → [Scene Composition] 打包为 .scene 文件
  → [Developer Refinement] 手动调整分层/交互区域/深度
```

---

## 四、UI自定义系统

### 设计理念：完全数据驱动

UI不是写在C++代码中的，而是通过JSON定义布局+主题+Lua行为脚本。

**三层架构**:
1. **数据层**: JSON定义控件树 + 主题 + 数据绑定
2. **逻辑层**: C++控件树构建 + Flexbox布局 + 状态管理 + 事件分发
3. **渲染层**: 控件绘制 + SDF字体 + 九宫格图片 + 裁剪

**数据绑定支持**:
- `ecs`: 监听ECS组件字段变化
- `event`: 监听特定事件
- `script`: 每帧执行Lua表达式
- `static`: 静态常量
- `loc`: 本地化字符串

**内建控件**: Panel, Text, Button, Image, Slider, Input, ScrollView, Toggle, Dropdown, ProgressBar, Tooltip, Custom(Lua回调)

---

## 五、游戏状态 → LLM上下文 (Context Assembly)

由于LLM上下文窗口有限，采用**分层上下文系统**：

| 层级 | 内容 | Token预算 |
|------|------|-----------|
| Tier 0 | 世界规则 (世界观描述、叙事约束) | ~500, 始终包含 |
| Tier 1 | 当前场景 (位置、在场角色、氛围) | ~300, 始终包含 |
| Tier 2 | 交互目标 (NPC角色设定、关系、目标) | ~200, 始终包含 |
| Tier 3 | 玩家状态 (属性、相关物品、活跃任务) | ~200, 始终包含 |
| Tier 4 | 相关历史 (最近对话、过往交互摘要) | ~1000, 选择性包含 |
| Tier 5 | 世界知识 (RAG检索的相关Lore) | ~500, 按需包含 |

通过Jinja2模板系统，不同场景(对话/任务生成/剧情分支)使用不同的context模板。

---

## 六、技术栈

| 层次 | 技术 | 理由 |
|------|------|------|
| **核心语言** | C++20 | 60fps确定性帧预算，无GC暂停 |
| **构建系统** | CMake 3.27+ | 跨平台标准 |
| **渲染后端** | bgfx | 跨平台(DX11/12, Metal, Vulkan)，Shader交叉编译 |
| **ECS** | EnTT 3.13+ | 最快C++ ECS，Header-only，零开销 |
| **数学库** | GLM | GLSL兼容，业界标准 |
| **窗口/输入** | GLFW 3.4+ | 跨平台窗口创建 |
| **音频** | SoLoud | 跨平台，简易API |
| **AI服务** | Python 3.11+ | ML生态(PyTorch/ONNX)的事实标准 |
| **C++↔Python IPC** | ZeroMQ | 轻量高性能消息传递 |
| **ONNX Runtime** | onnxruntime-gpu | GPU加速推理，支持DirectML/CoreML |
| **分割模型** | SAM2-ONNX (可替换) | 支持开发者自行训练/替换模型 |
| **深度估计** | Depth-Anything-V2-ONNX | SOTA单目深度估计 |
| **LLM SDK** | anthropic / openai | 官方异步SDK |
| **提示词模板** | Jinja2 3.x | 灵活的prompt工程 |
| **脚本语言** | Lua 5.4 + sol2 | 轻量嵌入式，对业余开发者友好 |
| **序列化** | nlohmann/json | 所有资源和IPC消息格式 |
| **日志** | spdlog | 高性能异步日志 |
| **UI字体** | msdf-atlas-gen | SDF字体图集，任意缩放清晰 |

---

## 七、开发者项目结构

```
my_game/
  game.json                 — 游戏清单 (标题、入口场景、分辨率、LLM配置)
  world/                    — 世界观定义
    world_rules.json, factions.json, timeline.json
  characters/
    player.json, npcs/*.json
  scenes/
    tavern/tavern.scene, layers/*.png, ambiance.ogg
  dialogue_trees/*.dialogue
  quests/*.quest
  ui/
    layouts/*.json, themes/*.theme
  prompts/                  — Jinja2提示词模板
    dialogue_system.j2, quest_generation.j2...
  scripts/                  — Lua游戏逻辑
    main.lua, tavern_events.lua
  config/
    providers.json, keybindings.json
```

---

## 八、分阶段实现路线图

### Phase 1: 基础框架 (预计2个月)
- CMake项目搭建 + 依赖集成 (bgfx, GLFW, EnTT, GLM, spdlog, nlohmann)
- 渲染后端：bgfx初始化、纹理加载、精灵批绘制
- ECS核心：World/Entity/Component/System
- 输入与窗口：GLFW窗口 + 键鼠输入 + 动作映射
- 游戏循环：固定时间步逻辑 + 可变时间步渲染

### Phase 2: AI集成 (预计2个月)
- Python AI Service进程 + ZeroMQ IPC桥接
- LLM集成：Provider接口 → Claude/OpenAI实现 → Context Builder → Response Parser
- 图像生成API：DALL-E / SD API集成
- ONNX分割管道：SAM2 + Depth-Anything-V2 + 遮罩处理

### Phase 3: 场景管线 (预计2个月)
- 场景数据模型 (.scene格式)
- 2.5D视差渲染器：相机模型、视差变换、深度排序渲染
- 交互系统：多边形碰撞检测、悬停/点击、视觉反馈
- Scene Composer工具：可视化层编辑器

### Phase 4: UI系统 (预计2个月)
- UI核心：控件树、Flexbox布局、脏标记、事件分发
- 内建控件完整实现
- 主题系统：.theme文件、样式类、SDF字体、九宫格
- 数据绑定系统

### Phase 5: 叙事系统 (预计2个月)
- 对话系统：对话树 + AI对话生成 + 对话UI
- 任务系统：任务定义 + 状态机 + 目标追踪 + AI任务生成
- 剧情分支：全局标记系统 + 分支条件 + 结果追踪
- Lua脚本集成：sol2绑定所有引擎系统

### Phase 6: 工具与打磨 (预计3个月)
- World Editor可视化编辑器
- 打包分发 (CPack)
- 后期特效、粒子系统、转场效果
- 音频集成
- 文档 + 示例游戏

### 依赖关系
```
Phase 1 (Foundation)
  ├→ Phase 2 (AI Integration) ─→ Phase 3 (Scene Pipeline) ─→ Phase 5 (Narrative)
  ├→ Phase 4 (UI System) ──────────────────────────────────→ Phase 5 ─→ Phase 6
  └→ Phase 2 + Phase 4 可部分并行
```

---

## 九、风险与缓解

| 风险 | 严重度 | 缓解措施 |
|------|--------|----------|
| ONNX分割在低端GPU速度慢 | 高 | 模型级联(轻量→重量)，支持CPU回退 |
| LLM叙事长对话不连贯 | 高 | 分层上下文+周期性摘要注入+世界约束prompt |
| LLM API延迟打断游戏体验 | 中 | 流式输出，预生成常见变体，高频回复缓存 |
| LLM API费用高 | 中 | 回复缓存，小模型处理常规交互，费用上限 |
| 2.5D视差效果不理想 | 中 | 保守旋转限制(5°起步)，预验证再扩展 |
| C++/Python IPC复杂 | 中 | ZeroMQ封装序列化；编辑器模式可嵌入Python |

---

## 十、关键架构决策汇总

| 决策 | 选择 | 理由 |
|------|------|------|
| 逻辑架构 | ECS (EnTT) | 组合优于继承，支持异构游戏实体 |
| 渲染方式 | 深度排序精灵 + 透视偏移视差 | 保留AI画质，增加深度感，无需全3D |
| 核心语言 | C++20 | 60fps确定性帧预算 |
| AI语言 | Python 3.11+ | ML生态原生支持 |
| C++/Python桥接 | ZeroMQ IPC | 干净进程隔离，AI延迟不阻塞渲染 |
| UI方案 | 数据驱动控件树 + Flexbox | 无需代码即可完整自定义 |
| LLM抽象 | Provider接口 + Registry | 切换模型零引擎改动 |
| 上下文组装 | 分层上下文 + Token预算 | 适配任何模型的上下文窗口 |
| 脚本 | Lua 5.4 via sol2 | 小巧快速，为嵌入而设计 |

---

## 验证方式

1. 每一Phase结束时的里程碑验证：可编译运行的最小可工作原型
2. Phase 1完成后：窗口启动 → 渲染测试精灵 → 响应输入
3. Phase 2完成后：LLM对话 → AI图像生成 → 分割管道输出可视化
4. Phase 3完成后：2.5D场景渲染 → 鼠标点击实体交互
5. Phase 6完成后：完整的示例游戏 (小规模奇幻叙事)

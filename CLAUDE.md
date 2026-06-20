# AI驱动互动叙事游戏框架 — 项目初始化文档

## 项目概述

构建一个桌面端（Windows优先）游戏框架，核心创新：
- **AI生成的2D图像** → 自动分割 + 深度估计 → 分层可交互的2.5D场景
- **LLM驱动**剧情、对话、支线任务的实时生成
- **ECS架构**维护完整游戏逻辑状态（物品、数值、关系、任务）
- **数据驱动UI** — JSON定义布局 + Lua定义行为

## 技术栈

| 层 | 技术 | 备注 |
|---|------|------|
| 核心 | C++20 | MSVC 2022+ |
| 构建 | CMake 3.27+ | Windows + Visual Studio |
| 渲染 | bgfx | DX11后端 |
| ECS | EnTT 3.13+ | header-only |
| 数学 | GLM | header-only |
| 窗口 | GLFW 3.4+ | Windows |
| 脚本 | Lua 5.4 + sol2 | Phase 5集成 |
| AI服务 | Python 3.11+ | Phase 2集成 |
| IPC | ZeroMQ | C++ ↔ Python |
| 序列化 | nlohmann/json | header-only |
| 日志 | spdlog | header-only |

## 代码规范 — 最高优先级

### 0. 写代码前的思考流程（强制执行）

**在写任何代码之前，必须先思考并输出以下内容：**

1. **目的**：这部分代码要解决什么问题？在整个架构中处于什么位置？
2. **主要内容**：会包含哪些类/函数/文件？对外暴露什么接口？
3. **依赖关系**：这部分代码依赖哪些已有模块？会被哪些后续模块依赖？
4. **边界条件**：有哪些需要处理的错误情况？空指针、资源加载失败、初始化顺序等
5. **设计取舍**：为什么选择这个方案而不是其他方案？有哪些已知的局限性？

**没有经过上述思考，不允许开始写代码。**

### 1. 质量优先原则

- 这是一个**长期开发项目**，不赶时间，把每一行代码做好做精
- 宁可花时间把接口设计好，也不要为了进度匆忙实现
- 每个模块在写完之后应该具备**独立可编译、独立可验证**的特性
- 注释使用中文，代码命名使用英文

### 2. 命名规范

```
类名/结构体:   PascalCase    (例: RenderBackend, ComponentManager)
函数/方法:     camelCase     (例: createEntity(), loadTexture())
成员变量:      m_camelCase   (例: m_windowWidth, m_isRunning)
常量/枚举值:   UPPER_SNAKE   (例: MAX_ENTITIES, LAYER_BACKGROUND)
命名空间:      snake_case    (例: engine::render, engine::ecs)
文件名:        snake_case    (例: render_backend.h, stat_system.cpp)
```

### 3. 代码组织规范

- 一个 `.h` + 一个 `.cpp` = 一个核心抽象，禁止把多个不相关的类塞进一个文件
- 头文件使用 `#pragma once`
- 所有引擎代码在 `namespace engine` 下，子模块用嵌套命名空间
- `.h` 中只暴露公开接口，实现细节放 `.cpp`
- 每个 `.h` 文件包含清晰的文档注释（中文），说明：是什么、怎么用、注意事项

### 4. 错误处理

- 构造函数/析构函数中不抛出异常
- 资源加载使用 `std::optional` 或自定义 `Result<T, Error>` 返回
- 不可恢复的错误使用 `assert` 或 `spdlog::critical` + `std::abort`
- 每个可能失败的操作都要考虑失败路径

### 5. 平台与构建

- **当前仅考虑 Windows** (MSVC 2022+)
- CMake 中所有路径使用 `/`（CMake自动转换）
- bgfx 使用 DX11 后端
- 依赖管理优先使用 CMake FetchContent

## 项目目录结构

```
AI_game_frame/
├── CMakeLists.txt              # 根构建文件
├── CLAUDE.md                   # 本文件
├── toasty-dancing-ember.md     # 架构设计文档
├── engine/                     # 引擎源码
│   ├── CMakeLists.txt
│   ├── core/                   # 引擎初始化、游戏循环、事件总线
│   ├── ecs/                    # EnTT封装
│   ├── render/                 # bgfx渲染后端、精灵、纹理
│   ├── input/                  # 输入系统、动作映射
│   ├── resource/               # 异步资源加载、缓存
│   └── script/                 # Lua绑定（Phase 5）
├── ai_service/                 # Python AI服务（Phase 2）
├── assets/                     # 测试资源
│   ├── textures/
│   └── shaders/
├── sample/                     # 示例游戏
│   ├── CMakeLists.txt
│   └── main.cpp
└── docs/                       # 额外文档
```

## 开发工作流

### Git 规范

- **main** 分支：稳定版本
- 每个 Phase 在完成里程碑验证后提交
- 每个独立模块完成后可单独提交
- Commit message 格式：`[Phase N] 简短描述 — 详细说明`
- 示例：`[Phase 1] ECS核心封装完成 — World/Entity/Component/System基础接口`

### 实现流程

1. 阅读架构设计文档相关章节
2. **执行代码前思考流程**（见规范 0）
3. 设计接口（.h 文件）
4. 实现（.cpp 文件）
5. 确保编译通过
6. 下一模块

### Phase 1 里程碑

Phase 1 完成标准：**窗口启动 → 渲染精灵 → 键盘控制精灵移动**

## 架构设计文档参考

详见 [toasty-dancing-ember.md](toasty-dancing-ember.md) — 这是所有实现工作的权威参考。

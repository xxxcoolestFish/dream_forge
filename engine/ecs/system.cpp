/**
 * @file engine/ecs/system.cpp
 * @brief System 基类实现（主要是虚析构函数的编译单元安置）
 */

#include "engine/ecs/system.h"

// System 基类的大部分方法在头文件中内联
// 此 .cpp 确保虚函数表有唯一的编译单元

/**
 * @file engine/audio/audio_engine.h
 * @brief 音频引擎 — 封装 SoLoud，提供 BGM + SFX + 环境音
 *
 * Phase 6.2：支持 WAV/OGG 音效、背景音乐循环、环境音切换、全局音量控制。
 *
 * 使用方式：
 *   auto audio = std::make_unique<AudioEngine>();
 *   audio->init();
 *   audio->playMusic("assets/audio/bgm.ogg");
 *   audio->playSound("assets/audio/click.wav");
 *
 * 设计：
 *   - PIMPL 隐藏 SoLoud 头文件，避免引擎其他模块依赖 SoLoud
 *   - 音乐通道独立，同一时间只有一首 BGM
 *   - 环境音复用音乐通道（切换时做淡入淡出）
 */

#pragma once

#include <cstdint>
#include <string>
#include <memory>

namespace engine::audio {

class AudioEngine
{
public:
    AudioEngine();
    ~AudioEngine();

    // 禁止拷贝
    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;

    // 初始化 / 关闭
    bool init();
    void shutdown();
    bool isInitialized() const;

    // 每帧更新（SoLoud 内部 3D 音频处理）
    void update(float dt);

    // --- 一次性音效（支持 WAV, OGG） ---
    // 返回音效句柄（可用于停止等操作，0 = 失败）
    uint32_t playSound(const std::string& path, float volume = 1.0f);

    // --- 背景音乐 ---
    void playMusic(const std::string& path, float volume = 0.7f, bool loop = true);
    void stopMusic();
    void pauseMusic();
    void resumeMusic();
    bool isMusicPlaying() const;

    // --- 环境音（与音乐同通道，支持淡入淡出切换） ---
    void playAmbiance(const std::string& path, float fadeInSec = 1.0f);
    void stopAmbiance(float fadeOutSec = 1.0f);

    // --- 全局音量控制（0.0 ~ 1.0） ---
    void setMasterVolume(float vol);
    void setSfxVolume(float vol);
    void setMusicVolume(float vol);
    float masterVolume() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace engine::audio

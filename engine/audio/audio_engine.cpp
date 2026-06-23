/**
 * @file engine/audio/audio_engine.cpp
 * @brief 音频引擎实现 — Phase 6.2：SoLoud 封装
 *
 * 后端：WinMM（Windows 多媒体，零外部依赖）
 * 格式：WAV（soloud_wav）、OGG（stb_vorbis 内嵌解码）
 */

#include "engine/audio/audio_engine.h"

#include <soloud.h>
#include <soloud_wav.h>

#include <spdlog/spdlog.h>
#include <algorithm>
#include <unordered_map>

namespace engine::audio {

// =========================================================================
// PIMPL 内部实现
// =========================================================================
struct AudioEngine::Impl
{
    SoLoud::Soloud soloud;
    SoLoud::Wav    currentMusic;      // 当前 BGM/环境音
    bool           musicLoaded = false;
    bool           musicPaused = false;
    uint32_t       musicHandle = 0;
    bool           initialized = false;

    float          masterVolume  = 1.0f;
    float          sfxVolume     = 1.0f;
    float          musicVolume   = 0.7f;

    // 已加载的音效缓存（路径 → Wav）
    std::unordered_map<std::string, std::unique_ptr<SoLoud::Wav>> sfxCache;
};

// =========================================================================
AudioEngine::AudioEngine()
    : m_impl(std::make_unique<Impl>())
{
    spdlog::debug("AudioEngine: instance created");
}

AudioEngine::~AudioEngine()
{
    if (isInitialized())
    {
        shutdown();
    }
}

bool AudioEngine::init()
{
    spdlog::info("AudioEngine: initializing SoLoud (WinMM backend)...");

    auto result = m_impl->soloud.init(
        SoLoud::Soloud::CLIP_ROUNDOFF,
        SoLoud::Soloud::WINMM);
    if (result != SoLoud::SOLOUD_ERRORS::SO_NO_ERROR)
    {
        spdlog::error("AudioEngine: SoLoud init failed, error code: {}", static_cast<int>(result));
        return false;
    }

    m_impl->soloud.setGlobalVolume(m_impl->masterVolume);
    m_impl->initialized = true;
    spdlog::info("AudioEngine: SoLoud initialized (backend=WinMM, sampleRate={})",
        m_impl->soloud.getBackendString());
    return true;
}

void AudioEngine::shutdown()
{
    spdlog::info("AudioEngine: shutting down...");

    // 停止所有音频
    m_impl->soloud.stopAll();

    // 释放音效缓存
    m_impl->sfxCache.clear();

    // 释放当前音乐
    if (m_impl->musicLoaded)
    {
        m_impl->currentMusic.stop();
        m_impl->musicLoaded = false;
    }

    m_impl->soloud.deinit();
    m_impl->initialized = false;
    spdlog::info("AudioEngine: shutdown complete.");
}

bool AudioEngine::isInitialized() const
{
    return m_impl->initialized;
}

void AudioEngine::update(float dt)
{
    (void)dt;
    // SoLoud 使用独立音频线程，不需要每帧驱动
    // dt 保留用于后续 3D 音频位置更新
}

// =========================================================================
// 音效
// =========================================================================
uint32_t AudioEngine::playSound(const std::string& path, float volume)
{
    // 查找或加载音效
    auto it = m_impl->sfxCache.find(path);
    if (it == m_impl->sfxCache.end())
    {
        auto wav = std::make_unique<SoLoud::Wav>();
        auto result = wav->load(path.c_str());
        if (result != SoLoud::SOLOUD_ERRORS::SO_NO_ERROR)
        {
            spdlog::warn("AudioEngine: failed to load sound '{}', error={}", path, static_cast<int>(result));
            return 0;
        }
        it = m_impl->sfxCache.emplace(path, std::move(wav)).first;
    }

    float actualVolume = volume * m_impl->sfxVolume * m_impl->masterVolume;
    uint32_t handle = m_impl->soloud.play(*it->second, actualVolume);
    spdlog::debug("AudioEngine: playSound '{}' (handle={}, vol={:.2f})", path, handle, actualVolume);
    return handle;
}

// =========================================================================
// 背景音乐
// =========================================================================
void AudioEngine::playMusic(const std::string& path, float volume, bool loop)
{
    // 先停止当前音乐
    stopMusic();

    auto result = m_impl->currentMusic.load(path.c_str());
    if (result != SoLoud::SOLOUD_ERRORS::SO_NO_ERROR)
    {
        spdlog::warn("AudioEngine: failed to load music '{}', error={}", path, static_cast<int>(result));
        return;
    }

    m_impl->currentMusic.setLooping(loop);
    m_impl->musicLoaded = true;
    m_impl->musicPaused = false;

    float actualVolume = volume * m_impl->musicVolume * m_impl->masterVolume;
    m_impl->musicHandle = m_impl->soloud.play(m_impl->currentMusic, actualVolume);
    spdlog::info("AudioEngine: playMusic '{}' (loop={}, handle={}, vol={:.2f})",
        path, loop, m_impl->musicHandle, actualVolume);
}

void AudioEngine::stopMusic()
{
    if (!m_impl->musicLoaded) return;

    m_impl->soloud.stop(m_impl->musicHandle);
    m_impl->currentMusic.stop();
    m_impl->musicLoaded = false;
    m_impl->musicPaused = false;
    m_impl->musicHandle = 0;
    spdlog::debug("AudioEngine: music stopped");
}

void AudioEngine::pauseMusic()
{
    if (!m_impl->musicLoaded || m_impl->musicPaused) return;
    m_impl->soloud.setPause(m_impl->musicHandle, true);
    m_impl->musicPaused = true;
    spdlog::debug("AudioEngine: music paused");
}

void AudioEngine::resumeMusic()
{
    if (!m_impl->musicLoaded || !m_impl->musicPaused) return;
    m_impl->soloud.setPause(m_impl->musicHandle, false);
    m_impl->musicPaused = false;
    spdlog::debug("AudioEngine: music resumed");
}

bool AudioEngine::isMusicPlaying() const
{
    if (!m_impl->musicLoaded || m_impl->musicPaused) return false;
    return m_impl->soloud.isValidVoiceHandle(m_impl->musicHandle);
}

// =========================================================================
// 环境音
// =========================================================================
void AudioEngine::playAmbiance(const std::string& path, float fadeInSec)
{
    // 环境音复用音乐通道，但做淡入处理
    // SoLoud 的 fadeGlobalVolume 是全局的，这里用 scheduleStop + 新播放实现简单切换
    if (m_impl->musicLoaded)
    {
        // 淡出当前音乐/环境音
        if (fadeInSec > 0.0f && m_impl->musicHandle != 0)
        {
            m_impl->soloud.fadeVolume(m_impl->musicHandle, 0.0f, fadeInSec);
            m_impl->soloud.scheduleStop(m_impl->musicHandle, fadeInSec);
        }
        else
        {
            stopMusic();
        }
    }

    auto result = m_impl->currentMusic.load(path.c_str());
    if (result != SoLoud::SOLOUD_ERRORS::SO_NO_ERROR)
    {
        spdlog::warn("AudioEngine: failed to load ambiance '{}', error={}", path, static_cast<int>(result));
        return;
    }

    m_impl->currentMusic.setLooping(true);
    m_impl->musicLoaded = true;
    m_impl->musicPaused = false;

    float actualVolume = m_impl->musicVolume * m_impl->masterVolume;
    m_impl->musicHandle = m_impl->soloud.play(m_impl->currentMusic, 0.0f); // 从 0 开始

    if (fadeInSec > 0.0f)
    {
        m_impl->soloud.fadeVolume(m_impl->musicHandle, actualVolume, fadeInSec);
    }
    else
    {
        m_impl->soloud.setVolume(m_impl->musicHandle, actualVolume);
    }

    spdlog::info("AudioEngine: playAmbiance '{}' (fadeIn={:.1f}s)", path, fadeInSec);
}

void AudioEngine::stopAmbiance(float fadeOutSec)
{
    if (!m_impl->musicLoaded) return;

    if (fadeOutSec > 0.0f && m_impl->musicHandle != 0)
    {
        m_impl->soloud.fadeVolume(m_impl->musicHandle, 0.0f, fadeOutSec);
        m_impl->soloud.scheduleStop(m_impl->musicHandle, fadeOutSec);
    }
    else
    {
        stopMusic();
    }

    // 延迟清除标记
    m_impl->musicLoaded = false;
    m_impl->musicHandle = 0;
    spdlog::debug("AudioEngine: ambiance stopped (fadeOut={:.1f}s)", fadeOutSec);
}

// =========================================================================
// 音量控制
// =========================================================================
void AudioEngine::setMasterVolume(float vol)
{
    m_impl->masterVolume = std::clamp(vol, 0.0f, 1.0f);
    m_impl->soloud.setGlobalVolume(m_impl->masterVolume);
}

void AudioEngine::setSfxVolume(float vol)
{
    m_impl->sfxVolume = std::clamp(vol, 0.0f, 1.0f);
}

void AudioEngine::setMusicVolume(float vol)
{
    m_impl->musicVolume = std::clamp(vol, 0.0f, 1.0f);
    if (m_impl->musicLoaded && !m_impl->musicPaused)
    {
        float actualVolume = m_impl->musicVolume * m_impl->masterVolume;
        m_impl->soloud.setVolume(m_impl->musicHandle, actualVolume);
    }
}

float AudioEngine::masterVolume() const
{
    return m_impl->masterVolume;
}

} // namespace engine::audio

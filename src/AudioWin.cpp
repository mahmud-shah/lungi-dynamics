#include "Audio.h"

#include <cmath>
#include <cstdio>

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#endif

namespace {

const int kOneShotVolume = 850;

#ifdef _WIN32
bool command(const char* text)
{
    return mciSendStringA(text, nullptr, 0, nullptr) == 0;
}
#else
bool command(const char*)
{
    return false;
}
#endif

} // namespace

AudioPlayer::AudioPlayer()
    : channels_{
          {"wind", "assets\\sounds\\wind_loop.wav", true, false, -1},
          {"rain", "assets\\sounds\\rain_loop.wav", true, false, -1},
          {"gust", "assets\\sounds\\gust.wav", false, false, -1},
          {"thunder", "assets\\sounds\\thunder.wav", false, false, -1},
          {"knotpop", "assets\\sounds\\knot_pop.wav", false, false, -1},
          {"flyup", "assets\\sounds\\fly_up.wav", false, false, -1},
          {"slip", "assets\\sounds\\slip.wav", false, false, -1},
      }
{
}

// MCI ("Media Control Interface", part of winmm) can open several WAV files
// under different aliases and play them at the same time, which PlaySound
// cannot. "type mpegvideo" is used because that device supports looping
// ("repeat") and a volume per sound (0 - 1000). If a file fails to open,
// that sound is simply skipped.
void AudioPlayer::init()
{
    char text[256];
    for (Channel& c : channels_) {
        std::snprintf(text, sizeof(text), "open \"%s\" type mpegvideo alias %s", c.file, c.alias);
        c.open = command(text);
#ifdef _WIN32
        if (!c.open) {
            std::printf("Audio: could not open %s (that sound is disabled)\n", c.file);
        }
#endif
    }
    applyVolumes();
    for (Channel& c : channels_) {
        if (c.open && c.loop) {
            std::snprintf(text, sizeof(text), "play %s repeat", c.alias);
            command(text);
        }
    }
}

void AudioPlayer::shutdown()
{
    command("close all");
}

void AudioPlayer::play(Sound sound)
{
    Channel& c = channels_[2 + static_cast<int>(sound)];
    if (!c.open || muted_) {
        return;
    }
    char text[96];
    std::snprintf(text, sizeof(text), "play %s from 0", c.alias);
    command(text);
}

void AudioPlayer::setVolume(Channel& channel, int volume)
{
    if (!channel.open || channel.volume == volume) {
        return;
    }
    char text[96];
    std::snprintf(text, sizeof(text), "setaudio %s volume to %d", channel.alias, volume);
    command(text);
    channel.volume = volume;
}

void AudioPlayer::applyVolumes()
{
    setVolume(channels_[0], muted_ ? 0 : static_cast<int>(std::lround(wind_ * 1000.0f)));
    setVolume(channels_[1], muted_ ? 0 : static_cast<int>(std::lround(rain_ * 1000.0f)));
    for (int i = 2; i < kChannelCount; ++i) {
        setVolume(channels_[i], muted_ ? 0 : kOneShotVolume);
    }
}

// Levels are rounded to 2% steps, and setVolume skips unchanged values, so
// MCI is only called when the loudness actually changes.
void AudioPlayer::setLoopLevels(float wind, float rain)
{
    wind_ = std::round(wind * 50.0f) / 50.0f;
    rain_ = std::round(rain * 50.0f) / 50.0f;
    applyVolumes();
}

void AudioPlayer::toggleMute()
{
    muted_ = !muted_;
    applyVolumes();
}

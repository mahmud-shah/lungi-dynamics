#pragma once

enum class Sound {
    Gust,
    Thunder,
    KnotPop,
    FlyUp,
    Slip
};

// Sound effects and two looping ambiences (wind and rain). The Windows
// version is in AudioWin.cpp; on other systems every call does nothing.
class AudioPlayer {
public:
    AudioPlayer();

    void init();
    void shutdown();
    void play(Sound sound);
    void setLoopLevels(float wind, float rain);
    void toggleMute();
    bool muted() const { return muted_; }

private:
    struct Channel {
        const char* alias;
        const char* file;
        bool loop;
        bool open;
        int volume;
    };

    static const int kChannelCount = 7;

    void applyVolumes();
    void setVolume(Channel& channel, int volume);

    Channel channels_[kChannelCount];
    bool muted_ = false;
    float wind_ = 0.0f;
    float rain_ = 0.0f;
};

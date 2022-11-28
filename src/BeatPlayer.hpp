/// BeatPlayer
///
/// Plays a beat

#ifndef MNOME_BEATPLAYER_H
#define MNOME_BEATPLAYER_H

#include "AudioSignal.hpp"

#include <memory>
#include <miniaudio.h>

#include <atomic>
#include <mutex>
#include <vector>


namespace mnome {

constexpr size_t DEFAULT_BPM = 100;

/// Types of metronome beats: accent, normal beat and pause
enum class BeatType : char
{
    accent = '!',
    beat   = '+',
    pause  = '.',
};

/// A list of beats is a beat pattern
using BeatPatternType = std::vector<mnome::BeatType>;

/// A beat pattern is a list of different beat types
class MetronomeBeats
{
private:
    BeatPatternType pattern{BeatType::beat};

public:
    explicit MetronomeBeats(const std::string& strPattern);
    explicit MetronomeBeats(const BeatPatternType& otherPattern);

    void fromString(const std::string& strPattern);
    std::string toString() const;

    const BeatPatternType& getBeatPattern() const;
};


/// Plays a beat at a certain number of times per minute
class BeatPlayer
{
private:
    // data members
    size_t beatRate;
    std::unique_ptr<AudioSignal> beat;
    std::unique_ptr<AudioSignal> accentuatedBeat;
    AudioDataType playBackBuffer;
    MetronomeBeats beatPattern{"!+++"};

    // synchronization
    std::recursive_mutex setterMutex;
    std::atomic_bool requestStop{false};
    std::atomic_bool running{false};

    // miniaudio
    ma_context context;
    ma_device_config deviceConfig;
    ma_device device;
    ma_audio_buffer_config buf_config;
    ma_audio_buffer buf;


public:
    BeatPlayer();

    ~BeatPlayer();

    /// Set the sound of the beat that is played back
    /// \param  newBeat  samples the represent the beat
    void setBeat(const AudioSignal& newBeat);

    /// Set the sound of the beat that is played back
    /// \param  newBeat  samples the represent the accentuated beat
    void setAccentuatedBeat(const AudioSignal& newBeat);

    void setAccentuatedPattern(const MetronomeBeats& pattern);

    /// Start the BeatPlayer
    void start();

    /// Stop the beat playback
    /// \note Does not block
    void stop();

    /// Change the beat per minute
    /// \param  bpm  beats per minute
    void setBPM(size_t bpm);

    /// Get the current bpm setting
    size_t getBPM() const;

    /// Change the beat that is played back
    /// \param  beatData  The beat that is played back
    void setAccentuatedBeatData(const AudioSignal& beatData);

    /// Change the beat and the beats per minute, for convenience
    /// \param  beatData  The beat that is played back
    /// \param  bpm  beats per minute
    void setDataAndBPM(const AudioSignal& beatData, size_t bpm);

    /// Indicates whether the audio playback is running
    bool isRunning() const;

private:
    /// Start the audio playback
    void startAudio();

    /// Restart the audio playback
    void restart();
};


}  // namespace mnome


#endif  //  MNOME_BEATPLAYER_H

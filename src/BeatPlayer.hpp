/// BeatPlayer
///
/// Plays a beat

#ifndef MNOME_BEATPLAYER_H
#define MNOME_BEATPLAYER_H

#include <miniaudio.h>

#include <atomic>
#include <mutex>
#include <vector>


namespace mnome {


using TBeatDataType = int16_t;


/// A beat pattern is a list of different beat types
///
/// Currently accent, normal beat and pause beat types are supported.
class BeatPattern
{
public:
    enum BeatType : char
    {
        accent = '!',
        beat   = '+',
        pause  = '.',
    };

private:
    std::vector<mnome::BeatPattern::BeatType> pattern{};

public:
    BeatPattern() = default;
    BeatPattern(const std::string& pattern);
    BeatPattern(const std::vector<mnome::BeatPattern::BeatType>& pattern);

    void fromString(const std::string& strPattern);
    std::string toString() const;

    const std::vector<mnome::BeatPattern::BeatType>& getBeatPattern() const;
};


/// Plays a beat at a certain number of times per minute
class BeatPlayer
{
    size_t beatRate;
    std::recursive_mutex setterMutex;
    std::vector<TBeatDataType> beat;
    std::vector<TBeatDataType> accentuatedBeat;
    std::vector<TBeatDataType> playBackBuffer;
    BeatPattern beatPattern{std::vector<BeatPattern::BeatType>{BeatPattern::BeatType::beat}};
    std::atomic_bool requestStop{false};
    std::atomic_bool running{false};

    // miniaudio
    ma_context context;
    ma_device_config deviceConfig;
    ma_device device;
    ma_audio_buffer_config buf_config;
    ma_audio_buffer buf;


public:
    /// Constructor
    /// \param  beatRate  rate of the beat in [bpm]
    /// \param  beat      raw data of one beat
    BeatPlayer(size_t bRate);

    BeatPlayer() = delete;

    ~BeatPlayer();

    /// Set the sound of the beat that is played back
    /// \param  newBeat  samples the represent the beat
    void setBeat(const std::vector<TBeatDataType>& newBeat);

    /// Set the sound of the beat that is played back
    /// \param  newBeat  samples the represent the accentuated beat
    void setAccentuatedBeat(const std::vector<TBeatDataType>& newBeat);

    void setAccentuatedPattern(const BeatPattern& pattern);

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
    void setAccentuatedBeatData(const std::vector<TBeatDataType>& beatData);

    /// Change the beat and the beats per minute, for convenience
    /// \param  beatData  The beat that is played back
    /// \param  bpm  beats per minute
    void setDataAndBPM(const std::vector<TBeatDataType>& beatData, size_t bpm);

    /// Indicates whether the audio playback is running
    bool isRunning() const;

private:
    /// Start the audio playback
    void startAudio();

    /// Restart the audio playback
    void restart();
};


/// Generate a tone
/// \param[in]   freq       tone frequency
/// \param[in]   lengthS    length in seconds
/// \param[in]   harmonics  number of harmonics to add to the tone
/// \return      data       samples to be generated
std::vector<int16_t> generateTone(const double freq, const double lengthS,
                                  const size_t addHarmonics);

}  // namespace mnome


#endif  //  MNOME_BEATPLAYER_H

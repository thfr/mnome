#include <cstddef>
#include <cstdint>
#include <vector>

#ifndef MNOME_AUDIOSIGNAL_HPP
#define MNOME_AUDIOSIGNAL_HPP

namespace mnome {

using SampleType    = float;
using AudioDataType = std::vector<SampleType>;


struct AudioSignalConfiguration
{
    double       sampleRate;  //< [Hz]
    std::uint8_t channels;    //< number of interleaved channels
};


struct ToneConfiguration
{
    double       length;     //< [s]
    double       frequency;  //< [Hz]
    std::uint8_t overtones;  //< number overtones
};


class AudioSignal
{
private:
    AudioSignalConfiguration config;
    AudioDataType            data;

public:
    AudioSignal() = delete;

    explicit AudioSignal(const AudioSignalConfiguration& config, double lengthS);

    // usual initialization
    explicit AudioSignal(const AudioSignalConfiguration& config, AudioDataType&& data);

    void lowPass20KHz();
    void highPass20Hz();
    void fadeInOut(size_t fadeInSamples, size_t fadeOutSamples);

    [[nodiscard]] auto getAudioData() const -> const AudioDataType&;

    [[nodiscard]] auto numberSamples() const -> size_t;
    [[nodiscard]] auto length() const -> double;

    void resizeSamples(size_t numberSamples, SampleType value = 0);

    [[nodiscard]] auto mixingPossibile(const AudioSignal& other) const -> bool;

    auto operator+=(const AudioSignal& summand) -> AudioSignal&;
    auto operator-=(const AudioSignal& summand) -> AudioSignal&;
};

auto operator+(AudioSignal summand1, const AudioSignal& summand) -> AudioSignal;
auto operator-(AudioSignal minuend, const AudioSignal& subtrahend) -> AudioSignal;


/// Generate specific tone as an AudioSignal
auto generateTone(const AudioSignalConfiguration& audioConfig, const ToneConfiguration& toneConfig) -> AudioSignal;

/// Calculate frequency certain half steps away from a base frequency
auto halfToneOffset(double baseFreq, size_t offset) -> double;

};  // namespace mnome

#endif  // MNOME_AUDIOSIGNAL_HPP

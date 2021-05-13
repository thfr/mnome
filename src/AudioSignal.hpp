#include <cstddef>
#include <cstdint>
#include <iterator>
#include <vector>

#ifndef MNOME_AUDIOSIGNAL_HPP
#define MNOME_AUDIOSIGNAL_HPP

namespace mnome {

using SampleType    = float;
using AudioDataType = std::vector<SampleType>;


struct AudioSignalConfiguration
{
    double sampleRate;      //< [Hz]
    std::uint8_t channels;  //< number of interleaved channels
};


struct ToneConfiguration
{
    double length;           //< [s]
    double frequency;        //< [Hz]
    std::uint8_t overtones;  //< number overtones
};


class AudioSignal
{
private:
    AudioSignalConfiguration config;
    AudioDataType data;

public:
    AudioSignal() = delete;

    explicit AudioSignal(const AudioSignalConfiguration& config, double lengthS);

    // usual initialization
    explicit AudioSignal(const AudioSignalConfiguration& config, AudioDataType&& data);

    void lowPass20KHz();
    void highPass20Hz();
    void fadeInOut(size_t fadeInSamples, size_t fadeOutSamples);

    const AudioDataType& getAudioData() const;

    size_t numberSamples() const;
    double length() const;

    void resizeSamples(size_t numberSamples, SampleType value = 0);

    bool isCompatible(const AudioSignal& other) const;

    AudioSignal& operator+=(const AudioSignal& summand);
    AudioSignal& operator-=(const AudioSignal& summand);
};

AudioSignal operator+(AudioSignal summand1, const AudioSignal& summand);
AudioSignal operator-(AudioSignal minuend, const AudioSignal& summand);


/// Generate specific tone as an AudioSignal
AudioSignal generateTone(const AudioSignalConfiguration& audioConfig, const ToneConfiguration& toneConfig);

};  // namespace mnome

#endif  // MNOME_AUDIOSIGNAL_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

#ifndef MNOME_AUDIOSIGNAL_HPP
#define MNOME_AUDIOSIGNAL_HPP

namespace mnome {

using SampleType = float;

struct AudioSignalConfiguration
{
    double sampleRate;
    double frequency;
    double length;           //< length in [s]
    std::uint8_t overtones;  //< number overtones
    std::uint8_t channels;   //< number of interleaved channels
};


class AudioSignal
{
private:
    AudioSignalConfiguration config;
    std::vector<SampleType> data;

public:
    AudioSignal() = delete;

    // usual initialization
    explicit AudioSignal(const AudioSignalConfiguration& config);

    // copy constructor
    explicit AudioSignal(const AudioSignal& audio);

    // move constructor
    explicit AudioSignal(AudioSignal&& audio);

    void lowPass20KHz();
    void highPass20Hz();
    void fadeInOut(size_t fadeInSamples, size_t fadeOutSamples);

    AudioSignal& operator+=(const AudioSignal& summand);
    AudioSignal& operator-=(const AudioSignal& summand);
};

AudioSignal&& operator+(const AudioSignal& summand);

AudioSignal&& operator-(const AudioSignal& summand);

};  // namespace mnome

#endif  // MNOME_AUDIOSIGNAL_HPP

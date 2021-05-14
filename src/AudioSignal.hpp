#include <cstddef>
#include <cstdint>
#include <vector>

namespace mnome {

struct AudioSignalConfiguration
{
    double sampleRate;
    double frequency;
    double length;           //< length in [s]
    std::uint8_t overtones;  //< number overtones
    std::uint8_t channels;   //< number of interleaved channels
};


template <typename TSample>
class AudioSignal
{
private:
    AudioSignalConfiguration config;
    std::vector<TSample> data;

public:
    AudioSignal(const AudioSignalConfiguration& config);

    void lowPass20KHz();
    void highPass20Hz();
};

template <typename TSample>
AudioSignal<TSample> operator+(const AudioSignal<TSample>& summand);

template <typename TSample>
AudioSignal<TSample> operator-(const AudioSignal<TSample>& summand);

template <typename TSample>
AudioSignal<TSample>& operator+=(AudioSignal<TSample>& result, const AudioSignal<TSample>& summand);

template <typename TSample>
AudioSignal<TSample>& operator-=(AudioSignal<TSample>& result, const AudioSignal<TSample>& summand);

};  // namespace mnome

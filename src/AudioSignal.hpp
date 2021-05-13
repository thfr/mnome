#include <cstddef>
#include <vector>

struct AudioSignalConfiguration
{
    double sampleRate;
    double frequency;
    size_t overtones;
    double length;    //< length in [s]
    double channels;  // are stored interleaved
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

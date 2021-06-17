/// AudioSignal
///
/// Contains audio samples and allows some operations

#include "AudioSignal.hpp"

#include <algorithm>
#include <cmath>
#include <cstdbool>
#include <cstdint>
#include <limits>


namespace mnome {

using namespace std;

constexpr double PI = 3.141592653589793;

// Generated with http://www-users.cs.york.ac.uk/~fisher/mkfilter/ - no license given -
// and adjusted to work as a standalone function.
void AudioSignal::lowPass20KHz()
{
    /* Digital filter designed by mkfilter/mkshape/gencode   A.J. Fisher
     *    Command line: /www/usr/fisher/helpers/mkfilter -Bu -Lp -o 2 -a 4.1666666667e-01
     * 0.0000000000e+00 -l */

    constexpr size_t NZEROS = 2;
    constexpr size_t NPOLES = 2;
    constexpr double GAIN   = 1.450734152e+00;

    double xv[NZEROS + 1]{};
    double yv[NPOLES + 1]{};

    for (auto& sample : data) {
        xv[0]  = xv[1];
        xv[1]  = xv[2];
        xv[2]  = sample / GAIN;
        yv[0]  = yv[1];
        yv[1]  = yv[2];
        yv[2]  = (xv[0] + xv[2]) + 2 * xv[1] + (-0.4775922501 * yv[0]) + (-1.2796324250 * yv[1]);
        sample = static_cast<SampleType>(yv[2]);
    }
}
// Generated with http://www-users.cs.york.ac.uk/~fisher/mkfilter/ - no license given -
// and adjusted to work as a standalone function.
void AudioSignal::highPass20Hz()
{
    /* Digital filter designed by mkfilter/mkshape/gencode   A.J. Fisher
     *    Command line: /www/usr/fisher/helpers/mkfilter -Bu -Lp -o 2 -a 4.1666666667e-01
     * 0.0000000000e+00 -l */

    constexpr size_t NZEROS = 2;
    constexpr size_t NPOLES = 2;
    constexpr double GAIN   = 1.001852916e+00;

    double xv[NZEROS + 1]{};
    double yv[NPOLES + 1]{};

    for (auto& sample : data) {
        xv[0]  = xv[1];
        xv[1]  = xv[2];
        xv[2]  = sample / GAIN;
        yv[0]  = yv[1];
        yv[1]  = yv[2];
        yv[2]  = (xv[0] + xv[2]) - 2 * xv[1] + (-0.9963044430 * yv[0]) + (1.9962976018 * yv[1]);
        sample = static_cast<SampleType>(yv[2]);
    }
}

/// Fade a signal in and out
/// \param  data  signal to be faded
/// \param  fadeInSamples  number of samples on which the fading in is done
/// \param  fadeOutSamples  number of samples on which the fading out is done
void AudioSignal::fadeInOut(size_t fadeInSamples, size_t fadeOutSamples)
{
    // *Exponential Fading* is used because it is more pleasant to ear than linear fading.
    //
    // A factor with changing value is multiplied to each sample of the fading period.
    // The factor must be increased by multiplying it with a constant ratio that. Therefore the
    // factor must have a starting value > 0.0 .
    //    fs * (r ** steps) = 1         (discrete form: f[n+1] = f[n] * r , while f[n+1] <= 1)
    //    r ** steps  = 1 / fs
    //    r = (1 / fs) ** (1 / steps)
    //       where fs = factor at start
    //              r = ratio

    // apply all factors for the fade in
    double startValue  = 1.0 / INT16_MAX;
    double fadeInRatio = pow(1.0 / startValue, 1.0 / fadeInSamples);
    double factor      = startValue;
    for (size_t samIdx = 0; samIdx < fadeInSamples; ++samIdx) {
        data[samIdx] *= factor;
        factor *= fadeInRatio;
    };

    // apply all factors for the fade out
    double fadeOutRatio = 1.0 / pow(1.0 / startValue, 1.0 / fadeOutSamples);
    size_t fadeOutIndex = max(static_cast<size_t>(0), data.size() - fadeOutSamples);
    factor              = fadeOutRatio;
    for (size_t samIdx = fadeOutIndex; samIdx < data.size(); ++samIdx) {
        data[samIdx] *= factor;
        factor *= fadeOutRatio;
    };
}

vector<SampleType> generateTone(const double freq, const double lengthS, const size_t addHarmonics,
                                const size_t sampleRate)
{
    auto samples = static_cast<size_t>(floor(sampleRate * lengthS));

    vector<SampleType> data;
    data.reserve(samples);

    for (size_t samIdx = 0; samIdx < samples; samIdx++) {
        double sample = sin(samIdx * 2 * PI * freq / sampleRate);

        // add harmonics
        double harmonicGainFactor = 0.5;
        double gain               = 0.5;
        for (size_t harmonic = 0; harmonic < addHarmonics; ++harmonic) {
            gain *= harmonicGainFactor;
            sample += gain * sin(samIdx * 2 * PI * (harmonic + 2) * freq / sampleRate);
        }
        data.emplace_back(static_cast<SampleType>(0.5 * sample));
    }
    return data;
}

AudioSignal::AudioSignal(const AudioSignalConfiguration& config)
    : config{config}, data{generateTone(config.frequency, config.length, config.overtones, config.sampleRate)} {}

AudioSignal::AudioSignal(const AudioSignal& audio) : config{audio.config}, data{audio.data} {}

AudioSignal::AudioSignal(AudioSignal&& audio) : config{std::move(audio.config)}, data{std::move(audio.data)} {}

AudioSignal& AudioSignal::operator+=(const AudioSignal& summand)
{
    size_t max_length = std::max(data.size(), summand.data.size());
    if (max_length > data.size()) {
        data.resize(max_length, static_cast<SampleType>(0));
    }
    for (size_t index = 0; index < summand.data.size(); ++index) {
        data[index] += summand.data[index];
    }
    return *this;
}

AudioSignal& AudioSignal::operator-=(const AudioSignal& summand)
{
    size_t max_length = std::max(data.size(), summand.data.size());
    if (max_length > data.size()) {
        data.resize(max_length, static_cast<SampleType>(0));
    }
    for (size_t index = 0; index < summand.data.size(); ++index) {
        data[index] -= summand.data[index];
    }
    return *this;
}

AudioSignal&& operator+(const AudioSignal& summand1, const AudioSignal& summand2)
{
    AudioSignal result{summand1};
    result += summand2;
    return std::move(result);
}

AudioSignal&& operator-(const AudioSignal& minuend, const AudioSignal& subtrahend)
{
    AudioSignal result{minuend};
    result -= subtrahend;
    return std::move(result);
}

};  // namespace mnome

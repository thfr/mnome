/// AudioSignal
///
/// Contains audio samples and allows some operations

#include "AudioSignal.hpp"

#include <array>
#include <doctest.h>

#include <algorithm>
#include <cmath>
#include <cstdbool>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <numbers>


namespace mnome {

const size_t halfStepsInOctave = 12;

using namespace std;

AudioSignal::AudioSignal(const AudioSignalConfiguration& as_config, double lengthS)
    : config{as_config}, data{AudioDataType(static_cast<size_t>(config.channels * config.sampleRate * lengthS), 0)}
{
}

AudioSignal::AudioSignal(const AudioSignalConfiguration& as_config, AudioDataType&& audio_data)
    : config{as_config}, data{audio_data}
{
}

void biquad_2nd_order_df1_normalized(vector<SampleType>& data, double gain, double biquad_y0_factor,
                                     double biquad_y1_factor)
{
    constexpr size_t NZEROS = 2;
    constexpr size_t NPOLES = 2;

    std::array<double, NZEROS + 1> biquad_x{};
    std::array<double, NPOLES + 1> biquad_y{};

    for (auto& sample : data) {
        biquad_x[0] = biquad_x[1];
        biquad_x[1] = biquad_x[2];
        biquad_x[2] = sample / gain;
        biquad_y[0] = biquad_y[1];
        biquad_y[1] = biquad_y[2];
        biquad_y[2] = (biquad_x[0] + biquad_x[2]) + 2 * biquad_x[1] + (biquad_y0_factor * biquad_y[0]) +
                      (biquad_y1_factor * biquad_y[1]);
        sample = static_cast<SampleType>(biquad_y[2]);
    }
}

// Generated with http://www-users.cs.york.ac.uk/~fisher/mkfilter/ - no license given -
// and adjusted to work as a standalone function.
void AudioSignal::lowPass20KHz()
{
    /* Digital filter designed by mkfilter/mkshape/gencode   A.J. Fisher
     *    Command line: /www/usr/fisher/helpers/mkfilter -Bu -Lp -o 2 -a 4.1666666667e-01
     * 0.0000000000e+00 -l */

    constexpr double GAIN             = 1.450734152e+00;
    constexpr double biquad_y0_factor = -0.4775922501;
    constexpr double biquad_y1_factor = -1.2796324250;
    biquad_2nd_order_df1_normalized(data, GAIN, biquad_y0_factor, biquad_y1_factor);
}
// Generated with http://www-users.cs.york.ac.uk/~fisher/mkfilter/ - no license given -
// and adjusted to work as a standalone function.
void AudioSignal::highPass20Hz()
{
    /* Digital filter designed by mkfilter/mkshape/gencode   A.J. Fisher
     *    Command line: /www/usr/fisher/helpers/mkfilter -Bu -Lp -o 2 -a 4.1666666667e-01
     * 0.0000000000e+00 -l */
    constexpr double GAIN             = 1.001852916e+00;
    constexpr double biquad_y0_factor = -0.9963044430;
    constexpr double biquad_y1_factor = 1.9962976018;
    biquad_2nd_order_df1_normalized(data, GAIN, biquad_y0_factor, biquad_y1_factor);
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

const AudioDataType& AudioSignal::getAudioData() const
{
    return data;
}

size_t AudioSignal::numberSamples() const
{
    return data.size();
}

double AudioSignal::length() const
{
    return static_cast<double>(data.size()) / config.sampleRate;
}

void AudioSignal::resizeSamples(size_t numberSamples, SampleType value)
{
    data.resize(numberSamples, value);
}

bool AudioSignal::mixingPossibile(const AudioSignal& other) const
{
    return this->config.sampleRate == other.config.sampleRate && this->config.channels == other.config.channels;
}

AudioSignal& AudioSignal::operator+=(const AudioSignal& summand)
{
    if (!this->mixingPossibile(summand)) {
        throw std::exception();
    }
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
    if (!this->mixingPossibile(summand)) {
        throw std::exception();
    }
    size_t max_length = std::max(data.size(), summand.data.size());
    if (max_length > data.size()) {
        data.resize(max_length, static_cast<SampleType>(0));
    }
    for (size_t index = 0; index < summand.data.size(); ++index) {
        data[index] -= summand.data[index];
    }
    return *this;
}

AudioSignal operator+(AudioSignal summand1, const AudioSignal& summand2)
{
    if (!summand1.mixingPossibile(summand2)) {
        throw std::exception();
    }
    summand1 += summand2;
    return AudioSignal{std::move(summand1)};
}

AudioSignal operator-(AudioSignal minuend, const AudioSignal& subtrahend)
{
    if (!minuend.mixingPossibile(subtrahend)) {
        throw std::exception();
    }
    minuend -= subtrahend;
    return AudioSignal{std::move(minuend)};
}


AudioSignal generateTone(const AudioSignalConfiguration& audioConfig, const ToneConfiguration& toneConfig)
{
    const auto sampleRate   = audioConfig.sampleRate;
    const auto lengthS      = toneConfig.length;
    const auto freq         = toneConfig.frequency;
    const auto addHarmonics = toneConfig.overtones;
    const auto samples      = static_cast<size_t>(floor(sampleRate * lengthS));
    const auto gainFactor   = 0.5;

    AudioDataType data;
    data.reserve(samples);

    for (size_t samIdx = 0; samIdx < samples; samIdx++) {
        double sample = sin(samIdx * 2 * numbers::pi * freq / sampleRate);

        // add harmonics
        const double harmonicGainFactor = gainFactor;
        double gain                     = harmonicGainFactor;
        for (size_t harmonic = 0; harmonic < addHarmonics; ++harmonic) {
            gain *= harmonicGainFactor;
            sample += gain * sin(samIdx * 2 * numbers::pi * (harmonic + 2) * freq / sampleRate);
        }
        for (size_t channelIdx = 0; channelIdx < audioConfig.channels; ++channelIdx) {
            data.emplace_back(static_cast<SampleType>(gainFactor * sample));
        }
    }
    return AudioSignal(audioConfig, std::move(data));
}

double halfToneOffset(double baseFreq, size_t offset)
{
    return baseFreq * pow(pow(2, 1.0 / halfStepsInOctave), offset);
};

TEST_CASE("AudioSignalTest")
{
    const auto sampleRate = 48'000;
    const auto channels   = 1;
    const auto sineFreq   = 440;
    const auto sineLength = 0.1;
    AudioSignalConfiguration audioConfig{
        .sampleRate = sampleRate,
        .channels   = channels,
    };
    ToneConfiguration sine440hzConfig{
        .length    = sineLength,
        .frequency = sineFreq,
        .overtones = 0,
    };

    AudioSignal sine440hz1 = generateTone(audioConfig, sine440hzConfig);
    AudioSignal op_plus    = sine440hz1 + sine440hz1;

    CHECK_EQ(op_plus.getAudioData().size(), sine440hz1.getAudioData().size());

    for (size_t idx = 0; idx < op_plus.getAudioData().size(); ++idx) {
        CHECK_EQ(op_plus.getAudioData()[idx], 2 * sine440hz1.getAudioData()[idx]);
    }

    op_plus -= sine440hz1;
    op_plus -= sine440hz1;

    for (size_t idx = 0; idx < op_plus.getAudioData().size(); ++idx) {
        CHECK_EQ(op_plus.getAudioData()[idx], 0);
    }
}


};  // namespace mnome

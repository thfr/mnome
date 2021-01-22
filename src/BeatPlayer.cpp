/// BeatPlayer
///
/// Plays a beat

#include "BeatPlayer.hpp"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <sstream>
#include <vector>


using namespace std;


constexpr size_t PLAYBACK_RATE           = 48'000;  // [Hz]
constexpr double PLAYBACK_MIN_ALSA_WRITE = 0.1;     // [s]
constexpr double FADE_MIN_TIME           = 0.025;   // [s]
constexpr double FADE_MIN_PERCENTAGE     = 0.30;
constexpr double PI                      = 3.141592653589793;


namespace mnome {


// Generated with http://www-users.cs.york.ac.uk/~fisher/mkfilter/ - no license given -
// and adjusted to work as a standalone function.
void lowPass20KHz(vector<TBeatDataType>& signal)
{
    /* Digital filter designed by mkfilter/mkshape/gencode   A.J. Fisher
     *    Command line: /www/usr/fisher/helpers/mkfilter -Bu -Lp -o 2 -a 4.1666666667e-01
     * 0.0000000000e+00 -l */

    constexpr size_t NZEROS = 2;
    constexpr size_t NPOLES = 2;
    constexpr double GAIN   = 1.450734152e+00;

    double xv[NZEROS + 1]{};
    double yv[NPOLES + 1]{};

    for (auto& sample : signal) {
        xv[0]  = xv[1];
        xv[1]  = xv[2];
        xv[2]  = sample / GAIN;
        yv[0]  = yv[1];
        yv[1]  = yv[2];
        yv[2]  = (xv[0] + xv[2]) + 2 * xv[1] + (-0.4775922501 * yv[0]) + (-1.2796324250 * yv[1]);
        sample = static_cast<TBeatDataType>(round(yv[2]));
    }
}

// Generated with http://www-users.cs.york.ac.uk/~fisher/mkfilter/ - no license given -
// and adjusted to work as a standalone function.
void highPass20Hz(vector<TBeatDataType>& signal)
{
    /* Digital filter designed by mkfilter/mkshape/gencode   A.J. Fisher
     *    Command line: /www/usr/fisher/helpers/mkfilter -Bu -Lp -o 2 -a 4.1666666667e-01
     * 0.0000000000e+00 -l */

    constexpr size_t NZEROS = 2;
    constexpr size_t NPOLES = 2;
    constexpr double GAIN   = 1.001852916e+00;

    double xv[NZEROS + 1]{};
    double yv[NPOLES + 1]{};

    for (auto& sample : signal) {
        xv[0]  = xv[1];
        xv[1]  = xv[2];
        xv[2]  = sample / GAIN;
        yv[0]  = yv[1];
        yv[1]  = yv[2];
        yv[2]  = (xv[0] + xv[2]) - 2 * xv[1] + (-0.9963044430 * yv[0]) + (1.9962976018 * yv[1]);
        sample = static_cast<TBeatDataType>(round(yv[2]));
    }
}

/// Fade a signal in and out
/// \tparam  T  datatype of the signal
/// \param  data  signal to be faded
/// \param  fadeInSamples  number of samples on which the fading in is done
/// \param  fadeOutSamples  number of samples on which the fading out is done
template <typename TSample>
void fadeInOut(vector<TSample>& data, size_t fadeInSamples, size_t fadeOutSamples)
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

    // filter output so that the signal stays inbetween 20 and 20000Hz
    // this removes audible aliasing effects
    lowPass20KHz(data);
    highPass20Hz(data);
}


/// Return the time it takes to playback a number of samples
/// \param[in]  samples  number of samples
double getDuration(size_t samples)
{
    return static_cast<double>(samples) / PLAYBACK_RATE;
}


/// Return the time it takes to playback a number of samples
/// \param[in]  time  time in seconds
/// \return     samples  number of samples that resemble \p time samples
size_t getNumbersOfSamples(double time)
{
    return static_cast<size_t>(round(time * PLAYBACK_RATE));
}


vector<int16_t> generateTone(const double freq, const double lengthS, const size_t addHarmonics)
{
    auto samples = static_cast<size_t>(floor(PLAYBACK_RATE * lengthS));

    vector<int16_t> data;
    data.reserve(samples);

    for (size_t samIdx = 0; samIdx < samples; samIdx++) {
        double sample = sin(samIdx * 2 * PI * freq / PLAYBACK_RATE);

        // add harmonics
        double harmonicGainFactor = 0.5;
        double gain               = 0.5;
        for (size_t harmonic = 0; harmonic < addHarmonics; ++harmonic) {
            gain *= harmonicGainFactor;
            sample += gain * sin(samIdx * 2 * PI * (harmonic + 2) * freq / PLAYBACK_RATE);
        }
        data.emplace_back(static_cast<int16_t>(INT16_MAX * 0.5 * sample));
    }
    return data;
}


BeatPlayer::BeatPlayer(const size_t brate) : beatRate(brate)
{
}

BeatPlayer::~BeatPlayer()
{
    stop();
}


void BeatPlayer::start()
{
    lock_guard<recursive_mutex> guard(setterMutex);
    if (isRunning()) {
        cout << "Error: BeatPlayer is already running, but was started again" << endl;
        return;
    }

    playBackBuffer.clear();

    const auto beatIntervalSamples = static_cast<size_t>(floor(1.0 / (beatRate / 60.0) * PLAYBACK_RATE));
    auto localBeat                 = beat;
    auto localAccentuatedBeat      = accentuatedBeat;
    decltype(localBeat) pause;

    const auto& pattern = beatPattern.getBeatPattern();

    if (localBeat.empty()) {
        cout << "Warning: the beat is silence, you will not hear anything." << endl;
    }
    if (pattern.empty()) {
        cout << "Not playing, beat pattern is empty" << endl;
        return;
    }
    const double lengthS = getDuration(beat.size());

    // Fade the beat in and out to avoid click/pop noises because of too sudden output value
    // changes
    const auto rampingSteps = (lengthS * FADE_MIN_PERCENTAGE < FADE_MIN_TIME)
                                  ? getNumbersOfSamples(lengthS * FADE_MIN_PERCENTAGE)
                                  : getNumbersOfSamples(FADE_MIN_TIME);

    // Since fadeInOut only uses the first x and last y samples,
    // make sure not to fade out zero values
    auto adjustBuffer = [&beatIntervalSamples, &rampingSteps](vector<TBeatDataType>& buffer) {
        if (buffer.size() > beatIntervalSamples) {
            buffer.resize(beatIntervalSamples, 0);
            fadeInOut(buffer, rampingSteps, rampingSteps);
        }
        else {
            fadeInOut(buffer, rampingSteps, rampingSteps);
            buffer.resize(beatIntervalSamples, 0);
        }
    };

    // prepare the sounds for each beat pattern type
    adjustBuffer(localBeat);
    if (localAccentuatedBeat.empty()) {
        localAccentuatedBeat = localBeat;
    }
    else {
        adjustBuffer(localAccentuatedBeat);
    }
    pause.clear();
    pause.resize(beatIntervalSamples, 0);


    // fill the playback buffer according to the pattern
    auto playBackBufferIterator = back_inserter(playBackBuffer);
    for (const auto& beatType : pattern) {
        switch (beatType) {
        case BeatPattern::accent:
            playBackBufferIterator =
                copy(begin(localAccentuatedBeat), end(localAccentuatedBeat), playBackBufferIterator);
            break;
        case BeatPattern::beat:
            playBackBufferIterator = copy(begin(localBeat), end(localBeat), playBackBufferIterator);
            break;
        case BeatPattern::pause:
            playBackBufferIterator = copy(begin(pause), end(pause), playBackBufferIterator);
            break;
        }
    }

    cout << "Playing " << beatPattern.toString() << " at " << beatRate << " bpm" << endl;

    startAudio();
}


void miniaudio_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    ma_audio_buffer* buffer = (ma_audio_buffer*)pDevice->pUserData;
    if (buffer == nullptr) {
        cout << "Buffer is empty\n";
        cout.flush();
        return;
    }
    ma_audio_buffer_read_pcm_frames(buffer, pOutput, frameCount, true);
    (void)pInput;
}


void BeatPlayer::startAudio()
{
    lock_guard<recursive_mutex> lg(setterMutex);
    if (isRunning()) {
        cout << "Audio playback was started though it is already running\n";
        return;
    }
    running = true;
    ma_result result;
    result = ma_context_init(nullptr, 0, nullptr, &context);

    // put the playback buffer inside the ma_audio_buffer structure
    buf_config = ma_audio_buffer_config_init(ma_format_s16, 1, playBackBuffer.size(), playBackBuffer.data(), nullptr);
    result     = ma_audio_buffer_init(&buf_config, &buf);
    if (result != MA_SUCCESS) {
        cout << "Audio buffer initialization failed, aborting\n";
        running = false;
        return;
    }

    deviceConfig                          = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format          = ma_format_s16;
    deviceConfig.playback.channels        = 1;
    deviceConfig.sampleRate               = 48000;
    deviceConfig.periods                  = 2;
    deviceConfig.periodSizeInMilliseconds = PLAYBACK_MIN_ALSA_WRITE * 1000;
    deviceConfig.dataCallback             = miniaudio_data_callback;
    deviceConfig.pUserData                = &buf;

    result = ma_device_init(&context, &deviceConfig, &device);
    if (result != MA_SUCCESS) {
        cout << "Device initialization failed, aborting\n";
        running = false;
        return;
    }

    ma_device_start(&device);
}

void BeatPlayer::stop()
{
    lock_guard<recursive_mutex> lg(setterMutex);
    if (isRunning()) {
        cout << "Stopping playback" << endl;
        ma_device_uninit(&device);
        ma_audio_buffer_uninit(&buf);
        ma_context_uninit(&context);
        running = false;
    }
}

void BeatPlayer::restart()
{
    lock_guard<recursive_mutex> guard(setterMutex);
    if (isRunning()) {
        stop();
        start();
    }
}

void BeatPlayer::setBPM(size_t bpm)
{
    lock_guard<recursive_mutex> guard(setterMutex);
    beatRate = bpm;
    restart();
}

size_t BeatPlayer::getBPM() const
{
    return beatRate;
}

void BeatPlayer::setAccentuatedBeat(const vector<TBeatDataType>& newBeat)
{
    lock_guard<recursive_mutex> guard(setterMutex);
    accentuatedBeat = newBeat;
    restart();
}

void BeatPlayer::setBeat(const vector<TBeatDataType>& newBeat)
{
    lock_guard<recursive_mutex> guard(setterMutex);
    beat = newBeat;
    restart();
}

void BeatPlayer::setDataAndBPM(const vector<TBeatDataType>& beatData, size_t bpm)
{
    lock_guard<recursive_mutex> guard(setterMutex);
    beat     = beatData;
    beatRate = bpm;
    restart();
}

void BeatPlayer::setAccentuatedPattern(const BeatPattern& pattern)
{
    lock_guard<recursive_mutex> guard(setterMutex);
    beatPattern = pattern;
    restart();
}

bool BeatPlayer::isRunning() const
{
    return running;
}


BeatPattern::BeatPattern(const std::string& strPattern)
{
    fromString(strPattern);
}
BeatPattern::BeatPattern(const std::vector<mnome::BeatPattern::BeatType>& otherPattern)
{
    pattern = otherPattern;
}

void BeatPattern::fromString(const std::string& strPattern)
{
    pattern.clear();
    for (const char& character : strPattern) {
        const auto convertedType = static_cast<BeatPattern::BeatType>(character);

        // the following switch will ignore all non valid conversions of character to
        // BeatPattern::BeatType
        switch (convertedType) {
        case BeatType::accent:
            pattern.push_back(BeatType::accent);
            break;
        case BeatType::beat:
            pattern.push_back(BeatType::beat);
            break;
        case BeatType::pause:
            pattern.push_back(BeatType::pause);
            break;
        }
    }
}

std::string BeatPattern::toString() const
{
    std::stringstream ss;
    for (auto& type : pattern) {
        ss << static_cast<char>(type);
    }
    return ss.str();
}

const std::vector<mnome::BeatPattern::BeatType>& BeatPattern::getBeatPattern() const
{
    return pattern;
}

}  // namespace mnome

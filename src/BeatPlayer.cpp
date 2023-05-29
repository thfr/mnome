/// BeatPlayer
///
/// Plays a beat

#include "BeatPlayer.hpp"
#include <miniaudio.h>

#include <algorithm>
#include <atomic>
#include <bits/chrono.h>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>
#include <numbers>
#include <sstream>
#include <vector>


using namespace std;


constexpr double FADE_MIN_PERCENTAGE     = 0.30;
constexpr double FADE_MIN_TIME           = 0.025;   // [s]
constexpr size_t PLAYBACK_MIN_ALSA_WRITE = 100;     // [ms]
constexpr size_t PLAYBACK_RATE           = 48'000;  // [Hz]


namespace mnome {


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
        double sample = sin(samIdx * 2 * numbers::pi * freq / PLAYBACK_RATE);

        // add harmonics
        constexpr double harmonicGain = 0.5;
        constexpr double initialGain  = 0.5;
        constexpr double volume       = 0.5;
        double gain                   = initialGain;
        for (size_t harmonic = 0; harmonic < addHarmonics; ++harmonic) {
            gain *= harmonicGain;
            sample += gain * sin(samIdx * 2 * numbers::pi * (harmonic + 2) * freq / PLAYBACK_RATE);
        }
        data.emplace_back(static_cast<int16_t>(INT16_MAX * volume * sample));
    }
    return data;
}


BeatPlayer::BeatPlayer() : beatRate(DEFAULT_BPM)
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
    if (!beat || !accentuatedBeat) {
        cout << "Error: No beat audio signal has been set" << endl;
    }
    auto localBeat            = AudioSignal(*beat);
    auto localAccentuatedBeat = AudioSignal(*accentuatedBeat);
    playBackBuffer.clear();

    const double beatIntervalLength  = static_cast<double>(beatRate / 60.0);
    const size_t beatIntervalSamples = static_cast<size_t>(floor(1.0 / beatIntervalLength * PLAYBACK_RATE));
    auto pause                       = AudioSignal(AudioSignalConfiguration{PLAYBACK_RATE, 1}, beatIntervalLength);

    const auto& pattern = beatPattern.getBeatPattern();

    if (0 == localBeat.numberSamples()) {
        cout << "Warning: the beat is silence, you will not hear anything." << endl;
    }
    if (pattern.empty()) {
        cout << "Not playing, beat pattern is empty" << endl;
        return;
    }
    const double lengthS = localBeat.length();

    // Fade the beat in and out to avoid click/pop noises because of too sudden amplitude changes
    const auto rampingSteps = (lengthS * FADE_MIN_PERCENTAGE < FADE_MIN_TIME)
                                  ? getNumbersOfSamples(lengthS * FADE_MIN_PERCENTAGE)
                                  : getNumbersOfSamples(FADE_MIN_TIME);

    // Since fadeInOut only uses the first x and last y samples,
    // make sure not to fade out zero values
    auto adjustBuffer = [&beatIntervalSamples, &rampingSteps](AudioSignal& signal) {
        if (signal.numberSamples() > beatIntervalSamples) {
            signal.resizeSamples(beatIntervalSamples, 0);
            signal.fadeInOut(rampingSteps, rampingSteps);
        }
        else {
            signal.fadeInOut(rampingSteps, rampingSteps);
            signal.resizeSamples(beatIntervalSamples, 0);
        }
    };

    // prepare the sounds for each beat pattern type
    adjustBuffer(localBeat);
    adjustBuffer(pause);
    if (localAccentuatedBeat.numberSamples() == 0) {
        localAccentuatedBeat = AudioSignal(static_cast<const AudioSignal&>(localBeat));
    }
    else {
        adjustBuffer(localAccentuatedBeat);
    }

    // fill the playback buffer according to the pattern
    auto playBackBufferIterator = back_inserter(playBackBuffer);
    for (const auto& beatType : pattern) {
        switch (beatType) {
        case BeatType::accent:
            playBackBufferIterator = copy(begin(localAccentuatedBeat.getAudioData()),
                                          end(localAccentuatedBeat.getAudioData()), playBackBufferIterator);
            break;
        case BeatType::beat:
            playBackBufferIterator =
                copy(begin(localBeat.getAudioData()), end(localBeat.getAudioData()), playBackBufferIterator);
            break;
        case BeatType::pause:
            cout << "pause is inserted" << endl;
            playBackBufferIterator =
                copy(begin(pause.getAudioData()), end(pause.getAudioData()), playBackBufferIterator);
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
    ma_audio_buffer_read_pcm_frames(buffer, pOutput, frameCount, static_cast<ma_bool32>(true));
    (void)pInput;
}


void BeatPlayer::startAudio()
{
    lock_guard<recursive_mutex> lockGuard(setterMutex);
    if (isRunning()) {
        cout << "Audio playback was started though it is already running\n";
        return;
    }
    running = true;
    ma_result result;
    result = ma_context_init(nullptr, 0, nullptr, &context);

    ma_format sample_format = ma_format_f32;

    // put the playback buffer inside the ma_audio_buffer structure
    buf_config = ma_audio_buffer_config_init(sample_format, 1, playBackBuffer.size(), playBackBuffer.data(), nullptr);
    result     = ma_audio_buffer_init(&buf_config, &buf);
    if (result != MA_SUCCESS) {
        cout << "Audio buffer initialization failed, aborting\n";
        running = false;
        return;
    }

    deviceConfig                          = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format          = sample_format;
    deviceConfig.playback.channels        = 1;
    deviceConfig.sampleRate               = PLAYBACK_RATE;
    deviceConfig.periods                  = 2;
    deviceConfig.periodSizeInMilliseconds = PLAYBACK_MIN_ALSA_WRITE;
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
    lock_guard<recursive_mutex> lockGuard(setterMutex);
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

void BeatPlayer::setAccentuatedBeat(const AudioSignal& newBeat)
{
    lock_guard<recursive_mutex> guard(setterMutex);
    accentuatedBeat = std::make_unique<AudioSignal>(newBeat);
    restart();
}

void BeatPlayer::setBeat(const AudioSignal& newBeat)
{
    lock_guard<recursive_mutex> guard(setterMutex);
    beat = std::make_unique<AudioSignal>(newBeat);
    restart();
}

void BeatPlayer::setAccentuatedPattern(const MetronomeBeats& pattern)
{
    lock_guard<recursive_mutex> guard(setterMutex);
    beatPattern = pattern;
    restart();
}

bool BeatPlayer::isRunning() const
{
    return running;
}


MetronomeBeats::MetronomeBeats(const std::string& strPattern)
{
    fromString(strPattern);
}
MetronomeBeats::MetronomeBeats(const BeatPatternType& otherPattern)
{
    pattern = otherPattern;
}

void MetronomeBeats::fromString(const std::string& strPattern)
{
    pattern.clear();
    for (const char& character : strPattern) {
        const auto convertedType = static_cast<BeatType>(character);

        // the following switch will ignore all non valid conversions of character to
        // MetronomeBeats::BeatType
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

std::string MetronomeBeats::toString() const
{
    std::stringstream sStream;
    for (const auto& type : pattern) {
        sStream << static_cast<char>(type);
    }
    return sStream.str();
}

const std::vector<mnome::BeatType>& MetronomeBeats::getBeatPattern() const
{
    return pattern;
}

}  // namespace mnome

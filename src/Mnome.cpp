#include "Mnome.hpp"
#include "AudioSignal.hpp"
#include "BeatPlayer.hpp"

#include <cmath>
#include <cstddef>

using namespace std;

namespace mnome {

constexpr size_t PLAYBACK_RATE    = 48'000;  // [Hz]
constexpr double TONE_A1_BASEFREQ = 440;     // [Hz]
constexpr size_t QUINT_HALFSTEPS  = 7;

Mnome::Mnome()
{
    const double normalBeatHz      = halfToneOffset(TONE_A1_BASEFREQ, 2);            // base tone = B
    const double accentuatedBeatHz = halfToneOffset(normalBeatHz, QUINT_HALFSTEPS);  // base tone + quint
    constexpr double beatDuration  = 0.05;                                           // [s]
    constexpr uint8_t overtones    = 1;
    AudioSignalConfiguration audioConfig{PLAYBACK_RATE, 1};
    ToneConfiguration toneConfigNormal{beatDuration, normalBeatHz, overtones};
    ToneConfiguration toneConfigAccentuated{beatDuration, accentuatedBeatHz, overtones};

    // generate the beat
    const auto accentuatedBeat = generateTone(audioConfig, toneConfigAccentuated);
    const auto normalBeat      = generateTone(audioConfig, toneConfigNormal);

    bp.setBeat(normalBeat);
    bp.setAccentuatedBeat(accentuatedBeat);
    bp.setAccentuatedPattern(MetronomeBeats("!+++"));

    // bind keywords to function callbacks
    ReplCommandList commands;
    commands.emplace("exit", bind(&Mnome::stop, this));
    commands.emplace("quit", bind(&Mnome::stop, this));
    commands.emplace("start", bind(&Mnome::startPlayback, this));
    commands.emplace("stop", bind(&Mnome::stopPlayback, this));
    commands.emplace("bpm", bind(&Mnome::setBPM, this, std::placeholders::_1));
    commands.emplace("pattern", bind(&Mnome::setBeatPattern, this, std::placeholders::_1));
    // make ENTER start and stop
    commands.emplace("", bind(&Mnome::togglePlayback, this));

    repl.setCommands(commands);
    repl.start();
}

void Mnome::stop()
{
    lock_guard<mutex> lockGuard(cmdMtx);
    bp.stop();
    repl.stop();
}

/// Wait for the read evaluate loop to finish
void Mnome::waitForStop()
{
    repl.waitForStop();
}

void Mnome::stopPlayback()
{
    lock_guard<mutex> lockGuard(cmdMtx);
    bp.stop();
}
void Mnome::startPlayback()
{
    lock_guard<mutex> lockGuard(cmdMtx);
    bp.start();
}
void Mnome::togglePlayback()
{
    lock_guard<mutex> lockGuard(cmdMtx);
    if (bp.isRunning()) {
        bp.stop();
    }
    else {
        bp.start();
    }
}
void Mnome::setBPM(const std::optional<std::string> args)
{
    auto displayHelp = []() { cout << "Command usage: bpm <number>" << endl; };
    lock_guard<mutex> lockGuard(cmdMtx);
    if (args) {
        const string bpmStr = args.value();
        try {
            size_t bpm = !bpmStr.empty() ? stoul(bpmStr) : DEFAULT_BPM;
            bp.setBPM(bpm);
        }
        catch (exception& e) {
            cout << "Could get beats per minute from \"" << bpmStr << "\"" << endl;
            displayHelp();
        };
    }
    else {
        displayHelp();
    }
}
void Mnome::setBeatPattern(const std::optional<std::string> args)
{
    auto displayHelp = []() {
        cout << "Command usage: pattern <pattern>" << endl;
        cout << "  <pattern> must be in the form of `[!|+|.]*`" << endl;
        cout << "  `!` = accentuated beat  `+` = normal beat  `.` = pause" << endl;
    };
    lock_guard<mutex> lockGuard(cmdMtx);
    if (args) {
        const string patternStr = args.value();
        if ((patternStr.find('*') == string::npos) && (patternStr.find('+') == string::npos)) {
            displayHelp();
            return;
        }
        bp.setAccentuatedPattern(MetronomeBeats(patternStr));
    }
    else {
        displayHelp();
    }
}

bool Mnome::isPlaying() const
{
    return bp.isRunning();
}

}  // namespace mnome

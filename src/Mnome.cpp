#include "Mnome.hpp"
#include "AudioSignal.hpp"
#include "BeatPlayer.hpp"

#include <cmath>
#include <cstddef>

using namespace std;

namespace mnome {

constexpr size_t PLAYBACK_RATE = 48'000;  // [Hz]

Mnome::Mnome() : bp{80}
{
    double normalBeatHz      = halfToneOffset(440, 2);           // base tone = B
    double accentuatedBeatHz = halfToneOffset(normalBeatHz, 7);  // base tone + quint
    uint8_t overtones        = 4;
    AudioSignalConfiguration audioConfig{PLAYBACK_RATE, 1};
    ToneConfiguration toneConfigNormal{0.0750, normalBeatHz, overtones};
    ToneConfiguration toneConfigAccentuated{0.0750, accentuatedBeatHz, overtones};

    // generate the beat
    auto accentuatedBeat = generateTone(audioConfig, toneConfigAccentuated);
    auto normalBeat      = generateTone(audioConfig, toneConfigNormal);

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
    lock_guard<mutex> lg(cmdMtx);
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
    lock_guard<mutex> lg(cmdMtx);
    bp.stop();
}
void Mnome::startPlayback()
{
    lock_guard<mutex> lg(cmdMtx);
    bp.start();
}
void Mnome::togglePlayback()
{
    lock_guard<mutex> lg(cmdMtx);
    if (bp.isRunning()) {
        bp.stop();
    }
    else {
        bp.start();
    }
}
void Mnome::setBPM(const string& bpmStr)
{
    lock_guard<mutex> lg(cmdMtx);
    try {
        size_t bpm = !bpmStr.empty() ? stoul(bpmStr, nullptr, 10) : 80;
        bp.setBPM(bpm);
    }
    catch (exception& e) {
        cout << "Could get beats per minute from \"" << bpmStr << "\"" << endl;
    };
}
void Mnome::setBeatPattern(const string& patternStr)
{
    lock_guard<mutex> lg(cmdMtx);
    if ((patternStr.find('*') == string::npos) && (patternStr.find('+') == string::npos)) {
        cout << "Command usage: pattern <pattern>" << endl;
        cout << "  <pattern> must be in the form of `[!|+|.]*`" << endl;
        cout << "  `!` = accentuated beat  `+` = normal beat  `.` = pause" << endl;
        return;
    }
    bp.setAccentuatedPattern(MetronomeBeats(patternStr));
}

bool Mnome::isPlaying() const
{
    return bp.isRunning();
}

}  // namespace mnome

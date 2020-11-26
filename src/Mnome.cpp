#include "Mnome.hpp"

#include <cmath>

using namespace std;

namespace mnome {

Mnome::Mnome() : bp{80}
{
    auto halfToneOffset = [](double baseFreq, size_t offset) -> double {
        return baseFreq * pow(pow(2, 1 / 12.0), offset);
    };
    double normalBeatHz      = halfToneOffset(440, 2);           // base tone = B
    double accentuatedBeatHz = halfToneOffset(normalBeatHz, 7);  // base tone + quint
    // generate the beat
    vector<TBeatDataType> beatData{generateTone(normalBeatHz, 0.0750, 4)};
    vector<TBeatDataType> accentuatedBeat{generateTone(accentuatedBeatHz, 0.0750, 4)};

    bp.setBeat(beatData);
    bp.setAccentuatedBeat(accentuatedBeat);
    bp.setAccentuatedPattern(vector<BeatPattern::BeatType>{BeatPattern::accent, BeatPattern::beat,
                                                           BeatPattern::beat, BeatPattern::beat});

    // set commands for the repl
    ReplCommandList commands;
    // make ENTER start and stop
    commands.emplace("", bind(&Mnome::togglePlayback, this));
    commands.emplace("exit", bind(&Mnome::stop, this));
    commands.emplace("quit", bind(&Mnome::stop, this));
    commands.emplace("start", bind(&Mnome::startPlayback, this));
    commands.emplace("stop", bind(&Mnome::stopPlayback, this));
    commands.emplace("bpm", bind(&Mnome::setBPM, this, std::placeholders::_1));
    commands.emplace("pattern", bind(&Mnome::setBeatPattern, this, std::placeholders::_1));

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
    BeatPattern pattern{};
    pattern.fromString(patternStr);
    bp.setAccentuatedPattern(pattern);
}

bool Mnome::isPlaying() const
{
    return bp.isRunning();
}

}  // namespace mnome

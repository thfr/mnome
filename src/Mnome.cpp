#include "Mnome.hpp"
#include "AudioSignal.hpp"
#include "BeatPlayer.hpp"

#include "Repl.hpp"
#include "doctest.h"

#include <cstddef>
#include <format>
#include <print>
#include <sstream>
#include <string_view>
#include <utility>

using namespace std;

namespace mnome {
using std::string_view;


constexpr size_t PLAYBACK_RATE    = 48'000;  // [Hz]
constexpr double TONE_A1_BASEFREQ = 440;     // [Hz]
constexpr size_t QUINT_HALFSTEPS  = 7;

Mnome::Mnome()
{
    // generate tone configurations
    const auto     normalBeatHz      = halfToneOffset(TONE_A1_BASEFREQ, 2);            // base tone = B
    const auto     accentuatedBeatHz = halfToneOffset(normalBeatHz, QUINT_HALFSTEPS);  // base tone + quint
    constexpr auto beatDuration      = 0.05;                                           // [s]
    constexpr auto overtones         = 1;

    const auto toneConfigNormal = ToneConfiguration{
        .length    = beatDuration,
        .frequency = normalBeatHz,
        .overtones = overtones,
    };
    const auto toneConfigAccentuated = ToneConfiguration{
        .length    = beatDuration,
        .frequency = accentuatedBeatHz,
        .overtones = overtones,
    };

    // generate the beat
    const auto audioConfig = AudioSignalConfiguration{
        .sampleRate = PLAYBACK_RATE,
        .channels   = 1,
    };
    const auto accentuatedBeat = generateTone(audioConfig, toneConfigAccentuated);
    const auto normalBeat      = generateTone(audioConfig, toneConfigNormal);

    bp.setBeat(normalBeat);
    bp.setAccentuatedBeat(accentuatedBeat);
    bp.setAccentuatedPattern(MetronomeBeats("!+++"));

    // bind keywords to function callbacks
    ReplCommandList commands;
    commands.emplace(
        "exit",
        ReplCommand{.function = [this](string_view) -> void { stop(); }, .name = "exit", .help = "Exit program"});
    commands.emplace(
        "quit",
        ReplCommand{.function = [this](string_view) -> void { stop(); }, .name = "quit", .help = "Exit program"});
    commands.emplace("start", ReplCommand{.function = [this](string_view) -> void { startPlayback(); },
                                          .name     = "start",
                                          .help     = "Start playback"});
    commands.emplace("stop", ReplCommand{.function = [this](string_view) -> void { stopPlayback(); },
                                         .name     = "stop",
                                         .help     = "Stop playback"});
    commands.emplace("bpm", ReplCommand{.function = [this](string_view args) -> void { setBPM(args); },
                                        .name     = "bpm",
                                        .help     = "Set the bpm to an integer value"});
    commands.emplace("pattern",
                     ReplCommand{.function = [this](string_view args) -> void { setBeatPattern(args); },
                                 .name     = "pattern",
                                 .help = std::format("Command usage: pattern <pattern>\n"
                                                     "  <pattern> must be in the form of `[{0}|{1}|{2}]*`\n"
                                                     "  `{0}` = accentuated beat  `{1}` = normal beat  `{2}` = pause",
                                                     BeatType::accent, BeatType::accent, BeatType::pause)});
    // make ENTER start and stop
    commands.emplace("", ReplCommand{.function = [this](string_view) -> void { togglePlayback(); },
                                     .name     = "<ENTER KEY>",
                                     .help     = "Shortcut for toggling playback"});

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
void Mnome::setBPM(std::string_view args)
{
    auto              displayHelp = []() -> void { cout << "Command usage: bpm <number>\n"; };
    lock_guard<mutex> lockGuard(cmdMtx);
    if (!args.empty()) {
        try {
            size_t bpm = stoul(string(args));
            bp.setBPM(bpm);
        }
        catch (exception& e) {
            std::println("Could get beats per minute from \"{}\"", args);
            displayHelp();
        };
    }
    else {
        displayHelp();
    }
}
void Mnome::setBeatPattern(std::string_view args)
{
    auto displayHelp = []() -> void {
        cout << std::format("Command usage: pattern <pattern>\n"
                            "  <pattern> must be in the form of `[{0}|{1}|{2}]*`\n"
                            "  `{0}` = accentuated beat  `{1}` = normal beat  `{2}` = pause\n",
                            BeatType::accent, BeatType::accent, BeatType::pause);
    };
    lock_guard<mutex> lockGuard(cmdMtx);
    if (!args.empty()) {
        if (args.contains(to_underlying(BeatType::accent)) && args.contains(to_underlying(BeatType::beat))) {
            displayHelp();
            return;
        }
        bp.setAccentuatedPattern(MetronomeBeats(args));
    }
    else {
        displayHelp();
    }
}

auto Mnome::isPlaying() const -> bool
{
    return bp.isRunning();
}

// NOLINTNEXTLINE
TEST_CASE("MnomeTest - ChangeSettingsDuringPlayback")
{
    const auto   waitTime = std::chrono::milliseconds(10);
    stringstream stringStream;

    streambuf* cinbuf = cin.rdbuf();
    cin.rdbuf(stringStream.rdbuf());
    Mnome app{};
    CHECK_NOTHROW(app.startPlayback());
    CHECK(app.isPlaying());

    this_thread::sleep_for(waitTime);

    CHECK_NOTHROW(app.stopPlayback());
    CHECK_NOTHROW(app.startPlayback());

    this_thread::sleep_for(waitTime);

    CHECK_NOTHROW(app.setBeatPattern("!+.+"));

    this_thread::sleep_for(waitTime);

    CHECK(app.isPlaying());
    CHECK_NOTHROW(app.stop());

    this_thread::sleep_for(waitTime);

    stringStream << "exit\n";
    stringStream.flush();
    cin.rdbuf(cinbuf);
}


}  // namespace mnome

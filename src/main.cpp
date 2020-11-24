/// Mnome - A metronome program

#include "BeatPlayer.hpp"
#include "Repl.hpp"

#include <cmath>
#include <csignal>
#include <memory>
#include <mutex>


using namespace std;


namespace mnome {

/// The metronome object
class Mnome
{
    BeatPlayer bp;
    Repl repl;

public:
    /// Ctor
    Mnome() : bp{80}
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
        bp.setAccentuatedPattern(vector<BeatPattern::BeatType>{
            BeatPattern::accent, BeatPattern::beat, BeatPattern::beat, BeatPattern::beat});

        // set commands for the repl
        ReplCommandList commands;
        // make ENTER start and stop
        commands.emplace("", [this](string&) {
            if (bp.isRunning()) {
                this->bp.stop();
            }
            else {
                this->bp.start();
            }
        });
        commands.emplace("exit", [this](string&) { this->repl.stop(); });
        commands.emplace("quit", [this](string&) { this->repl.stop(); });
        commands.emplace("start", [this](string&) { this->bp.start(); });
        commands.emplace("stop", [this](string&) { this->bp.stop(); });
        commands.emplace("bpm", [this](string& s) {
            size_t bpm = !s.empty() ? stoul(s, nullptr, 10) : 80;
            this->bp.setBPM(bpm);
        });
        commands.emplace("pattern", [this](string& s) {
            if ((s.find('*') == string::npos) && (s.find('+') == string::npos)) {
                cout << "Command usage: pattern <pattern>" << endl;
                cout << "  <pattern> must be in the form of `[!|+|.]*`" << endl;
                cout << "  `!` = accentuated beat  `+` = normal beat  `.` = pause" << endl;
                return;
            }
            BeatPattern pattern{};
            pattern.fromString(s);
            this->bp.setAccentuatedPattern(pattern);
        });


        repl.setCommands(commands);
        repl.start();
    }

    void stop()
    {
        bp.stop();
        repl.stop();
    }

    /// Wait for the read evaluate loop to finish
    void waitForStop() const { repl.waitForStop(); }
};

}  // namespace mnome


namespace {
mutex AppMutex;
unique_ptr<mnome::Mnome> App;
}  // namespace


void shutDownAppHandler(int)
{
    lock_guard<mutex> lg{AppMutex};
    if (App) {
        App->stop();
    }
}

int main()
{
    signal(SIGINT, shutDownAppHandler);
    signal(SIGTERM, shutDownAppHandler);
    signal(SIGABRT, shutDownAppHandler);

    {
        lock_guard<mutex> lg{AppMutex};
        App = make_unique<mnome::Mnome>();
    }

    // no mutex needed, member function is const
    App->waitForStop();

    return 0;
}

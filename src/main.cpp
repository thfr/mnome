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
      vector<TBeatDataType> beatData{generateInt16Sine(normalBeatHz, 0.050)};
      vector<TBeatDataType> accentuatedBeat{generateInt16Sine(accentuatedBeatHz, 0.050)};

      bp.setBeat(beatData);
      bp.setAccentuatedBeat(accentuatedBeat);
      bp.setAccentuatedPattern(vector<bool>{true, false, false, false});

      // set commands for the repl
      commandlist_t commands;
      // make ENTER start and stop
      commands.emplace("", [this](string&) {
         if (bp.isRunning()) {
            this->bp.stop();
            this->bp.waitForStop();
         }
         else {
            this->bp.start();
         }
      });
      commands.emplace("exit", [this](string&) { this->repl.stop(); });
      commands.emplace("start", [this](string&) { this->bp.start(); });
      commands.emplace("stop", [this](string&) {
         this->bp.stop();
         this->bp.waitForStop();
      });
      commands.emplace("bpm", [this](string& s) {
         size_t bpm = !s.empty() ? stoul(s, nullptr, 10) : 80;
         this->bp.setBPM(bpm);
      });
      commands.emplace("pattern", [this](string& s) {
         if ((s.find('*') == string::npos) && (s.find('+') == string::npos)) {
            return;
         }
         vector<bool> pattern;
         for (char c : s) {
            if (c == '*') {
               pattern.push_back(true);
            }
            else if (c == '+') {
               pattern.push_back(false);
            }
         }
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

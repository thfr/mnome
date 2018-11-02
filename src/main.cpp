/// Mnome - A metronome program

#include "BeatPlayer.hpp"
#include "Repl.hpp"


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
      // generate the beat
      vector<TBeatDataType> beatData;
      generateInt16Sine(beatData, 500, 0.040);
      bp.setBeat(beatData);

      // set commands for the repl
      commandlist_t commands;
      commands.emplace("exit", [this](string&) { this->repl.stop(); });
      commands.emplace("start", [this](string&) { this->bp.start(); });
      commands.emplace("stop", [this](string&) {
         this->bp.stop();
         this->bp.waitForStop();
      });
      commands.emplace("bpm", [this](string& s) {
         size_t bpm      = !s.empty() ? stoul(s, nullptr, 10) : 80;
         bool wasRunning = bp.isRunning();
         if (wasRunning) {
            this->bp.stop();
            this->bp.waitForStop();
         }
         this->bp.setBPM(bpm);
         if (wasRunning) {
            this->bp.start();
         }
      });


      repl.setCommands(commands);
      repl.start();
   }

   /// Wait for the read evaluate loop to finish
   void waitForStop() const { repl.waitForStop(); }
};

}  // namespace mnome


int main()
{
   mnome::Mnome mnome{};
   mnome.waitForStop();

   return 0;
}

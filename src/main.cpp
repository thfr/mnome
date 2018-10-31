/// Mnome - A metronome program
///
/// Main file
///
/// \file main.cpp


#include <alsa/asoundlib.h>

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <vector>


using namespace std;


constexpr size_t PLAYBACK_RATE           = 48'000;  // [Hz]
constexpr double PLAYBACK_MIN_ALSA_WRITE = 1.0;     // 100 ms
constexpr double FADE_MIN_TIME           = 0.01;    // 10 ms
constexpr double FADE_MIN_PERCENTAGE     = 0.30;
constexpr double PI                      = 3.141592653589793;

static const char* PLAYBACK_ALSA_DEVICE = "default";


namespace mnome {

using TBeatDataType = int16_t;

/// Fade a signal in and out
/// \tparam  T  datatype of the signal
/// \param  data  signal to be faded
/// \param  fadeInSamples  number of samples on which the fading in is done
/// \param  fadeOutSamples  number of samples on which the fading out is done
template <typename T>
void fadeInOut(vector<T>& data, size_t fadeInSamples, size_t fadeOutSamples)
{
   // *Exponential fading* is used because it is more pleasant to ear than linear fading.
   //
   // A factor with changing value is multiplied to each sample of the fading period.
   // The factor must be increased by multiplying it with a constant ratio that. Therefore the
   // factor must have a starting value > 0.0 .
   //    fs * (r ** steps) = 1
   //    r ** steps  = 1 / fs
   //    r = (1 / fs) ** (1 / steps)
   //       where fs = factor at start
   //              r = ratio
   double startValue  = 2.0 / INT16_MAX;
   double fadeInRatio = pow(1.0 / startValue, 1.0 / fadeInSamples);
   double factor      = startValue;

   // apply all factors for the ramp in
   for (size_t samIdx = 0; samIdx < fadeInSamples; ++samIdx) {
      data[samIdx] *= factor;
      factor *= fadeInRatio;
   };

   // apply all factors for the ramp out
   double fadeOutRatio = 1.0 / pow(1.0 / startValue, 1.0 / fadeOutSamples);
   size_t fadeOutIndex = max(static_cast<size_t>(0), data.size() - fadeOutSamples);
   factor              = fadeOutRatio;
   for (size_t samIdx = fadeOutIndex; samIdx < data.size(); ++samIdx) {
      data[samIdx] *= factor;
      factor *= fadeOutRatio;
   };
}


/// Generate a sine signal
/// \param[out]  data     samples to be generated
/// \param[in]   freq     tone frequency
/// \param[in]   lengthS  length in seconds
void generateInt16Sine(vector<int16_t>& data, const size_t freq, const double lengthS)
{
   size_t samples = static_cast<size_t>(floor(PLAYBACK_RATE * lengthS));

   data.clear();
   data.reserve(samples);

   for (size_t samIdx = 0; samIdx < samples; samIdx++) {
      double sample = sin(samIdx * 2 * PI * freq / PLAYBACK_RATE);
      data.emplace_back(static_cast<int16_t>(INT16_MAX * 0.75 * sample));
   }
}

/// Return the time it takes to playback a number of samples
/// \param[in]  samples  number of samples
double getDuration(size_t samples) { return static_cast<double>(samples) / PLAYBACK_RATE; }

/// Return the time it takes to playback a number of samples
/// \param[in]  time  time in seconds
/// \return     samples  number of samples that resemble \p time samples
size_t getNumbersOfSamples(double time) { return static_cast<size_t>(round(time * PLAYBACK_RATE)); }


/// Trim first spaces of a string
/// \note see https://stackoverflow.com/a/217605
static inline std::string& ltrim(std::string& s)
{
   s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int c) { return !std::isspace(c); }));
   return s;
}

/// Trim trailing spaces of a string
/// \note see https://stackoverflow.com/a/217605
static inline void rtrim(std::string& s)
{
   s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) { return !std::isspace(ch); }).base(),
           s.end());
}


/// Thread that plays the beat
class BeatPlayer
{
   size_t beatRate;
   mutex setterMutex;
   vector<TBeatDataType> beat;
   vector<TBeatDataType> playBackBuffer;
   std::unique_ptr<thread> myThread;
   atomic_bool requestStop;

public:
   /// Constructor
   /// \param  beatRate  rate of the beat in [bpm]
   /// \param  beat      raw data of one beat
   BeatPlayer(const size_t beatRate)
      : beatRate(beatRate), beat{}, playBackBuffer{}, myThread{nullptr}, requestStop{false}
   {
   }

   ~BeatPlayer()
   {
      stop();
      waitForStop();
   }

   /// Set the sound of the beat that is played back
   void setBeat(const vector<TBeatDataType>& newBeat)
   {
      beat.clear();
      beat = newBeat;
   }

   /// Start the BeatPlayer
   void start()
   {
      size_t beatIntervalSamples;
      vector<TBeatDataType> playBackBuffer;
      vector<TBeatDataType> localBeat;

      // lock mutex to have stable settings
      {
         lock_guard<mutex> guard(setterMutex);

         cout << "Playing at " << beatRate << " bpm" << endl;

         // create the playback buffer so that it contains one beat and
         // the correct amount of silence before the next beat starts
         beatIntervalSamples = static_cast<size_t>(floor(1.0 / (beatRate / 60.0) * PLAYBACK_RATE));
         localBeat           = beat;
      }
      assert(localBeat.size() > 0);
      double lengthS = getDuration(localBeat.size());

      // Fade the beat in and out to avoid click/pop noises because of too sudden output value
      // changes: First 2 ms is fading in, last 2 ms is fading out If the beat length is less then 8
      // ms use 25% each for fading in and out
      size_t rampingSteps = static_cast<size_t>(
         (lengthS < FADE_MIN_TIME)
            ? floor(static_cast<double>(PLAYBACK_RATE) * lengthS * FADE_MIN_PERCENTAGE)
            : floor(static_cast<double>(PLAYBACK_RATE) * FADE_MIN_TIME * FADE_MIN_PERCENTAGE));

      // Since fadeInOut only uses the first x and last y samples,
      // make sure not to fade out zero values
      if (localBeat.size() > beatIntervalSamples) {
         localBeat.resize(beatIntervalSamples, 0);
         fadeInOut(localBeat, rampingSteps, rampingSteps);
      }
      else {
         fadeInOut(localBeat, rampingSteps, rampingSteps);
         localBeat.resize(beatIntervalSamples, 0);
      }

      // check for length: the playback buffer should be at least 1000 ms
      if (localBeat.size() < getNumbersOfSamples(PLAYBACK_MIN_ALSA_WRITE)) {
         size_t numberOfCopies = static_cast<size_t>(
            ceil(getNumbersOfSamples(PLAYBACK_MIN_ALSA_WRITE) / localBeat.size()));

         playBackBuffer.reserve(numberOfCopies * localBeat.size());
         auto playBackBufferIterator = back_inserter(playBackBuffer);
         for (size_t copyIdx = 0; copyIdx < numberOfCopies; ++copyIdx) {
            playBackBufferIterator =
               copy(localBeat.begin(), localBeat.end(), playBackBufferIterator);
         }
      }
      else {
         playBackBuffer.reserve(localBeat.size());
         copy(localBeat.begin(), localBeat.end(), back_inserter(playBackBuffer));
      }

      // start the thread
      waitForStop();
      myThread.reset(new thread([this, playBackBuffer]() {
         requestStop = false;

         int err;
         snd_pcm_t* handle;
         snd_pcm_sframes_t frames;
         bool initSuccess = true;

         err = snd_pcm_open(&handle, PLAYBACK_ALSA_DEVICE, SND_PCM_STREAM_PLAYBACK, 0);
         if (err < 0) {
            initSuccess = false;
         }
         else {
            err = snd_pcm_set_params(handle, SND_PCM_FORMAT_S16, SND_PCM_ACCESS_RW_INTERLEAVED, 1,
                                     PLAYBACK_RATE, 1, 500'000);
            if (err < 0) {
               initSuccess = false;
            }
         }

         if (initSuccess) {
            while (!requestStop) {
               frames = snd_pcm_writei(handle, playBackBuffer.data(), playBackBuffer.size());
               if (frames < 0) {
                  frames = snd_pcm_recover(handle, static_cast<int>(frames), 0);
               }
               if (frames < 0) {
                  cout << "snd_pcm_writei failed:" << snd_strerror(static_cast<int>(frames))
                       << "\n";
                  break;
               }
               if (frames > 0 &&
                   frames < static_cast<snd_pcm_sframes_t>(sizeof(playBackBuffer.data())))
                  cout << "Short write (expected " << sizeof(playBackBuffer.data()) << " wrote "
                       << frames << ")\n";
            }
         }
         else {
            cout << "Playback open error: " << snd_strerror(err) << endl;
         }

         snd_pcm_close(handle);
         requestStop = false;
      }));
   }

   /// Stop the beat playback
   /// \note Does not block
   void stop()
   {
      if (isRunning()) {
         requestStop = true;
      }
   }

   /// Change the beat per minute
   /// @param  bpm  beats per minute
   void setBPM(size_t bpm)
   {
      lock_guard<mutex> guard(setterMutex);
      beatRate = bpm;
   }

   size_t getBPM() { return beatRate; }

   /// Change the beat that is played back
   /// @param  beatData  The beat that is played back
   void setData(const vector<TBeatDataType>& beatData)
   {
      lock_guard<mutex> guard(setterMutex);
      beat = beatData;
   }

   /// Change the beat and the beats per minute, for convenience
   /// @param  beatData  The beat that is played back
   /// @param  bpm  beats per minute
   void setDataAndBPM(const vector<TBeatDataType>& beatData, size_t bpm)
   {
      lock_guard<mutex> guard(setterMutex);
      beat     = beatData;
      beatRate = bpm;
   }

   /// Indicates whether the thread is running
   bool isRunning() const { return myThread && myThread->joinable(); }

   /// Wait for the REPL thread to finish
   void waitForStop() const
   {
      if (isRunning()) {
         myThread->join();
      }
   }
};


using commandlist_t = unordered_map<string, function<void(string&)>>;

/// Read evaluate print loop
class Repl
{
   commandlist_t commands;
   std::unique_ptr<thread> myThread;
   atomic_bool requestStop;

public:
   class ForbiddenCommandExecption : exception
   {
   };

   /// Ctor with empty command list
   Repl() : commands{}, myThread{nullptr}, requestStop{false} {}

   /// Ctor with command list
   Repl(commandlist_t& cmds) : commands{cmds}, myThread{nullptr}, requestStop{false} {}

   ~Repl()
   {
      stop();
      waitForStop();
   }

   /// Set the commands that the REPL should recognize
   /// \note Do not forget the exit/quit command
   void setCommands(const commandlist_t& cmds) { commands = cmds; }

   /// Start the read evaluate print loop
   void start()
   {
      waitForStop();
      myThread.reset(new thread([this]() { this->run(); }));
   }

   /// Stops the read evaluate print loop
   /// \note Does not block
   void stop()
   {
      if (isRunning()) {
         requestStop = true;
      }
   }

   /// Indicates whether the thread is running
   bool isRunning() const { return myThread && myThread->joinable(); }

   /// Make sure the thread has stopped
   /// \note blocks until thread is finished
   void waitForStop() const
   {
      if (isRunning()) {
         myThread->join();
      }
   }

private:
   /// The method that the thread runs
   void run()
   {
      string input;
      while (!requestStop) {
         // prompt
         cout << endl;
         cout << "[mnome]: ";

         getline(cin, input);
         rtrim(ltrim(input));

         if (input.size() == 0) {
            continue;
         }
         size_t cmdSep = input.find(" ", 0);

         // find command in command list
         string commandString = input.substr(0, cmdSep);
         auto possibleCommand = commands.find(commandString);
         if (end(commands) == possibleCommand) {
            cout << "\"" << commandString << "\" is not a valid command" << endl;
            continue;
         }

         try {
            // execute command with parameters
            string args = (cmdSep != input.npos) ? input.substr(cmdSep + 1) : "";
            possibleCommand->second(args);
         }
         catch (const std::exception& e) {  // reference to the base of a polymorphic object
            cout << "Could not get that, please try again" << endl;
            cout << e.what();  // information from length_error printed
         }
      }
      requestStop = false;
   }
};


/// The metronome object
class Mnome
{
   BeatPlayer bp;
   Repl repl;

public:
   /// Ctor
   Mnome() : bp{80}, repl{}
   {
      vector<TBeatDataType> beatData;
      generateInt16Sine(beatData, 500, 0.040);
      bp.setBeat(beatData);

      commandlist_t commands;
      commands.emplace("exit", [this](string&) { this->repl.stop(); });
      commands.emplace("start", [this](string&) { this->bp.start(); });
      commands.emplace("stop", [this](string&) { this->bp.stop(); });
      commands.emplace("bpm", [this](string& s) {
         size_t bpm      = s.size() > 0 ? stoul(s) : 80;
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
   using namespace mnome;

   Mnome mnome{};
   mnome.waitForStop();

   return 0;
}

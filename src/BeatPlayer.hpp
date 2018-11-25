/// BeatPlayer
///
/// Plays a beat

#ifndef MNOME_BEATPLAYER_H
#define MNOME_BEATPLAYER_H

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>


using namespace std;


namespace mnome {

using TBeatDataType = int16_t;


/// Plays a beat at a certain number of times per minute
class BeatPlayer
{
   size_t beatRate;
   mutex setterMutex;
   vector<TBeatDataType> beat;
   vector<TBeatDataType> accentuatedBeat;
   vector<TBeatDataType> playBackBuffer;
   vector<bool> accentuatedPattern;
   std::unique_ptr<thread> myThread;
   atomic_bool requestStop;

public:
   /// Constructor
   /// \param  beatRate  rate of the beat in [bpm]
   /// \param  beat      raw data of one beat
   BeatPlayer(size_t bRate);

   ~BeatPlayer();

   /// Set the sound of the beat that is played back
   /// \param  newBeat  samples the represent the beat
   void setBeat(const vector<TBeatDataType>& newBeat);

   /// Set the sound of the beat that is played back
   /// \param  newBeat  samples the represent the accentuated beat
   void setAccentuatedBeat(const vector<TBeatDataType>& newBeat);

   void setAccentuatedPattern(const vector<bool>& pattern);

   /// Start the BeatPlayer
   void start();

   /// Stop the beat playback
   /// \note Does not block
   void stop();

   /// Change the beat per minute
   /// \param  bpm  beats per minute
   void setBPM(size_t bpm);

   /// Get the current bpm setting
   size_t getBPM() const;

   /// Change the beat that is played back
   /// \param  beatData  The beat that is played back
   void setAccentuatedBeatData(const vector<TBeatDataType>& beatData);

   /// Change the beat and the beats per minute, for convenience
   /// \param  beatData  The beat that is played back
   /// \param  bpm  beats per minute
   void setDataAndBPM(const vector<TBeatDataType>& beatData, size_t bpm);

   /// Indicates whether the thread is running
   bool isRunning() const;

   /// Wait for the REPL thread to finish
   void waitForStop() const;

private:
   /// Thread method
   void run();

   /// Restart the thread if was started
   void restartNecessary();
};


/// Generate a sine signal
/// \param[out]  data     samples to be generated
/// \param[in]   freq     tone frequency
/// \param[in]   lengthS  length in seconds
vector<int16_t> generateInt16Sine(const double freq, const double lengthS);

}  // namespace mnome


#endif  //  MNOME_BEATPLAYER_H

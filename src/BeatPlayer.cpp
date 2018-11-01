/// BeatPlayer
///
/// Plays a beat

#include "BeatPlayer.hpp"

#include <alsa/asoundlib.h>

#include <atomic>
#include <cassert>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>


using namespace std;


constexpr size_t PLAYBACK_RATE           = 48'000;  // [Hz]
constexpr double PLAYBACK_MIN_ALSA_WRITE = 0.1;     // 100 ms
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
template <typename TSample>
void fadeInOut(vector<TSample>& data, size_t fadeInSamples, size_t fadeOutSamples)
{
   // *Exponential Fading* is used because it is more pleasant to ear than linear fading.
   //
   // A factor with changing value is multiplied to each sample of the fading period.
   // The factor must be increased by multiplying it with a constant ratio that. Therefore the
   // factor must have a starting value > 0.0 .
   //    fs * (r ** steps) = 1         (discrete form: f[n+1] = f[n] * r , while f[n+1] <= 1)
   //    r ** steps  = 1 / fs
   //    r = (1 / fs) ** (1 / steps)
   //       where fs = factor at start
   //              r = ratio

   // apply all factors for the fade in
   double startValue  = 2.0 / INT16_MAX;
   double fadeInRatio = pow(1.0 / startValue, 1.0 / fadeInSamples);
   double factor      = startValue;
   for (size_t samIdx = 0; samIdx < fadeInSamples; ++samIdx) {
      data[samIdx] *= factor;
      factor *= fadeInRatio;
   };

   // apply all factors for the fade out
   double fadeOutRatio = 1.0 / pow(1.0 / startValue, 1.0 / fadeOutSamples);
   size_t fadeOutIndex = max(static_cast<size_t>(0), data.size() - fadeOutSamples);
   factor              = fadeOutRatio;
   for (size_t samIdx = fadeOutIndex; samIdx < data.size(); ++samIdx) {
      data[samIdx] *= factor;
      factor *= fadeOutRatio;
   };
}


/// Return the time it takes to playback a number of samples
/// \param[in]  samples  number of samples
double getDuration(size_t samples) { return static_cast<double>(samples) / PLAYBACK_RATE; }


/// Return the time it takes to playback a number of samples
/// \param[in]  time  time in seconds
/// \return     samples  number of samples that resemble \p time samples
size_t getNumbersOfSamples(double time) { return static_cast<size_t>(round(time * PLAYBACK_RATE)); }


void generateInt16Sine(vector<int16_t>& data, const size_t freq, const double lengthS)
{
   auto samples = static_cast<size_t>(floor(PLAYBACK_RATE * lengthS));

   data.clear();
   data.reserve(samples);

   for (size_t samIdx = 0; samIdx < samples; samIdx++) {
      double sample = sin(samIdx * 2 * PI * freq / PLAYBACK_RATE);
      data.emplace_back(static_cast<int16_t>(INT16_MAX * 0.75 * sample));
   }
}


BeatPlayer::BeatPlayer(const size_t beatRate)
   : beatRate(beatRate),  myThread{nullptr}, requestStop{false}
{
}

BeatPlayer::~BeatPlayer()
{
   stop();
   waitForStop();
}

void BeatPlayer::setBeat(const vector<TBeatDataType>& newBeat)
{
   beat.clear();
   beat = newBeat;
}

void BeatPlayer::start()
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
   assert(!localBeat.empty());
   double lengthS = getDuration(localBeat.size());

   // Fade the beat in and out to avoid click/pop noises because of too sudden output value
   // changes: First 2 ms is fading in, last 2 ms is fading out If the beat length is less then 8
   // ms use 25% each for fading in and out
   auto rampingSteps = static_cast<size_t>(
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
      auto numberOfCopies =
         static_cast<size_t>(ceil(getNumbersOfSamples(PLAYBACK_MIN_ALSA_WRITE) / localBeat.size()));

      playBackBuffer.reserve(numberOfCopies * localBeat.size());
      auto playBackBufferIterator = back_inserter(playBackBuffer);
      for (size_t copyIdx = 0; copyIdx < numberOfCopies; ++copyIdx) {
         playBackBufferIterator = copy(localBeat.begin(), localBeat.end(), playBackBufferIterator);
      }
   }
   else {
      playBackBuffer.reserve(localBeat.size());
      copy(localBeat.begin(), localBeat.end(), back_inserter(playBackBuffer));
   }

   // start the thread
   waitForStop();
   myThread = std::make_unique<thread>([this, playBackBuffer]() {
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
               cout << "snd_pcm_writei failed:" << snd_strerror(static_cast<int>(frames)) << "\n";
               break;
            }
            if (frames > 0 &&
                frames < static_cast<snd_pcm_sframes_t>(sizeof(playBackBuffer.data()))) {
               cout << "Short write (expected " << sizeof(playBackBuffer.data()) << " wrote "
                    << frames << ")\n";
}
         }
      }
      else {
         cout << "Playback open error: " << snd_strerror(err) << endl;
      }

      snd_pcm_close(handle);
      requestStop = false;
   });
}

void BeatPlayer::stop()
{
   if (isRunning()) {
      requestStop = true;
   }
}

void BeatPlayer::setBPM(size_t bpm)
{
   lock_guard<mutex> guard(setterMutex);
   beatRate = bpm;
}

size_t BeatPlayer::getBPM() const { return beatRate; }

void BeatPlayer::setData(const vector<TBeatDataType>& beatData)
{
   lock_guard<mutex> guard(setterMutex);
   beat = beatData;
}

void BeatPlayer::setDataAndBPM(const vector<TBeatDataType>& beatData, size_t bpm)
{
   lock_guard<mutex> guard(setterMutex);
   beat     = beatData;
   beatRate = bpm;
}

bool BeatPlayer::isRunning() const { return myThread && myThread->joinable(); }

void BeatPlayer::waitForStop() const
{
   if (isRunning()) {
      myThread->join();
   }
}

}  // namespace mnome

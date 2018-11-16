/// BeatPlayer
///
/// Plays a beat

#include "BeatPlayer.hpp"

#include <alsa/asoundlib.h>

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
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


BeatPlayer::BeatPlayer(const size_t brate) : beatRate(brate), myThread{nullptr}, requestStop{false}
{
}

BeatPlayer::~BeatPlayer()
{
   stop();
   waitForStop();
}

void BeatPlayer::start()
{
   if (isRunning()) {
      cout << "Error: BeatPlayer is already running, but was started again" << endl;
      return;
   }

   lock_guard<mutex> guard(setterMutex);

   playBackBuffer.clear();

   auto beatIntervalSamples  = static_cast<size_t>(floor(1.0 / (beatRate / 60.0) * PLAYBACK_RATE));
   auto localBeat            = beat;
   auto localAccentuatedBeat = accentuatedBeat;

   bool hasAccent =
      end(accentuatedPattern) != find(begin(accentuatedPattern), end(accentuatedPattern), true);

   if (localBeat.empty()) {
      cout << "Warning: the beat is silence, you will not hear anything." << endl;
   }
   if (localAccentuatedBeat.empty() && hasAccent) {
      localAccentuatedBeat = localBeat;
   }

   double lengthS = getDuration(beat.size());

   // Fade the beat in and out to avoid click/pop noises because of too sudden output value
   // changes: First 2 ms is fading in, last 2 ms is fading out If the beat length is less then 8
   // ms use 25% each for fading in and out
   auto calcRampingSteps = [](double l) -> size_t {
      return floor(PLAYBACK_RATE * l * FADE_MIN_PERCENTAGE);
   };
   auto rampingSteps =
      (lengthS < FADE_MIN_TIME) ? calcRampingSteps(lengthS) : calcRampingSteps(FADE_MIN_TIME);

   // Since fadeInOut only uses the first x and last y samples,
   // make sure not to fade out zero values
   auto adjustBuffer = [=](vector<TBeatDataType>& buffer) {
      if (buffer.size() > beatIntervalSamples) {
         buffer.resize(beatIntervalSamples, 0);
         fadeInOut(buffer, rampingSteps, rampingSteps);
      }
      else {
         fadeInOut(buffer, rampingSteps, rampingSteps);
         buffer.resize(beatIntervalSamples, 0);
      }
   };

   adjustBuffer(localBeat);
   if (hasAccent) {
      adjustBuffer(localAccentuatedBeat);

      // fill the playback buffer according to the pattern
      auto playBackBufferIterator = back_inserter(playBackBuffer);
      for (bool accent : accentuatedPattern) {
         if (accent) {
            playBackBufferIterator =
               copy(begin(localAccentuatedBeat), end(localAccentuatedBeat), playBackBufferIterator);
         }
         else {
            playBackBufferIterator = copy(begin(localBeat), end(localBeat), playBackBufferIterator);
         }
      }
   }
   else {
      playBackBuffer = localBeat;
   }

   cout << "Playing at " << beatRate << " bpm" << endl;

   // make sure join was called before potentially destroying a previous thread object
   waitForStop();
   // start the thread
   myThread = make_unique<thread>([this]() { this->run(); });
}


void BeatPlayer::run()
{
   requestStop = false;

   int pfdcount;
   int err;
   snd_pcm_t* handle;
   snd_pcm_sframes_t frames;
   unique_ptr<struct pollfd> pfd = make_unique<struct pollfd>();

   // house keeping before exiting
   auto cleanup = [&]() {
      snd_pcm_close(handle);
      requestStop = false;
   };

   // open ALSA device
   err = snd_pcm_open(&handle, PLAYBACK_ALSA_DEVICE, SND_PCM_STREAM_PLAYBACK, SND_PCM_ASYNC);
   if (err < 0) {
      cout << "Could not open device " << PLAYBACK_ALSA_DEVICE << endl;
      cleanup();
      return;
   }

   // set playback settings
   err = snd_pcm_set_params(handle, SND_PCM_FORMAT_S16, SND_PCM_ACCESS_RW_INTERLEAVED, 1,
                            PLAYBACK_RATE, 1, 500'000);
   if (err < 0) {
      cout << "Could not open device " << PLAYBACK_ALSA_DEVICE << endl;
      cleanup();
      return;
   }

   // get count and file descriptors for poll to wait for
   pfdcount = snd_pcm_poll_descriptors_count(handle);
   if (pfdcount < 0) {
      cout << "Error getting pcm poll descriptors count for " << PLAYBACK_ALSA_DEVICE << endl;
      cleanup();
      return;
   }
   err = snd_pcm_poll_descriptors(handle, pfd.get(), pfdcount);
   if (err < 0) {
      cout << "Error getting pcm poll descriptors for " << PLAYBACK_ALSA_DEVICE << endl;
      cleanup();
      return;
   }

   size_t samplesOffset  = 0;
   size_t samplesToWrite = min(getNumbersOfSamples(PLAYBACK_MIN_ALSA_WRITE), playBackBuffer.size());
   while (!requestStop) {
      // wait until next write
      uint16_t revents;
      poll(pfd.get(), pfdcount, -1);
      snd_pcm_poll_descriptors_revents(handle, pfd.get(), pfdcount, &revents);
      if (((revents & POLLERR) != 0) || ((revents & POLLOUT) == 0)) {
         cout << "Error during poll on " << PLAYBACK_ALSA_DEVICE << " occured" << endl;
         cleanup();
         return;
      }

      // delivery samples to sound buffer
      size_t samplesTillEnd = playBackBuffer.size() - samplesOffset;
      if (samplesTillEnd < samplesToWrite) {
         size_t samplesAfterWrapAround = samplesToWrite - samplesTillEnd;
         frames = snd_pcm_writei(handle, &playBackBuffer[samplesOffset], samplesTillEnd);
         frames += snd_pcm_writei(handle, &playBackBuffer[0], samplesAfterWrapAround);
      }
      else {
         frames = snd_pcm_writei(handle, &playBackBuffer[samplesOffset], samplesToWrite);
      }

      // check for errors during sample delivery
      if (frames < 0) {
         cout << "Do pcm recover" << endl;
         frames = snd_pcm_recover(handle, static_cast<int>(frames), 0);
      }
      if (frames < 0) {
         cout << "snd_pcm_writei failed:" << snd_strerror(static_cast<int>(frames)) << "\n";
         break;
      }
      if ((frames > 0) && (frames < static_cast<snd_pcm_sframes_t>(samplesToWrite))) {
         cout << "Short write (expected " << samplesToWrite << " wrote " << frames << ")\n";
      }

      // refresh written samples
      samplesOffset = (samplesOffset + samplesToWrite) % playBackBuffer.size();
   }

   cleanup();
}

void BeatPlayer::restartNecessary()
{
   if (isRunning()) {
      stop();
      waitForStop();
      start();
   }
}

void BeatPlayer::stop()
{
   if (isRunning()) {
      requestStop = true;
   }
}

void BeatPlayer::setBPM(size_t bpm)
{
   {
      lock_guard<mutex> guard(setterMutex);
      beatRate = bpm;
   }
   restartNecessary();
}

size_t BeatPlayer::getBPM() const { return beatRate; }

void BeatPlayer::setAccentuatedBeat(const vector<TBeatDataType>& newBeat)
{
   {
      lock_guard<mutex> guard(setterMutex);
      accentuatedBeat = newBeat;
   }
   restartNecessary();
}

void BeatPlayer::setBeat(const vector<TBeatDataType>& newBeat)
{
   {
      lock_guard<mutex> guard(setterMutex);
      beat = newBeat;
   }
   restartNecessary();
}

void BeatPlayer::setDataAndBPM(const vector<TBeatDataType>& beatData, size_t bpm)
{
   {
      lock_guard<mutex> guard(setterMutex);
      beat     = beatData;
      beatRate = bpm;
   }
   restartNecessary();
}

void BeatPlayer::setAccentuatedPattern(const vector<bool>& pattern)
{
   {
      lock_guard<mutex> guard(setterMutex);
      accentuatedPattern = pattern;
   }
   restartNecessary();
}

bool BeatPlayer::isRunning() const { return myThread && myThread->joinable(); }

void BeatPlayer::waitForStop() const
{
   if (isRunning()) {
      myThread->join();
   }
}

}  // namespace mnome

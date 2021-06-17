
#include <AudioSignal.hpp>

#include <gtest/gtest.h>

#include <iostream>
#include <vector>

using namespace std;

namespace mnome {

TEST(AudioSignalTest, ChangeSettingsDuringPlayback)
{
    AudioSignalConfiguration sine44hzConfig;
    sine44hzConfig.sampleRate = 48'000;
    sine44hzConfig.sampleRate = 440;
    sine44hzConfig.length = 0.1;

    AudioSignal sine44hz{sine44hzConfig};
}

}  // namespace mnome

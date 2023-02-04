#include <AudioSignal.hpp>

#include <cstddef>
#include <gtest/gtest.h>

#include <iostream>
#include <vector>

using namespace std;

namespace mnome {

TEST(AudioSignalTest, Operators)
{
    const auto sampleRate = 48'000;
    const auto channels   = 1;
    const auto sineFreq   = 440;
    const auto sineLength = 0.1;
    AudioSignalConfiguration audioConfig;
    audioConfig.sampleRate = sampleRate;
    audioConfig.channels   = channels;
    ToneConfiguration sine440hzConfig;
    sine440hzConfig.frequency = sineFreq;
    sine440hzConfig.length    = sineLength;

    AudioSignal sine44hz1 = generateTone(audioConfig, sine440hzConfig);
    AudioSignal op_plus   = sine44hz1 + sine44hz1;

    ASSERT_EQ(op_plus.getAudioData().size(), sine44hz1.getAudioData().size());

    for (size_t idx = 0; idx < op_plus.getAudioData().size(); ++idx) {
        ASSERT_EQ(op_plus.getAudioData()[idx], 2 * sine44hz1.getAudioData()[idx]);
    }

    op_plus -= sine44hz1;
    op_plus -= sine44hz1;

    for (size_t idx = 0; idx < op_plus.getAudioData().size(); ++idx) {
        ASSERT_EQ(op_plus.getAudioData()[idx], 0);
    }
}

}  // namespace mnome

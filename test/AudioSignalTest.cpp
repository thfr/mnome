#include <AudioSignal.hpp>

#include <cstddef>
#include <gtest/gtest.h>

#include <iostream>
#include <vector>

using namespace std;

namespace mnome {

TEST(AudioSignalTest, Operators)
{
    AudioSignalConfiguration audioConfig;
    audioConfig.sampleRate = 48'000;
    audioConfig.channels   = 1;
    ToneConfiguration sine44hzConfig;
    sine44hzConfig.frequency = 440;
    sine44hzConfig.length    = 0.1;

    AudioSignal sine44hz1 = generateTone(audioConfig, sine44hzConfig);
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

#include <Mnome.hpp>

#include <gtest/gtest.h>

#include <iostream>
#include <vector>

using namespace std;

namespace mnome {

TEST(MnomeTest, ChangeSettingsDuringPlayback)
{
    const auto waitTime = std::chrono::milliseconds(10);
    stringstream stringStream;

    streambuf* cinbuf = cin.rdbuf();
    cin.rdbuf(stringStream.rdbuf());
    Mnome app{};
    EXPECT_NO_THROW(app.startPlayback());
    EXPECT_TRUE(app.isPlaying());

    this_thread::sleep_for(waitTime);

    EXPECT_NO_THROW(app.stopPlayback());
    EXPECT_NO_THROW(app.startPlayback());

    this_thread::sleep_for(waitTime);

    EXPECT_NO_THROW(app.setBeatPattern("!+.+"));

    this_thread::sleep_for(waitTime);

    EXPECT_TRUE(app.isPlaying());
    EXPECT_NO_THROW(app.stop());

    this_thread::sleep_for(waitTime);

    stringStream << "exit\n";
    stringStream.flush();
    cin.rdbuf(cinbuf);
}

}  // namespace mnome

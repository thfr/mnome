
#include <Mnome.hpp>

#include <gtest/gtest.h>

#include <iostream>
#include <vector>

using namespace std;

namespace mnome {

TEST(MnomeTest, ChangeSettingsDuringPlayback)
{
    stringstream ss;
    streambuf* cinbuf = cin.rdbuf();
    cin.rdbuf(ss.rdbuf());
    Mnome app{};
    EXPECT_NO_THROW(app.startPlayback());
    EXPECT_TRUE(app.isPlaying());
    this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_NO_THROW(app.stopPlayback());
    EXPECT_NO_THROW(app.startPlayback());
    this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_NO_THROW(app.setBeatPattern("!+.+"));
    this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_TRUE(app.isPlaying());
    EXPECT_NO_THROW(app.stop());
    this_thread::sleep_for(std::chrono::milliseconds(10));
    ss << "exit\n";
    ss.flush();
    cin.rdbuf(cinbuf);
}

}  // namespace mnome

#include <Repl.hpp>

#include <gtest/gtest.h>

#include <iostream>
#include <vector>

using namespace std;

namespace mnome {

TEST(ReplTest, 1)
{
    std::vector<const char*> executedCommands;

    const char* exit    = "exit";
    const char* start   = "start";
    const auto waitTime = chrono::milliseconds(5);

    ReplCommandList commands;
    commands.emplace(
        exit, [&exit, &executedCommands](const std::optional<std::string>) { executedCommands.push_back(exit); });
    commands.emplace(
        start, [&start, &executedCommands](const std::optional<std::string>) { executedCommands.push_back(start); });

    std::stringstream iStream;  // TODO does not work, fix it
    std::stringstream oStream;
    Repl dut(commands, iStream, oStream);

    dut.start();
    iStream << "exit\nstart\n";
    iStream.sync();
    this_thread::sleep_for(waitTime);
    dut.stop();
    dut.waitForStop();
    EXPECT_EQ(executedCommands[0], exit);
    EXPECT_EQ(executedCommands[1], start);
}

}  // namespace mnome

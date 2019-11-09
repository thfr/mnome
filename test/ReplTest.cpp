
#include <Repl.hpp>

#include <gtest/gtest.h>

#include <iostream>
#include <vector>

namespace mnome {

TEST(ReplTest, 1)
{
    std::vector<const char*> executedCommands;

    const char* exit  = "exit";
    const char* start = "start";

    commandlist_t commands;
    commands.emplace(
        exit, [&exit, &executedCommands](std::string&) { executedCommands.push_back(exit); });
    commands.emplace(
        start, [&start, &executedCommands](std::string&) { executedCommands.push_back(start); });

    std::stringstream is;  // TODO does not work, fix it
    std::stringstream os;
    Repl dut(commands, is, os);

    dut.start();
    is << "exit\nstart\n";
    is.sync();
    usleep(5'000);
    dut.stop();
    dut.waitForStop();
    EXPECT_EQ(executedCommands[0], exit);
    EXPECT_EQ(executedCommands[1], start);
}

}  // namespace mnome

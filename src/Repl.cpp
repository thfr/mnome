/// Repl: Read evaluate print loop
///
/// Recognizes given commands and executes the according functionality

#include "Repl.hpp"

#include <cctype>
#include <chrono>
#include <doctest.h>
#include <sstream>
#include <string_view>

using namespace std::chrono_literals;

namespace mnome {

/// Name of ENTER key within the list of commands
constexpr std::string_view ENTER_KEY_NAME = "<ENTER KEY>";

/// Trim first spaces of a string_view
static inline auto ltrim(std::string_view& input) -> std::string_view&
{
    while (!input.empty() && 0 != std::isspace(input.front())) {
        input.remove_prefix(1);
    }
    return input;
}

/// Trim trailing spaces of a string_view
static inline auto rtrim(std::string_view& input) -> std::string_view&
{
    while (!input.empty() && 0 != std::isspace(input.back())) {
        input.remove_suffix(1);
    }
    return input;
}

Repl::Repl(ReplCommandList& cmds, std::istream& iStream, std::ostream& oStream)
    : commands{cmds}, inputStream{iStream}, outputStream{oStream}, myThread{nullptr}, requestStop{false}
{
}

Repl::~Repl()
{
    stop();
    waitForStop();
}

auto Repl::setCommands(const ReplCommandList& cmds) -> bool
{
    if (!isRunning()) {
        commands = cmds;
        return true;
    }
    return false;
}

void Repl::start()
{
    waitForStop();
    myThread = std::make_unique<std::thread>([this]() -> void { this->run(); });
}

void Repl::stop()
{
    if (isRunning()) {
        requestStop = true;
    }
}

auto Repl::isRunning() const -> bool
{
    return myThread && myThread->joinable();
}

void Repl::waitForStop()
{
    if (isRunning()) {
        myThread->join();
    }
}

void Repl::run()
{
    while (!requestStop) {
        // prompt
        outputStream << "\n";
        outputStream << "[mnome]: ";

        std::string input;
        getline(inputStream, input);

        // catch ctrl+d
        if (inputStream.eof()) {
            break;
        }

        std::string_view inputSV{input};

        inputSV = rtrim(ltrim(inputSV));

        size_t cmdSep = inputSV.find(' ', 0);

        // find command and arguments in input
        const auto commandString = inputSV.substr(0, cmdSep);
        auto       args =
            (cmdSep != std::string_view::npos) ? inputSV.substr(cmdSep + 1, std::string::npos) : std::string_view{};
        auto possibleCommand = commands.find(commandString);

        // handle command not found
        if (std::end(commands) == possibleCommand) {
            // give standard implementations for help, exit and quit commands
            if (commandString == "help") {
                printHelp(args);
            }
            else if (commandString == "exit" || commandString == "quit") {
                stop();
            }
            else {
                outputStream << std::format("\"{}\" is not a valid command\n", commandString);
                printHelp();
            }
            continue;
        }

        // execute command with parameters
        try {
            possibleCommand->second.function(args);
        }
        catch (const std::exception& e) {
            outputStream << std::format("Could not get that, please try again\n");
            outputStream << e.what();
        }
    }
    requestStop = false;
}

void Repl::printHelp(std::string_view arg)
{
    if (!arg.empty()) {
        // try to display help message for the command specified in arg
        auto possibleCommand = commands.find(arg);
        if (end(commands) == possibleCommand) {
            outputStream << std::format("\"{}\" is not a valid command to show help for", arg);
        }
        else {
            outputStream << possibleCommand->second.help;
        }
    }
    else {
        // display known commands
        if (!commands.empty()) {
            outputStream << "Known commands: ";
            auto commandIter = begin(commands);
            outputStream << std::format("{}", commandIter->second.name);
            while (++commandIter != end(commands)) {
                outputStream << std::format(", {}", commandIter->second.name);
            }
        }
        else {
            outputStream << "There are no commands defined, this REPL does nothing";
        }
    }
    outputStream << '\n';
}

TEST_CASE("ReplTest")
{
    std::vector<std::string> executedCommands;

    const char* exit      = "exit";
    const char* start     = "start";
    const auto  sleepTime = 5ms;
    const auto  timeOut   = 5s;
    const auto  now       = std::chrono::steady_clock::now;

    ReplCommandList commands;
    commands.emplace(exit,
                     [&exit, &executedCommands](std::string_view) -> auto { executedCommands.emplace_back(exit); });
    commands.emplace(start,
                     [&start, &executedCommands](std::string_view) -> auto { executedCommands.emplace_back(start); });

    {
        std::stringstream iStream;
        std::stringstream oStream;
        Repl              dut(commands, iStream, oStream);

        dut.start();
        iStream << "\t exit   \t \n  \t start \t \n";
        iStream.sync();
        std::this_thread::sleep_for(sleepTime);
        CHECK(dut.isRunning());
        const auto startTime = now();
        while (timeOut > now() - startTime) {
            if (executedCommands.size() == commands.size()) {
                break;
            }
        }
        dut.stop();
        dut.waitForStop();
        REQUIRE_EQ(executedCommands.size(), commands.size());
    }

    CHECK_EQ(executedCommands[0], exit);
    CHECK_EQ(executedCommands[1], start);
}

TEST_CASE("String trimming")
{
    std::string_view testString = "\t \n bla \t \n";
    CHECK_EQ(ltrim(rtrim(testString)), "bla");
}


}  // namespace mnome

/// Repl: Read evaluate print loop
///
/// Recognizes given commands and executes the according functionality

#include "Repl.hpp"

#include <cctype>
#include <doctest.h>
#include <sstream>
#include <string_view>

namespace mnome {

/// Name of ENTER key within the list of commands
std::string_view ENTER_KEY_NAME = "<ENTER KEY>";

/// Trim first spaces of a string_view
static inline std::string_view& ltrim(std::string_view& input)
{
    while (!input.empty() && 0 != std::isspace(input.front())) {
        input.remove_prefix(1);
    }
    return input;
}

/// Trim trailing spaces of a string_view
static inline std::string_view& rtrim(std::string_view& input)
{
    while (!input.empty() && 0 != std::isspace(input.back())) {
        input.remove_suffix(1);
    }
    return input;
}


Repl::Repl() : inputStream{std::cin}, outputStream{std::cout}, myThread{nullptr}, requestStop{false}
{
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

void Repl::setCommands(const ReplCommandList& cmds)
{
    commands = cmds;
}

void Repl::start()
{
    waitForStop();
    myThread = std::make_unique<std::thread>([this]() { this->run(); });
}

void Repl::stop()
{
    if (isRunning()) {
        requestStop = true;
    }
}

bool Repl::isRunning() const
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
        outputStream << std::endl;
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
        const auto args          = (cmdSep != std::string_view::npos)
                                       ? std::optional{input.substr(cmdSep + 1, std::string::npos)}
                                       : std::nullopt;
        auto possibleCommand     = commands.find(commandString);

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
                outputStream << "\"" << commandString << "\" is not a valid command" << std::endl;
                printHelp(std::nullopt);
            }
            continue;
        }

        // execute command with parameters
        try {
            possibleCommand->second(args);
        }
        catch (const std::exception& e) {
            outputStream << "Could not get that, please try again" << std::endl;
            outputStream << e.what();
        }
    }
    requestStop = false;
}

void Repl::printHelp(std::optional<std::string> arg)
{
    auto displayCommandName = [](const std::string_view& cmd) -> const std::string_view& {
        if (cmd.empty()) {
            return ENTER_KEY_NAME;
        }
        return cmd;
    };
    if (arg) {
        // try to display help message for the command specified in arg
        const auto argString = arg.value();
        auto possibleCommand = commands.find(argString);
        if (end(commands) == possibleCommand) {
            outputStream << "\"" << argString << "\" is not a valid command to show help for" << std::endl;
        }
        else {
            outputStream << "\"" << displayCommandName(possibleCommand->first)
                         << "\" is valid command, displaying help message is not yet supported";
        }
        outputStream << std::endl;
    }
    else {
        // display known commands
        if (!commands.empty()) {
            outputStream << "Known commands: ";
            auto commandIter = begin(commands);
            outputStream << "\"" << displayCommandName(commandIter->first) << "\"";
            while (++commandIter != end(commands)) {
                outputStream << ", \"" << displayCommandName(commandIter->first) << "\"";
            }
            outputStream << std::endl;
        }
        else {
            outputStream << "There are no commands defined, this REPL does nothing" << std::endl;
        }
    }
}

TEST_CASE("ReplTest")
{
    std::vector<const char*> executedCommands;

    const char* exit    = "exit";
    const char* start   = "start";
    const auto waitTime = std::chrono::milliseconds(5);

    ReplCommandList commands;
    commands.emplace(
        exit, [&exit, &executedCommands](const std::optional<std::string>) { executedCommands.push_back(exit); });
    commands.emplace(
        start, [&start, &executedCommands](const std::optional<std::string>) { executedCommands.push_back(start); });

    std::stringstream iStream;
    std::stringstream oStream;
    Repl dut(commands, iStream, oStream);

    dut.start();
    iStream << "\t exit   \t \n  \t start \t \n";
    iStream.sync();
    std::this_thread::sleep_for(waitTime);
    CHECK(dut.isRunning());
    dut.stop();
    dut.waitForStop();
    CHECK_EQ(executedCommands[0], exit);
    CHECK_EQ(executedCommands[1], start);
}

TEST_CASE("String trimming")
{
    std::string_view testString = "\t \n bla \t \n";
    CHECK_EQ(ltrim(rtrim(testString)), "bla");
}


}  // namespace mnome

/// Repl: Read evaluate print loop
///
/// Recognizes given commands and executes the according functionality

#include "Repl.hpp"

using namespace std;

namespace mnome {

/// Name of ENTER key within the list of commands
std::string ENTER_KEY_NAME = "<ENTER KEY>";

/// Trim first spaces of a string
/// \note see https://stackoverflow.com/a/217605
static inline std::string& ltrim(std::string& input)
{
    input.erase(input.begin(),
                find_if(input.begin(), input.end(), [](int character) { return 0 == isspace(character); }));
    return input;
}

/// Trim trailing spaces of a string
/// \note see https://stackoverflow.com/a/217605
static inline void rtrim(std::string& input)
{
    input.erase(find_if(input.rbegin(), input.rend(), [](int character) { return 0 == isspace(character); }).base(),
                input.end());
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
    myThread = make_unique<thread>([this]() { this->run(); });
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
        outputStream << endl;
        outputStream << "[mnome]: ";

        string input;
        getline(inputStream, input);

        // catch ctrl+d
        if (inputStream.eof()) {
            break;
        }

        rtrim(ltrim(input));

        size_t cmdSep = input.find(' ', 0);

        // find command and arguments in input
        const string commandString = input.substr(0, cmdSep);
        const auto args = (cmdSep != string::npos) ? optional<string>{input.substr(cmdSep + 1, string::npos)} : nullopt;
        auto possibleCommand = commands.find(commandString);
        if (end(commands) == possibleCommand) {
            // give standard implementations for help, exit and quit commands
            if (commandString == "help") {
                printHelp(args);
            }
            else if (commandString == "exit" || commandString == "quit") {
                stop();
            }
            else {
                outputStream << "\"" << commandString << "\" is not a valid command" << endl;
                printHelp(nullopt);
            }
            continue;
        }

        try {
            // execute command with parameters
            possibleCommand->second(args);
        }
        catch (const exception& e) {
            outputStream << "Could not get that, please try again" << endl;
            outputStream << e.what();
        }
    }
    requestStop = false;
}

void Repl::printHelp(std::optional<std::string> arg)
{
    auto displayCommandName = [](const string& cmd) -> const string& {
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
            outputStream << "\"" << argString << "\" is not a valid command to show help for" << endl;
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
            outputStream << endl;
        }
        else {
            outputStream << "There are no commands defined, this REPL does nothing" << endl;
        }
    }
}

}  // namespace mnome

/// Repl: Read evaluate print loop
///
/// Recognizes given commands and executes the according functionality

#include "Repl.hpp"

using namespace std;

namespace mnome {

/// Trim first spaces of a string
/// \note see https://stackoverflow.com/a/217605
static inline std::string& ltrim(std::string& s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int c) { return 0 == std::isspace(c); }));
    return s;
}

/// Trim trailing spaces of a string
/// \note see https://stackoverflow.com/a/217605
static inline void rtrim(std::string& s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) { return 0 == std::isspace(ch); }).base(), s.end());
}


Repl::Repl() : inputStream{std::cin}, outputStream{std::cout}, myThread{nullptr}, requestStop{false}
{
}

Repl::Repl(ReplCommandList& cmds, std::istream& is, std::ostream& os)
    : commands{cmds}, inputStream{is}, outputStream{os}, myThread{nullptr}, requestStop{false}
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
    string input;
    while (!requestStop) {
        // prompt
        outputStream << endl;
        outputStream << "[mnome]: ";

        getline(inputStream, input);

        // catch ctrl+d
        if (inputStream.eof()) {
            break;
        }

        rtrim(ltrim(input));

        size_t cmdSep = input.find(' ', 0);

        // find command in command list
        string commandString = input.substr(0, cmdSep);
        auto possibleCommand = commands.find(commandString);
        if (end(commands) == possibleCommand) {
            outputStream << "\"" << commandString << "\" is not a valid command" << endl;
            continue;
        }

        try {
            // execute command with parameters
            string args = (cmdSep != string::npos) ? input.substr(cmdSep + 1, string::npos) : string{};
            possibleCommand->second(args);
        }
        catch (const std::exception& e) {  // reference to the base of a polymorphic object
            outputStream << "Could not get that, please try again" << endl;
            outputStream << e.what();  // information from length_error printed
        }
    }
    requestStop = false;
}

}  // namespace mnome

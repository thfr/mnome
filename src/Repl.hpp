/// Repl: Read evaluate print loop
///
/// Recognizes given commands and executes the according functionality

#ifndef MNOME_REPL_H
#define MNOME_REPL_H

#include <atomic>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>


namespace mnome {

using CommandFunction = std::function<void(const std::optional<std::string>)>;
using ReplCommandList = std::unordered_map<std::string, CommandFunction>;

/// Read evaluate print loop
class Repl
{
    ReplCommandList commands;
    std::istream& inputStream;
    std::ostream& outputStream;
    std::unique_ptr<std::thread> myThread;
    std::atomic_bool requestStop;

public:
    /// Ctor with empty command list
    Repl();

    /// Ctor with command list
    Repl(ReplCommandList& cmds, std::istream& is = std::cin, std::ostream& os = std::cout);

    ~Repl();

    /// Set the commands that the REPL should recognize
    /// \note Do not forget the exit/quit command
    void setCommands(const ReplCommandList& cmds);

    /// Start the read evaluate print loop
    void start();

    /// Stops the read evaluate print loop
    /// \note Does not block
    void stop();

    /// Indicates whether the thread is running
    bool isRunning() const;

    /// Make sure the thread has stopped
    /// \note blocks until thread is finished
    void waitForStop();

private:
    /// The method that the thread runs
    void run();

    /// Print all avaiable commands or the specific command help
    /// \par arg Command for which to display help message
    void printHelp(std::optional<std::string> arg);
};


}  // namespace mnome


#endif  //  MNOME_REPL_H

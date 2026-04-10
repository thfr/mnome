/// Repl: Read evaluate print loop
///
/// Recognizes given commands and executes the according functionality

#ifndef MNOME_REPL_H
#define MNOME_REPL_H

#include <atomic>
#include <functional>
#include <iostream>
#include <memory>
#include <string_view>
#include <thread>
#include <unordered_map>


namespace mnome {

using CommandFunction = std::function<void(std::string_view)>;
using ReplCommandList = std::unordered_map<std::string_view, CommandFunction>;

/// Read evaluate print loop
class Repl
{
    ReplCommandList              commands;
    std::istream&                inputStream{std::cin};
    std::ostream&                outputStream{std::cout};
    std::unique_ptr<std::thread> myThread;
    std::atomic_bool             requestStop{false};

public:
    /// Ctor with empty command list
    Repl() = default;

    /// Ctor with command list
    Repl(ReplCommandList& cmds, std::istream& iStream = std::cin, std::ostream& oStream = std::cout);

    ~Repl();

    // delete other contstructors
    Repl(const Repl&)                          = delete;
    Repl(Repl&&)                               = delete;
    auto operator=(Repl&&) -> Repl&&           = delete;
    auto operator=(const Repl&) -> const Repl& = delete;

    /// Set the commands that the REPL should recognize
    /// \note Only possible when Repl is not running
    /// \param  cmds  List of commands
    /// \return  True when successful
    auto setCommands(const ReplCommandList& cmds) -> bool;

    /// Start the read evaluate print loop
    void start();

    /// Stops the read evaluate print loop
    /// \note Does not block
    void stop();

    /// Indicates whether the thread is running
    auto isRunning() const -> bool;

    /// Make sure the thread has stopped
    /// \note blocks until thread is finished
    void waitForStop();

private:
    /// The method that the thread runs
    void run();

    /// Print all avaiable commands or the specific command help
    /// \par arg Command for which to display help message
    void printHelp(std::string_view arg = {});
};


}  // namespace mnome


#endif  //  MNOME_REPL_H

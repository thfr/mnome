/// Repl: Read evaluate print loop
///
/// Recognizes given commands and executes the according functionality

#ifndef MNOME_REPL_H
#define MNOME_REPL_H

#include <atomic>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>


using namespace std;


namespace mnome {

using commandlist_t = unordered_map<string, function<void(string&)>>;

/// Read evaluate print loop
class Repl
{
    commandlist_t commands;
    std::istream& inputStream;
    std::ostream& outputStream;
    std::unique_ptr<thread> myThread;
    atomic_bool requestStop;

public:
    /// Ctor with empty command list
    Repl();

    /// Ctor with command list
    Repl(commandlist_t& cmds, std::istream& is = std::cin, std::ostream& os = std::cout);

    ~Repl();

    /// Set the commands that the REPL should recognize
    /// \note Do not forget the exit/quit command
    void setCommands(const commandlist_t& cmds);

    /// Start the read evaluate print loop
    void start();

    /// Stops the read evaluate print loop
    /// \note Does not block
    void stop();

    /// Indicates whether the thread is running
    bool isRunning() const;

    /// Make sure the thread has stopped
    /// \note blocks until thread is finished
    void waitForStop() const;

private:
    /// The method that the thread runs
    void run();
};


}  // namespace mnome


#endif  //  MNOME_REPL_H

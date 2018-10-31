/// Repl: Read evaluate print loop
///
/// Recognizes given commands and executes the according functionality

#include <string>
#include <iostream>
#include <memory>
#include <atomic>
#include <thread>
#include <functional>
#include <unordered_map>


using namespace std;


namespace mnome {

using commandlist_t = unordered_map<string, function<void(string&)>>;

/// Read evaluate print loop
class Repl
{
   commandlist_t commands;
   std::unique_ptr<thread> myThread;
   atomic_bool requestStop;

public:
   /// Ctor with empty command list
   Repl();

   /// Ctor with command list
   Repl(commandlist_t& cmds);

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

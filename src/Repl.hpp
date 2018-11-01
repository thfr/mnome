/// Repl: Read evaluate print loop
///
/// Recognizes given commands and executes the according functionality

#include <atomic>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <sstream>

#include <stdexcept>
#include <utility>
#include <iterator>
#include <type_traits>

using namespace std;


namespace mnome {

using commandlist_t = unordered_map<string, function<void(string&)>>;

using paramlist_t = std::vector<std::string>;

template<typename F>
struct signature : signature<decltype(&F::operator())>{
};

template<typename F>
struct signature<F*> : signature<F>{
};

template<typename C, typename R, typename... P>
struct signature<R(C::*)(P...)>: signature<R(P...)>{
};

template<typename C, typename R, typename... P>
struct signature<R(C::*)(P...) const>: signature<R(P...)>{
};

template<typename R, typename... P>
struct signature<R(P...)>{
    using result_t=R;
    using param_t=std::tuple<P...>;
    using param_values_t=std::tuple<std::decay_t<P>...>;
};


template<typename T>
struct ArgumentParser;


template<>
struct ArgumentParser<std::string>{
    static auto parse(paramlist_t params){
        if(params.size() != 1){
            throw std::runtime_error("to much arguments");
        }

        return params[0];
    }

    static constexpr int number=1;

    static constexpr bool multiple=false;

};

template<typename... Ts>
struct ArgumentParser<std::tuple<Ts...>>{
    static auto parse(paramlist_t params){
        if(params.size() != sizeof...(Ts)){
            throw std::runtime_error("to much arguments");
        }

        return [&params] <std::size_t... I>(std::index_sequence<I...>){
            return std::make_tuple(ArgumentParser<std::tuple_element_t<I,std::tuple<Ts...>>>::parse({params[I]})...);
        }(std::index_sequence_for<Ts...>());
    }

    static constexpr int number=(ArgumentParser<Ts>::number+...);

    static constexpr bool multiple=(ArgumentParser<Ts>::multiple||...);

};

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
   explicit Repl(commandlist_t& cmds);

   ~Repl();

   template<typename F>
   void addCommand(std::string name, F function){
        commands[name]=[function](const string& params){
            std::istringstream iss(params);
            paramlist_t paramList{std::istream_iterator<std::string>(iss),
                                             std::istream_iterator<std::string>()};

            auto parsedParams = ArgumentParser<typename signature<F>::param_t>::parse(paramList);

            std::apply(function,parsedParams);
        };
   }

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

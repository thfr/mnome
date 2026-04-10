/// Mnome - A metronome program
#include "Mnome.hpp"

#include <csignal>


using namespace std;

namespace {
auto getApp() -> mnome::Mnome&
{
    static auto app = mnome::Mnome();
    return app;
}
}  // namespace


void shutDownAppHandler(int signalCode)
{
    (void)signalCode;
    getApp().stop();
}

auto main() -> int
{

    signal(SIGINT, shutDownAppHandler);
    signal(SIGTERM, shutDownAppHandler);
    signal(SIGABRT, shutDownAppHandler);

    auto& app = getApp();

    app.waitForStop();

    return 0;
}

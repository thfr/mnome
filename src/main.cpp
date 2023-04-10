/// Mnome - A metronome program
#include "Mnome.hpp"

#include <cmath>
#include <csignal>
#include <memory>
#include <mutex>


using namespace std;

namespace {
mutex AppMutex;
unique_ptr<mnome::Mnome> App;
}  // namespace


void shutDownAppHandler(int signalCode)
{
    (void)signalCode;
    lock_guard<mutex> lockGuard{AppMutex};
    if (App) {
        App->stop();
    }
}

int main()
{
    signal(SIGINT, shutDownAppHandler);
    signal(SIGTERM, shutDownAppHandler);
    signal(SIGABRT, shutDownAppHandler);

    {
        lock_guard<mutex> lockGuard{AppMutex};
        App = make_unique<mnome::Mnome>();
    }

    App->waitForStop();

    return 0;
}

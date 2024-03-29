/// Mnome main application
#ifndef MNOME_H
#define MNOME_H

#include "BeatPlayer.hpp"
#include "Repl.hpp"

#include <mutex>
#include <string>

namespace mnome {

extern const size_t PLAYBACK_RATE;

/// Mnome main application class
class Mnome
{
    BeatPlayer bp;
    Repl repl;
    std::mutex cmdMtx;

public:
    /// Ctor
    Mnome();

    void stop();

    void stopPlayback();
    void startPlayback();
    void togglePlayback();
    void setBPM(std::optional<std::string> args);
    void setBeatPattern(std::optional<std::string> args);
    bool isPlaying() const;

    /// Wait for the read evaluate loop to finish
    void waitForStop();
};

}  // namespace mnome

#endif  // MNOME_H

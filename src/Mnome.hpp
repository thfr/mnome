#ifndef MNOME_H
#define MNOME_H

#include "BeatPlayer.hpp"
#include "Repl.hpp"

#include <mutex>
#include <string>

namespace mnome {

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
    void setBPM(const std::string& bpmStr);
    void setBeatPattern(const std::string& patternStr);
    bool isPlaying() const;

    /// Wait for the read evaluate loop to finish
    void waitForStop();
};

}  // namespace mnome

#endif  // MNOME_H

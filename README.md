# Mnome

Mnome is a metronom application written in C++.

## Features:
* Arbitrary beat pattern that include **accent**, **normal beat** and **pause**
* Beat sound generated at runtime
* BPM change during playback
* A nice Read Evaluate Print Loop (REPL)


## Note
Right now this project is a training place and play ground for me making myself familiar with certain features of the programming language and the ecosystem around it.


# Usage

Following commands are implemented: `start`, `stop`, `bpm <number>`, `pattern <list of "!", "+" or ".">, `exit` and `quit`

```
[mnome]: <enter>
Playing !+++ at 80 bpm

[mnome]: <enter>
Stopping playback

[mnome]: start
Playing !+++ at 80 bpm

[mnome]: stop
Stopping playback

[mnome]: bpm 120

[mnome]: start
Playing !+++ at 120 bpm

[mnome]: bpm 160
Playing !+++ at 160 bpm

[mnome]: pattern asdf
Command usage: pattern <pattern>
  <pattern> must be in the form of `[!|+|.]*`
  `!` = accentuated beat  `+` = normal beat  `.` = pause

[mnome]: pattern !+.+
Playing !+.+ at 160 bpm

[mnome]: stop
Stopping playback

[mnome]: exit
```

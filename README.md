# Mnome

Mnome is a metronom application written in C++.
Right now it is more or less a training place and play ground for me in order to get familiar with certain features of the programming language and the ecosystem around it.

# Usage

Following commands are implemented: `start`, `stop`, `bpm <number>` and `pattern <list of "*" and "+">`

```
[mnome]: start
Playing at 80 bpm

[mnome]: stop

[mnome]: bpm 120

[mnome]: start
Playing at 120 bpm

[mnome]: bpm 160
Playing at 160 bpm

[mnome]: pattern *+++*++
Playing at 160 bpm

[mnome]: stop

[mnome]: exit
```

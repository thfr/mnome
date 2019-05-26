project('mnome', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'werror=true', 'cpp_std=c++17'])

asound_dep = dependency('alsa')
threads_dep = dependency('threads')

executable('mnome',
           'src/BeatPlayer.cpp',
           'src/BeatPlayer.hpp',
           'src/main.cpp',
           'src/Repl.cpp',
           'src/Repl.hpp',
           dependencies: [threads_dep, asound_dep],
           install : true)

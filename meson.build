project('mnome', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'werror=true', 'cpp_std=c++20'])

miniaudio = subproject('miniaudio')
miniaudio_dep = miniaudio.get_variable('miniaudio_dep')
doctest = subproject('doctest')
doctest_dep = doctest.get_variable('doctest_dep')
threads_dep = dependency('threads')

mnome_lib = executable('mnome',
  'src/main.cpp',
  'src/AudioSignal.cpp',
  'src/AudioSignal.hpp',
  'src/BeatPlayer.cpp',
  'src/BeatPlayer.hpp',
  'src/Repl.cpp',
  'src/Repl.hpp',
  'src/Mnome.cpp',
  'src/Mnome.hpp',
  dependencies: [miniaudio_dep, doctest_dep, threads_dep],
  implicit_include_directories: true,
  cpp_args: '-DDOCTEST_CONFIG_DISABLE=1',
  )


mnome_test = executable(
  'mnometest',
  'src/doctestmain.cpp',
  'src/AudioSignal.cpp',
  'src/AudioSignal.hpp',
  'src/BeatPlayer.cpp',
  'src/BeatPlayer.hpp',
  'src/Repl.cpp',
  'src/Repl.hpp',
  'src/Mnome.cpp',
  'src/Mnome.hpp',
  dependencies : [doctest_dep, miniaudio_dep, threads_dep],
  implicit_include_directories: true,
  )

test('MnomeTest', mnome_test)

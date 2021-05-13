project('mnome', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'werror=true', 'cpp_std=c++17'])

miniaudio = subproject('miniaudio')
miniaudio_dep = miniaudio.get_variable('miniaudio_dep')

mnome_lib = static_library('libmnome',
  'src/AudioSignal.cpp',
  'src/AudioSignal.hpp',
  'src/BeatPlayer.cpp',
  'src/BeatPlayer.hpp',
  'src/Repl.cpp',
  'src/Repl.hpp',
  'src/Mnome.cpp',
  'src/Mnome.hpp',
  dependencies: [miniaudio_dep],
  implicit_include_directories: true,
  )

threads_dep = dependency('threads')

mnome_exe = executable('mnome',
  'src/main.cpp',
  dependencies: [miniaudio_dep, threads_dep],
  link_with: mnome_lib,
  implicit_include_directories: true,
  )

gtest = subproject('gtest')
gtest_dep = gtest.get_variable('gtest_main_dep')

repl_test = executable(
  'ReplTest',
  files(['./test/ReplTest.cpp']),
  dependencies : [gtest_dep],
  link_with: mnome_lib,
  include_directories: './src/',
  )

test('ReplTest', repl_test)

mnome_test = executable(
  'MnomeTest',
  files(['./test/MnomeTest.cpp']),
  dependencies : [gtest_dep, miniaudio_dep],
  link_with: mnome_lib,
  include_directories: './src/',
  )

test('MnomeTest', mnome_test)

audiosignal_test = executable(
  'AudioSignalTest',
  files(['./test/AudioSignalTest.cpp']),
  dependencies : [gtest_dep],
  link_with: mnome_lib,
  include_directories: './src/',
  )

test('AudioSignalTest', audiosignal_test)

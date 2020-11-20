project('mnome', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'werror=false', 'cpp_std=c++17'])

miniaudio = subproject('miniaudio')
miniaudio_dep = miniaudio.get_variable('miniaudio_dep')

mnome_lib = static_library('libmnome',
  'src/BeatPlayer.cpp',
  'src/BeatPlayer.hpp',
  'src/Repl.cpp',
  'src/Repl.hpp',
  dependencies: [miniaudio_dep],
  implicit_include_directories: true,
  implib: true,
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

testapp = executable(
  'ReplTest',
  files(['./test/ReplTest.cpp']),
  dependencies : [gtest_dep],
  link_with: mnome_lib,
  include_directories: './src/',
  )

test('ReplTest', testapp)


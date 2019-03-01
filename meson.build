project('mnome', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'werror=true', 'cpp_std=c++17'])

asound_dep = dependency('alsa')
threads_dep = dependency('threads')
mnome_lib = static_library('mnome',
           'src/BeatPlayer.cpp',
           'src/BeatPlayer.hpp',
           'src/Repl.cpp',
           'src/Repl.hpp',
           )

mnome_exe = executable('mnome',
           'src/main.cpp',
           dependencies: [threads_dep, asound_dep],
           link_with: mnome_lib,
           install: true,
           implib: true,
           )


gtest_dep = dependency('gtest_main')

testapp = executable(
  'ReplTest',
  files(['./test/ReplTest.cpp']),
  dependencies : gtest_dep,
  link_with: mnome_lib,
  include_directories: './src/',
  install : false)

test('ReplTest', testapp)


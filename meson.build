project('mnome', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'werror=false', 'cpp_std=c++17'])

cc = meson.get_compiler('cpp')

asound_dep = dependency('alsa')
threads_dep = dependency('threads')
math_dep = cc.find_library('m', required : false)
dynamic_lib_dep = cc.find_library('dl', required: false)
mnome_lib = static_library('mnome',
  'src/BeatPlayer.cpp',
  'src/BeatPlayer.hpp',
  'src/Repl.cpp',
  'src/Repl.hpp',
  'src/miniaudio.h',
  )

mnome_exe = executable('mnome',
  'src/main.cpp',
  dependencies: [threads_dep, asound_dep, math_dep, dynamic_lib_dep],
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


project('miniaudio', 'c',
  version : '0.11.14',
  )

cc = meson.get_compiler('c')
math_dep = cc.find_library('m', required : false)
dl_dep = cc.find_library('dl', required : false)

threads_dep = dependency('threads')

include_dirs = include_directories('./extras/miniaudio_split/')

miniaudio_lib = library('miniaudio',
  './extras/miniaudio_split/miniaudio.c',
  './extras/miniaudio_split/miniaudio.h',
  dependencies: [threads_dep, math_dep, dl_dep],
  include_directories: include_dirs,
  )

miniaudio_dep = declare_dependency(
  include_directories: include_dirs,
  link_with: miniaudio_lib
  )

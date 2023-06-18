project('cli', 'c',
  version : '2.0.2',
  )

threads_dep = dependency('threads')
include_dirs = include_directories('./include/')

cli_lib = library('cli',
  dependencies: [threads_dep],
  include_directories: include_dirs,
  )

cli_dep = declare_dependency(
  include_directories: include_dirs,
  link_with: cli_lib
  )

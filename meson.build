project(
  'sfizz-tui',
  'cpp',
  version : '0.1.0',
  default_options : [
    'cpp_std=c++20',
    'warning_level=2',
    'buildtype=debugoptimized',
  ]
)

# dependências
sfizz_dep = dependency('sfizz')
ftxui_dep = dependency('ftxui')

core_proj = subproject('core')
core_dep = core_proj.get_variable('core_dep')

srcs = files(
  'src/main.cpp',
  'src/tui.cpp',
  'src/sfizz_engine.cpp',
)

executable(
  'sfizz-tui',
  srcs,
  include_directories : include_directories('src'),
  dependencies : [
    core_dep,
    sfizz_dep,
    ftxui_dep,
  ],
  install: true
)
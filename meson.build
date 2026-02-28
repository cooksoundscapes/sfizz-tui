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
jack_dep = dependency('jack')
sfizz_dep = dependency('sfizz')
ftxui_dep = dependency('ftxui')

srcs = files(
  'src/main.cpp',
  'src/jack.cpp',
  'src/tui.cpp',
  'src/sfizz_engine.cpp',
)

executable(
  'sfizz-tui',
  srcs,
  include_directories : include_directories('src'),
  dependencies : [
    jack_dep,
    sfizz_dep,
    ftxui_dep,
  ],
  install: true
)
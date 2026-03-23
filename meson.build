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
  'src/tui_base.cpp',
  'src/tui_core.cpp',
  'src/tui_view.cpp', 
  'src/sfizz_engine.cpp',
  'src/sfizz_client.cpp',
  'src/midi_parser.cpp'
)

rt_isolation_args = []
if host_machine.cpu_family() in ['arm', 'aarch64']
  rt_isolation_args = ['-DENABLE_RT_CPU_ISOLATION']
endif

executable(
  'sfizz-tui',
  srcs,
  include_directories : include_directories('src'),
  dependencies : [
    core_dep,
    sfizz_dep,
    ftxui_dep,
  ],
  cpp_args: rt_isolation_args,
  install: true
)
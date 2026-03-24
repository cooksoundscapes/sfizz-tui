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

if host_machine.cpu_family() in ['arm', 'aarch64']

  sysroot = meson.get_external_property('sys_root', '')
  message('ARM detected - activating CPU isolation and looking for deps at ' + sysroot + '/usr/local/lib')

  add_global_arguments('-DENABLE_CPU_ISOLATION', language: 'cpp')

  # add_global_arguments('-DSTUB_MODE', language: 'cpp')
  # add_global_arguments('-fsanitize=address', '-fno-omit-frame-pointer', language: 'cpp')
  # add_global_link_arguments('-fsanitize=address', language: 'cpp')

  cpp = meson.get_compiler('cpp')

  ftxui_screen = cpp.find_library('ftxui-screen', dirs: [sysroot + '/usr/local/lib'])
  ftxui_dom    = cpp.find_library('ftxui-dom',    dirs: [sysroot + '/usr/local/lib'])
  ftxui_component = cpp.find_library('ftxui-component', dirs: [sysroot + '/usr/local/lib'])

  ftxui_deps = [ftxui_screen, ftxui_dom, ftxui_component]

  sfizz_dep = cpp.find_library('sfizz', dirs: [sysroot + '/usr/local/lib'])
  includes  = [
    include_directories('src'),
    include_directories(sysroot + '/usr/local/include')
  ]

else
  sfizz_dep = dependency('sfizz')
  ftxui_deps = [dependency('ftxui')]
  includes  = [
    include_directories('src')
  ]
  
endif

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


executable(
  'sfizz-tui',
  srcs,
  include_directories : includes,
  dependencies : [core_dep, sfizz_dep] + ftxui_deps,
  install: true
)

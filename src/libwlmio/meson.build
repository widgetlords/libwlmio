inc = include_directories('.')

wlmio_lib = both_libraries(
  'wlmio',
  [ 'io.c', 'sync.c', 'wlmio.c' ],
  include_directories: inc,
  dependencies: [ canard_dep, libgpiod_dep ],
  install: true
)

wlmio_dep = declare_dependency(
  include_directories: inc,
  link_with: wlmio_lib.get_static_lib()
)

install_headers('wlmio.h')

pkg.generate(wlmio_lib)

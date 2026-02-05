project('libwlmio', 'c', default_options: ['c_std=gnu11', 'optimization=2'], license: 'MIT', version: '1.2.2')
add_project_arguments('-Werror=return-type', language: ['c'])

pkg = import('pkgconfig')

libgpiod_dep = dependency('libgpiod', static: true, version: '<2', fallback: 'libgpiod')

subdir('src/libcanard')
subdir('src/libwlmio')
subdir('src/pywlmio')
subdir('src/tools')

install_data('wlmio.txt', install_dir: get_option('prefix') / 'share/libwlmio')
install_data('50-wlmio.network', install_dir: get_option('prefix') / 'lib/systemd/network')
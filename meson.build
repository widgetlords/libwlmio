project('libwlmio', 'c', default_options: ['c_std=gnu11', 'b_lto=true', 'optimization=2'], license: 'MIT', version: '1.1.0')
add_project_arguments('-Werror=return-type', language: ['c'])

pkg = import('pkgconfig')

libgpiod_dep = dependency('libgpiod', static: true)

subdir('src/libcanard')
subdir('src/libwlmio')
subdir('src/modbusd')
subdir('src/pywlmio')
subdir('src/tools')

subdir('overlay')

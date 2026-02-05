project('libgpiod', 'c', version: '1.6.5')

mod = import('unstable-external_project')

p = mod.add_project('configure',
  configure_options: [
    '--prefix=@PREFIX@',
    '--libdir=@PREFIX@/@LIBDIR@',
    '--includedir=@PREFIX@/@INCLUDEDIR@',
  ],
  verbose: true
)

libgpiod_dep = p.dependency('gpiod')
meson.override_dependency('libgpiod', libgpiod_dep)
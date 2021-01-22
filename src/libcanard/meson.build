canard_lib = static_library('canard', [ 'canard.c', 'canard_dsdl.c' ], c_args: '-DCANARD_DSDL_CONFIG_LITTLE_ENDIAN')
canard_dep = declare_dependency(include_directories: include_directories('.'), link_with: canard_lib, version: '0.100')

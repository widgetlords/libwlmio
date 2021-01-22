pymod = import('python')
python = pymod.find_installation('python3', required: false, disabler: true)

python.install_sources([ 'pywlmio.py' ], subdir: '../dist-packages')

python.extension_module(
  '_wlmio',
  ['wlmio.c'],
  dependencies: [python.dependency(), wlmio_dep], 
  subdir: '../dist-packages',
  install: true
)

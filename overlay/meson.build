dtc = find_program('dtc')

overlay = custom_target('wlmio.dtbo',
	output: 'wlmio.dtbo',
	input: 'wlmio.dts',
	command: [ dtc, '-@', '-I', 'dts', '-O', 'dtb', '-o', '@OUTPUT@', '@INPUT@' ],
	build_by_default: true,
	install: true,
	install_dir: '/boot/overlays/',
	# install_dir: '/usr/share/rpikernelhack/overlays/',
	install_mode: ['rwxr-xr-x', 'root', 'root']
	# install_mode: 'rwxr-xr-x'
)

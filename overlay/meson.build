dtc = find_program('dtc')

overlay = custom_target('wlmio.dtbo',
	output: 'wlmio.dtbo',
	input: 'wlmio.dts',
	command: [ dtc, '-@', '-I', 'dts', '-O', 'dtb', '-o', '@OUTPUT@', '@INPUT@' ],
	build_by_default: true,
	install: true,
	install_dir: '/boot/firmware/overlays/',
	# install_dir: '/usr/share/rpikernelhack/overlays/',
	install_mode: ['rwxr-xr-x', 'root', 'root']
	# install_mode: 'rwxr-xr-x'
)

overlay = custom_target('wlmio-pi5.dtbo',
	output: 'wlmio-pi5.dtbo',
	input: 'wlmio-pi5.dts',
	command: [ dtc, '-@', '-I', 'dts', '-O', 'dtb', '-o', '@OUTPUT@', '@INPUT@' ],
	build_by_default: true,
	install: true,
	install_dir: '/boot/firmware/overlays/',
	# install_dir: '/usr/share/rpikernelhack/overlays/',
	install_mode: ['rwxr-xr-x', 'root', 'root']
	# install_mode: 'rwxr-xr-x'
)


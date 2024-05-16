deb:
	meson setup build
	meson configure --buildtype release --prefix /usr build
	cd build; ninja

install:
	cd build; meson install

#VERSION = $(shell grep Version DEBIAN/control | sed 's/^Version: //')

deb:
	meson build
	meson configure --buildtype release --prefix /usr build
	cd build; ninja
	#mkdir -p build/deb/DEBIAN
	#cp -r DEBIAN build/deb/
	#rm -rf build/deb/usr/lib/python*/site-packages/
	#dpkg-deb --build build/deb libwlmio-$(VERSION)-armhf.deb

install:
	cd build; meson install

clean:
	#rm *.deb
	#rm -rf build/deb

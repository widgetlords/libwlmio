VERSION = $(shell grep Version DEBIAN/control | sed 's/^Version: //')

deb:
	meson configure --buildtype release --prefix /usr build
	cd build && DESTDIR=deb meson install
	mkdir -p build/deb/DEBIAN
	cp -r DEBIAN build/deb/
	rm -rf build/deb/usr/lib/python*/site-packages/
	dpkg-deb --build build/deb libwlmio-$(VERSION)-armhf.deb

clean:
	rm *.deb
	rm -rf build/deb

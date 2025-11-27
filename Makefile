include compile.mk

clean: 
	rm -rf .install

install: .install/cairn
	mkdir -p ${DESTDIR}/usr/local/bin
	install -m 755 .install/cairn ${DESTDIR}/usr/local/bin

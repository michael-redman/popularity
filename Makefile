.PHONY: common player slideshow

all: common player slideshow

common player slideshow:
	cd $@ && make

clean:
	cd common && make clean
	cd player && make clean
	cd slideshow && make clean

install:
	cd common && make install
	cd player && make install
	cd slideshow && make install

uninstall:
	cd common && make uninstall
	cd player && make uninstall
	cd slideshow && make uninstall
	rm -rf /usr/local/share/popularity
	rm -rf /usr/local/share/doc/popularity

#IN GOD WE TRVST.

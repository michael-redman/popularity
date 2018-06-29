all:  libpopularity player

.PHONY: libpopularity player

clean:
	cd libpopularity && if [ "`uname | grep NetBSD`" ]; then . ./NetBSD-env.sh; fi; if [ "`uname | grep CYGWIN`" ]; then . ./Cygwin-env.sh; fi && make clean
	cd player && if [ "`uname | grep NetBSD`" ]; then . ./NetBSD-env.sh; fi && make clean

libpopularity:
	cd libpopularity && if [ "`uname | grep NetBSD`" ]; then . ./NetBSD-env.sh; fi; if [ "`uname | grep CYGWIN`" ]; then . ./Cygwin-env.sh; fi && make

player: libpopularity
	cd player && if [ "`uname | grep NetBSD`" ]; then . ./NetBSD-env.sh; fi && make

install:
	cd libpopularity && if [ "`uname | grep NetBSD`" ]; then . ./NetBSD-env.sh; fi; if [ "`uname | grep CYGWIN`" ]; then . ./Cygwin-env.sh; fi && make install
	cd player && if [ "`uname | grep NetBSD`" ]; then . ./NetBSD-env.sh; fi && make install

uninstall:
	cd libpopularity && if [ "`uname | grep NetBSD`" ]; then . ./NetBSD-env.sh; fi; if [ "`uname | grep CYGWIN`" ]; then . ./Cygwin-env.sh; fi && make uninstall
	cd player && if [ "`uname | grep NetBSD`" ]; then . ./NetBSD-env.sh; fi && make uninstall

#IN GOD WE TRVST.

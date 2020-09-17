CFLAGS=-Wall -g -fstack-protector -ftrapv
MAN_PATH?=/usr/local/share/man
PQ_CFLAGS?=-I /usr/include/postgresql
PQ_LDFLAGS?=-L /usr/lib/postgredql -lpq
SHARED_LIB_EXT?=so

PROGS=entropy clear_dead_paths print_new_paths
#SCRIPTS=focuser_bstep libpq_connect_string rebalance
SQL=known_paths_count.plpgsql post_delta.plpgsql
MAN1=import.1 clear_dead_paths.1

LIBPOPULARITY=libpopularity.$(SHARED_LIB_EXT)

#.PHONY: player slideshow

.SUFFIXES: .c .pgc
.pgc.c:
	ecpg $<

all: common player slideshow

clean_common:
	rm -f $(PROGS) $(LIBPOPULARITY)
	for file in $(ECPG_PROGS); do rm -f "$$file".c; done

clean:
	cd player && if [ "`uname | grep NetBSD`" ]; then . ./NetBSD-env.sh; fi && make clean
	cd slideshow && if [ "`uname | grep NetBSD`" ]; then . ./NetBSD-env.sh; fi && make clean
	make clean_common

entropy: entropy.c popularity.h
	cc $(CFLAGS) $(PQ_CFLAGS) -o $@ $< -lm

#n-sn-ge-1: n-sn-ge-1.c
#	cc $(CFLAGS) -o $@ $< $(ECPG_CFLAGS) $(ECPG_LDFLAGS) -lm

print_new_paths: print_new_paths.c 
	cc $(CFLAGS) -o $@ $< $(PQ_CFLAGS) $(PQ_LDFLAGS)

#select-from-dist: select-from-dist.c libpopularity.so
#	cc $(CFLAGS) -o $@ $< $(ECPG_CFLAGS) $(ECPG_LDFLAGS) $(LDFLAGS) -lpopularity

clear_dead_paths: clear_dead_paths.c
	cc $(CFLAGS) -o $@ $< $(PQ_CFLAGS) $(PQ_LDFLAGS)

$(LIBPOPULARITY): popularity.c popularity.h
	cc $(CFLAGS) -c -fpic -shared -o $@ popularity.c $(PQ_CFLAGS)

common: $(PROGS) $(LIBPOPULARITY)

#common:
#	if [ "`uname | grep NetBSD`" ]; then . ./NetBSD-env.sh; fi; if [ "`uname | grep CYGWIN`" ]; then . ./Cygwin-env.sh; fi && make

player: common
	cd player && if [ "`uname | grep NetBSD`" ]; then . ./NetBSD-env.sh; fi && make

slideshow: common
	cd slideshow && if [ "`uname | grep NetBSD`" ]; then . ./NetBSD-env.sh; fi && make

install_common:
	mkdir -p /usr/local/bin
	for prog in $(PROGS) ; do install -m 0755 $$prog /usr/local/bin/popularity_$$prog; done
	mkdir -p /usr/local/lib
	install -m 0644 $(LIBPOPULARITY) /usr/local/lib/$(LIBPOPULARITY)
	mkdir -p /usr/local/include
	install -m 0644 popularity.h /usr/local/include/popularity.h
	mkdir -p /usr/local/share/popularity/common/sql
	for file in $(SQL); do install -m 0644 $$file /usr/local/share/popularity/common/sql/$$file; done
	mkdir -p /usr/local/share/popularity/common/doc
	for doc in COPYRIGHT LICENSE README; do install -m 0644 $$doc /usr/local/share/popularity/common/doc/$$doc; done
	mkdir -p $(MAN_PATH)/man1
	mkdir -p $(MAN_PATH)/man7
	for file in $(MAN1); do install -m 0644 $$file $(MAN_PATH)/man1/popularity_$$file; done
	install -m 0644 popularity.7 $(MAN_PATH)/man7/popularity.7

install: install_common
	#cd libpopularity && if [ "`uname | grep NetBSD`" ]; then . ./NetBSD-env.sh; fi; if [ "`uname | grep CYGWIN`" ]; then . ./Cygwin-env.sh; fi && make install
	cd player && if [ "`uname | grep NetBSD`" ]; then . ./NetBSD-env.sh; fi && make install
	cd slideshow && if [ "`uname | grep NetBSD`" ]; then . ./NetBSD-env.sh; fi && make install

uninstall_common:
	for prog in $(PROGS) ; do rm -f /usr/local/bin/popularity_$$prog; done
	rm -f /usr/local/lib/$(LIBPOPULARITY)
	rm -f /usr/local/include/popularity.h
	rm -rf /usr/local/share/popularity/common
	for file in $(MAN1); do rm -f $(MAN_PATH)/man1/popularity$$file; done
	rm -f $(MAN_PATH)/man7/popularity.7

uninstall:
	#cd libpopularity && if [ "`uname | grep NetBSD`" ]; then . ./NetBSD-env.sh; fi; if [ "`uname | grep CYGWIN`" ]; then . ./Cygwin-env.sh; fi && make uninstall
	cd slideshow && if [ "`uname | grep NetBSD`" ]; then . ./NetBSD-env.sh; fi && make uninstall
	cd player && if [ "`uname | grep NetBSD`" ]; then . ./NetBSD-env.sh; fi && make uninstall
	make uninstall_common

#IN GOD WE TRVST.

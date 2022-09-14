CFLAGS?=-Wall -g -fstack-protector -ftrapv
LDFLAGS?=-L .
MAN_PATH?=/usr/local/share/man
PQ_CFLAGS?=-I /usr/include/postgresql
PQ_LDFLAGS?=-L /usr/lib/postgredql -lpq
SHARED_LIB_EXT?=so

PROGS=entropy clear_dead_paths mkdist print_new_paths
SQL=count_estimate.plpgsql known_paths_count.plpgsql post_delta.plpgsql
MAN1=clear_dead_paths.1 print_new_paths.1

LIBPOPULARITY=libpopularity.$(SHARED_LIB_EXT)

#.PHONY: player slideshow

.SUFFIXES: .c .pgc
.pgc.c:
	ecpg $<

all: $(PROGS) $(LIBPOPULARITY)

clean:
	rm -f $(PROGS) $(LIBPOPULARITY) *.o

#clean:
#	cd player && if [ "`uname | grep NetBSD`" ]; then . ./NetBSD-env.sh; fi && make clean
#	cd slideshow && if [ "`uname | grep NetBSD`" ]; then . ./NetBSD-env.sh; fi && make clean
#	make clean_common

entropy: entropy.c popularity.h
	cc $(CFLAGS) $(PQ_CFLAGS) -o $@ $< -lm

mkdist: mkdist.c libpopularity.so
	cc $(CFLAGS) -o $@ $< $(PQ_CFLAGS) $(PQ_LDFLAGS) $(LDFLAGS) -lm -lpopularity

print_new_paths: print_new_paths.c 
	cc $(CFLAGS) -o $@ $< $(PQ_CFLAGS) $(PQ_LDFLAGS)

#select-from-dist: select-from-dist.c libpopularity.so
#	cc $(CFLAGS) -o $@ $< $(ECPG_CFLAGS) $(ECPG_LDFLAGS) $(LDFLAGS) -lpopularity

clear_dead_paths: clear_dead_paths.c
	cc $(CFLAGS) -o $@ $< $(PQ_CFLAGS) $(PQ_LDFLAGS)

popularity.o: popularity.c
	cc $(CFLAGS) -c -fpic popularity.c $(PQ_CFLAGS)

$(LIBPOPULARITY): popularity.o popularity.h
	cc -shared -o $@ popularity.o $(PQ_LDFLAGS)

install:
	install -d /usr/local/bin
	for prog in $(PROGS) ; do install -m 0755 $$prog /usr/local/bin/popularity_$$prog || exit; done
	install -d /usr/local/lib
	install -m 0644 $(LIBPOPULARITY) /usr/local/lib/$(LIBPOPULARITY)
	install -d /usr/local/include
	install -m 0644 popularity.h /usr/local/include/popularity.h
	install -d /usr/local/share/popularity/common/tables
	install -m 0644 tables/* /usr/local/share/popularity/common/tables
	install -d /usr/local/share/popularity/common/sql
	install -m 0644 sql/* /usr/local/share/popularity/common/sql
	#for file in $(SQL); do install -m 0644 sql/$$file /usr/local/share/popularity/common/sql/$$file; done
	install -d /usr/local/share/popularity
	for doc in ../COPYRIGHT ../LICENSE ../README; do install -m 0644 $$doc /usr/local/share/popularity || exit; done
	install -d $(MAN_PATH)/man1
	install -d $(MAN_PATH)/man7
	for file in $(MAN1); do install -m 0644 $$file $(MAN_PATH)/man1/popularity_$$file || exit; done
	install -m 0644 popularity.7 $(MAN_PATH)/man7/popularity.7

#install: install_common
	#cd libpopularity && if [ "`uname | grep NetBSD`" ]; then . ./NetBSD-env.sh; fi; if [ "`uname | grep CYGWIN`" ]; then . ./Cygwin-env.sh; fi && make install
#	cd player && if [ "`uname | grep NetBSD`" ]; then . ./NetBSD-env.sh; fi && make install
#	cd slideshow && if [ "`uname | grep NetBSD`" ]; then . ./NetBSD-env.sh; fi && make install

uninstall:
	for prog in $(PROGS) ; do rm -f /usr/local/bin/popularity_$$prog; done
	rm -f /usr/local/lib/$(LIBPOPULARITY)
	rm -f /usr/local/include/popularity.h
	rm -rf /usr/local/share/popularity/common
	for file in $(MAN1); do rm -f $(MAN_PATH)/man1/popularity$$file; done
	rm -f $(MAN_PATH)/man7/popularity.7

#uninstall:
	#cd libpopularity && if [ "`uname | grep NetBSD`" ]; then . ./NetBSD-env.sh; fi; if [ "`uname | grep CYGWIN`" ]; then . ./Cygwin-env.sh; fi && make uninstall
#	cd slideshow && if [ "`uname | grep NetBSD`" ]; then . ./NetBSD-env.sh; fi && make uninstall
#	cd player && if [ "`uname | grep NetBSD`" ]; then . ./NetBSD-env.sh; fi && make uninstall
#	make uninstall_common

#IN GOD WE TRVST.

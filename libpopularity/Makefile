CFLAGS=-Wall -g -fstack-protector -ftrapv
MAN_PATH?=/usr/local/share/man
PQ_CFLAGS?=-I /usr/include/postgresql
PQ_LDFLAGS?=-L /usr/lib/postgredql -lpq
SHARED_LIB_EXT?=so

PROGS=entropy import mkdist clear_dead_paths print_new_paths
SCRIPTS=focuser_bstep libpq_connect_string rebalance
SQL=focuser.psql import_upsert.plpgsql post_delta.plpgsql
MAN1=import.1 clear_dead_paths.1 print_new_paths.1 rebalance.1

LIBPOPULARITY=libpopularity.$(SHARED_LIB_EXT)

all: $(PROGS) $(LIBPOPULARITY)

clean:
	rm -f $(PROGS) libpopularity.so
	for file in $(ECPG_PROGS); do rm -f "$$file".c; done

entropy: entropy.c popularity.h
	cc $(CFLAGS) $(PQ_CFLAGS) -o $@ $< -lm

.SUFFIXES: .c .pgc
.pgc.c:
	ecpg $<

import: import.c sha1_of_file.c
	cc $(CFLAGS) -o $@ import.c sha1_of_file.c $(PQ_CFLAGS) $(PQ_LDFLAGS) -lcrypto

mkdist: mkdist.c
	cc $(CFLAGS) -o $@ $< $(PQ_CFLAGS) $(PQ_LDFLAGS) -lm

n-sn-ge-1: n-sn-ge-1.c
	cc $(CFLAGS) -o $@ $< $(ECPG_CFLAGS) $(ECPG_LDFLAGS) -lm

sn-down: sn-down.c
	cc $(CFLAGS) -o $@ $< $(ECPG_CFLAGS) $(ECPG_LDFLAGS) -lm

print_new_paths: print_new_paths.c 
	cc $(CFLAGS) -o $@ $< $(PQ_CFLAGS) $(PQ_LDFLAGS)

select-from-dist: select-from-dist.c libpopularity.so
	cc $(CFLAGS) -o $@ $< $(ECPG_CFLAGS) $(ECPG_LDFLAGS) $(LDFLAGS) -lpopularity

clear_dead_paths: clear_dead_paths.c
	cc $(CFLAGS) -o $@ $< $(PQ_CFLAGS) $(PQ_LDFLAGS)

$(LIBPOPULARITY): popularity.c popularity.h
	cc $(CFLAGS) -c -fpic -shared -o $@ popularity.c $(PQ_CFLAGS)

install:
	mkdir -p /usr/local/bin
	for prog in $(PROGS) $(SCRIPTS); do install -m 0755 $$prog /usr/local/bin/popularity_$$prog; done
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

uninstall:
	for prog in $(PROGS) $(SCRIPTS); do rm -f /usr/local/bin/popularity_$$prog; done
	rm -f /usr/local/lib/$(LIBPOPULARITY)
	rm -f /usr/local/include/popularity.h
	rm -rf /usr/local/share/popularity/common
	for file in $(MAN1); do rm -f $(MAN_PATH)/man1/$$file; done
	rm $(MAN_PATH)/man7/popularity.7

#IN GOD WE TRVST.

include Version.mk
CFLAGS+=-DVERSION=\"$(VERSION)\"

CFLAGS+=-Wall -g -fstack-protector -I ..
ECPG_CFLAGS?=-I /usr/include/postgresql
ECPG_LDFLAGS?=-L /usr/lib/postgresql -lecpg
LDFLAGS?=-L ..
PQ_CFLAGS?=-I /usr/include/postgresql
PQ_LDFLAGS?=-L /usr/lib/postgresql -lpq
XLI_PATH?=/usr/bin/xli

PROGS=slideshow_import slideshow_mkdist slideshow_request slideshow
SQL=import_upsert.plpgsql

all: $(PROGS)

clean:
	rm -f $(PROGS) enspool.c slideshow.c request.c fetch_counts_ecpg.c

.SUFFIXES: .c .pgc
.pgc.c:
	ecpg $<

slideshow_import: import.c ../sha1_of_file.c
	cc $(CFLAGS) -o $@ import.c ../sha1_of_file.c $(PQ_CFLAGS) $(PQ_LDFLAGS) -lcrypto

slideshow_mkdist: mkdist.c
	cc $(CFLAGS) -o $@ mkdist.c $(PQ_CFLAGS) $(PQ_LDFLAGS) $(LDFLAGS) -lm -lpopularity

slideshow: slideshow.c enspool.c enspool.h fetch_counts_ecpg.c
	cc -o $@ -DXLI_PATH=\"$(XLI_PATH)\" slideshow.c enspool.c fetch_counts_ecpg.c $(CFLAGS) $(ECPG_CFLAGS) $(LDFLAGS) $(ECPG_LDFLAGS) -lX11 -lm -lpopularity -lpq

slideshow_request: request.c enspool.c enspool.h fetch_counts_ecpg.c
	cc -o $@ request.c enspool.c fetch_counts_ecpg.c $(CFLAGS) $(ECPG_CFLAGS) $(LDFLAGS) $(ECPG_LDFLAGS) -lpopularity -lm -lpq

install:
	for prog in $(PROGS) find_photos; do install -m 0755 $$prog /usr/local/bin/popularity_$$prog; done
	mkdir -p /usr/local/share/popularity/slideshow/doc
	for doc in README; do install -m 0644 $$doc /usr/local/share/popularity/slideshow/doc/$$doc; done
	cp -a schema /usr/local/share/popularity/slideshow/
	mkdir -p /usr/local/share/popularity/slideshow/sql
	for file in $(SQL); do install -m 0644 $$file /usr/local/share/popularity/slideshow/sql/$$file; done

uninstall:
	for prog in $(PROGS) find_photos; do rm -f /usr/local/bin/popularity_$$prog; done
	rm -rf /usr/local/share/popularity/slideshow

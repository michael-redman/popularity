CFLAGS+=-Wall -g -fstack-protector -I ..
ECPG_CFLAGS?=-I /usr/include/postgresql
ECPG_LDFLAGS?=-L /usr/lib/postgresql -lecpg
LDFLAGS=-L ..
PQ_CFLAGS?=-I /usr/include/postgresql
PQ_LDFLAGS?=-L /usr/lib/postgresql -lpq
XLI_PATH?=/usr/bin/xli

PROGS=slideshow slideshow_request

all: $(PROGS)

clean:
	rm -f $(PROGS) import-scene.c mkdist-scenes.c enspool.c slideshow.c request.c fetch_counts_ecpg.c

.SUFFIXES: .c .pgc
.pgc.c:
	ecpg $<

slideshow: slideshow.c enspool.c enspool.h fetch_counts_ecpg.c
	cc -o $@ -DXLI_PATH=\"$(XLI_PATH)\" slideshow.c enspool.c fetch_counts_ecpg.c $(CFLAGS) $(ECPG_CFLAGS) $(LDFLAGS) $(ECPG_LDFLAGS) -lX11 -lm -lpopularity -lpq

slideshow_request: request.c enspool.c enspool.h fetch_counts_ecpg.c
	cc -o $@ request.c enspool.c fetch_counts_ecpg.c $(CFLAGS) $(ECPG_CFLAGS) $(LDFLAGS) $(ECPG_LDFLAGS) -lpopularity -lm -lpq

install:
	for prog in $(PROGS) find_photos; do install -m 0755 $$prog /usr/local/bin/popularity_$$prog; done
	mkdir -p /usr/local/share/popularity/slideshow/doc
	for doc in COPYRIGHT LICENSE README; do install -m 0644 $$doc /usr/local/share/popularity/slideshow/doc/$$doc; done
	mkdir -p /usr/local/share/popularity/slideshow
	install -m 0644 schema.psql /usr/local/share/popularity/slideshow/schema.psql

uninstall:
	for prog in $(PROGS) find_photos; do rm /usr/local/bin/popularity_$$prog; done
	rm -rf /usr/local/share/popularity/slideshow

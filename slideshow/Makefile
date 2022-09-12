include Version.mk
CFLAGS+=-DVERSION=\"$(VERSION)\"

CFLAGS+=-Wall -g -fstack-protector -I ..
PQ_CFLAGS?=-I /usr/include/postgresql
PQ_LDFLAGS?=-L /usr/lib/postgresql -lpq
LDFLAGS?=-L ..
PQ_CFLAGS?=-I /usr/include/postgresql
PQ_LDFLAGS?=-L /usr/lib/postgresql -lpq
XLI_PATH?=/usr/bin/xli

PROGS=slideshow_import slideshow_request slideshow
SQL=despool.plpgsql import_upsert.plpgsql

all: $(PROGS)

clean:
	rm -f $(PROGS)

.SUFFIXES: .c .pgc
.pgc.c:
	ecpg $<

slideshow_import: import.c ../sha1_of_file.c
	cc $(CFLAGS) -o $@ import.c ../sha1_of_file.c $(PQ_CFLAGS) $(PQ_LDFLAGS) -lcrypto

slideshow: slideshow.c enspool.c enspool.h
	cc -o $@ -DXLI_PATH=\"$(XLI_PATH)\" slideshow.c enspool.c $(CFLAGS) $(PQ_CFLAGS) $(LDFLAGS) $(PQ_LDFLAGS) -lX11 -lm -lpopularity -lpq

slideshow_request: request.c enspool.c enspool.h
	cc -o $@ request.c enspool.c $(CFLAGS) $(PQ_CFLAGS) $(LDFLAGS) $(PQ_LDFLAGS) -lpopularity -lm -lpq

install:
	for prog in $(PROGS) find_photos; do install -m 0755 $$prog /usr/local/bin/popularity_$$prog; done
	install -d /usr/local/share/popularity/slideshow/doc 
	for doc in README; do install -m 0644 $$doc /usr/local/share/popularity/slideshow/doc/$$doc; done
	install -d /usr/local/share/popularity/slideshow/tables
	install -m 0644 tables/* /usr/local/share/popularity/slideshow/tables
	install -d /usr/local/share/popularity/slideshow/sql
	install -m 0644 sql/* /usr/local/share/popularity/slideshow/sql
	#for file in $(SQL); do install -m 0644 sql/$$file /usr/local/share/popularity/slideshow/sql/$$file; done

uninstall:
	for prog in $(PROGS) find_photos; do rm -f /usr/local/bin/popularity_$$prog; done
	rm -rf /usr/local/share/popularity/slideshow

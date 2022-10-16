include Version.mk
CFLAGS+=-DVERSION=\"$(VERSION)\"

CFLAGS+=-Wall -g -fstack-protector -ftrapv -O2 -I /usr/local/include -I ../common
LDFLAGS+=-L /usr/local/lib -L ../common
MAN_PATH?=/usr/local/share/man
PQ_CFLAGS?=-I /usr/include/postgresql
PQ_LDFLAGS?=-L /usr/lib/postgresql -lpq
SOX_PLAY_PATH?='"/usr/bin/play"'

PROGS=player_import player
SCRIPTS=find_music find_videos
SQL=despool.plpgsql import_upsert.plpgsql

all: $(PROGS)

clean:
	rm -f $(PROGS)

.SUFFIXES: .c .pgc
.pgc.c:
	ecpg $<

player_import: import.c ../common/sha1_of_file.c
	cc $(CFLAGS) -o $@ import.c ../common/sha1_of_file.c $(PQ_CFLAGS) $(PQ_LDFLAGS) -lcrypto -lavformat

player: player.c enspool.c enspool.h pglisten.c
	cc -o $@ -DSOX_PLAY_PATH=$(SOX_PLAY_PATH) player.c enspool.c pglisten.c $(CPPFLAGS) $(CFLAGS) $(PQ_CFLAGS) $(LDFLAGS) $(PQ_LDFLAGS) -lm -lpopularity -lpthread -lpq

install:
	for prog in $(PROGS) $(SCRIPTS); do install -m 0755 $$prog /usr/local/bin/popularity_$$prog || exit; done
	install -m 0644 -D -t /usr/local/share/popularity/player/tables tables/*
	install -m 0644 -D -t /usr/local/share/popularity/player/sql sql/*
	install -m 0644 upgrade_from_65f5.psql /usr/local/share/popularity/player/

uninstall:
	for prog in $(PROGS) $(SCRIPTS); do rm -f /usr/local/bin/popularity_$$prog; done
	rm -rf /usr/local/share/popularity/player
	rm -f $(MAN_PATH)/man7/popularity-player.7
	for file in $(MAN1); do rm -f $(MAN_PATH)/man1/popularity_$$file; done

#IN GOD WE TRVST.

CFLAGS+=-Wall -g -fstack-protector -ftrapv -O2 -I /usr/local/include -I ..
LDFLAGS+=-L /usr/local/lib -L ..
MAN_PATH?=/usr/local/share/man
PQ_CFLAGS?=-I /usr/include/postgresql
PQ_LDFLAGS?=-L /usr/lib/postgresql -lpq
SOX_PLAY_PATH?='"/usr/bin/play"'

PROGS=player_import player_mkdist player
SCRIPTS=find_music find_videos player_init_db shuffler
SQL=despool.plpgsql import_upsert.plpgsql schema.psql
MAN1=player_init_db.1 shuffler.1

all: $(PROGS)

clean:
	rm -f $(PROGS)

.SUFFIXES: .c .pgc
.pgc.c:
	ecpg $<

player_import: import.c ../sha1_of_file.c
	cc $(CFLAGS) -o $@ import.c ../sha1_of_file.c $(PQ_CFLAGS) $(PQ_LDFLAGS) -lcrypto -lavformat

player_mkdist: mkdist.c
	cc $(CFLAGS) -o $@ $< $(PQ_CFLAGS) $(PQ_LDFLAGS) -lm

player: player.c enspool.c enspool.h pglisten.c
	cc -o $@ -DSOX_PLAY_PATH=$(SOX_PLAY_PATH) player.c enspool.c pglisten.c $(CPPFLAGS) $(CFLAGS) $(PQ_CFLAGS) $(LDFLAGS) $(PQ_LDFLAGS) -lm -lpopularity -lpthread -lpq

install:
	for prog in $(PROGS) $(SCRIPTS); do install -m 0755 $$prog /usr/local/bin/popularity_$$prog; done
	mkdir -p /usr/local/share/popularity/player/doc
	for doc in COPYRIGHT LICENSE README; do install -m 0644 $$doc /usr/local/share/popularity/player/doc/$$doc; done
	mkdir -p /usr/local/share/popularity/player/sql
	for file in $(SQL); do install -m 0644 $$file /usr/local/share/popularity/player/sql/$$file; done
	mkdir -p $(MAN_PATH)/man1
	mkdir -p $(MAN_PATH)/man7
	install -m 0644 player.7 $(MAN_PATH)/man7/popularity-player.7
	for file in $(MAN1); do install -m 0644 $$file $(MAN_PATH)/man1/popularity_$$file; done

uninstall:
	for prog in $(PROGS) $(SCRIPTS); do rm -f /usr/local/bin/popularity_$$prog; done
	rm -rf /usr/local/share/popularity/player
	rm -f $(MAN_PATH)/man7/popularity-player.7
	for file in $(MAN1); do rm -f $(MAN_PATH)/man1/popularity_$$file; done

#IN GOD WE TRVST.

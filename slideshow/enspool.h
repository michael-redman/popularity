#ifndef __POPULARITY_SLIDESHOW_SPOOLING_H
#define __POPULARITY_SLIDESHOW_SPOOLING_H

#include <libpq-fe.h>

char fetch_counts(PGconn *, int *, int *);
float random_time(unsigned int, unsigned int);
char enspool(PGconn *, char *, float, float);

#endif

/*IN GOD WE TRVST.*/

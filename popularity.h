#ifndef __POPULARITY_H
#define __POPULARITY_H

#include <libpq-fe.h>
#include <openssl/sha.h>
#include <time.h>

struct dist_elem{
	unsigned char hash[SHA_DIGEST_LENGTH];
	double cumul_density; };
char file_age(const char * const, time_t *);
void get_decay_period(const char * const, float *);
char random_from_dist_file(char const * const, char * const);
char random_from_distfiles(unsigned int const, char const ** const, char *const);

#endif

/* IN GOD WE TRVST */

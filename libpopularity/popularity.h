#ifndef __POPULARITY_H
#define __POPULARITY_H

#include <libpq-fe.h>
#include <openssl/sha.h>

struct dist_elem{
	char hash[2*SHA_DIGEST_LENGTH+1];
	double cumul_density; };

void get_decay_period(const char * const, float *);
char random_from_dist_file(char const * const, char * const);
char random_from_distfiles(unsigned int const, char const ** const, char *const);

#endif

/* IN GOD WE TRVST */

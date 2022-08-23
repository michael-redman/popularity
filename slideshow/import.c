#define USE "find .... -type f -print0 | popularity_import libpq_connect_string"

#include <libpq-fe.h>
#include <limits.h>
#include <openssl/sha.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "at.h"
#define AT __WHERE__
#define AT_ERR fputs(AT "\n",stderr)

extern char sha1_of_file (	const char * const path,
				unsigned char sum[SHA_DIGEST_LENGTH]);

int main(int argc, char ** argv){
	PGconn *db;
	PGresult *result;
	unsigned char hash_binary[SHA_DIGEST_LENGTH];
	char *path=NULL, hash[2*SHA_DIGEST_LENGTH+1];
	unsigned int i;
	int exit_status=0;
	size_t path_len=0;
	if	(argc!=2)
		{ fputs(USE "\n", stderr); return 1; }
	db=PQconnectdb(argv[1]);
	if	(PQstatus(db)!=CONNECTION_OK)
		{ fputs(PQerrorMessage(db),stderr); AT_ERR; return 1; }
	while(!feof(stdin)){
	        if (getdelim(&path,&path_len,'\0',stdin)==-1) break;
		if	(sha1_of_file(path,hash_binary))
			{ exit_status|=1; AT_ERR; goto label0; }
		for	(i=0;i<SHA_DIGEST_LENGTH;i++)
			if	(sprintf(&hash[2*i],"%02hhx",hash_binary[i])!=2)
				{ exit_status=1; AT_ERR; goto label0; }
		result=PQexecParams(db,"select import_upsert($1,$2)",2,NULL,
			(char const * const []){ hash, path }, NULL,
			(const int []){ 0,0 }, 0);
		if	(PQresultStatus(result)!=PGRES_TUPLES_OK)
			{	exit_status|=1;
				fputs(PQerrorMessage(db),stderr); AT_ERR;
				PQclear(result);
				goto label0; }
		PQclear(result);
		}
	if (ferror(stdin)) { exit_status=1; perror("getdelim"); AT_ERR;  }
	label0:	if (path) free(path);
		PQfinish(db);
		return exit_status; }

//IN GOD WE TRVST.

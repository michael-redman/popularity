#include <libpq-fe.h>
#include <limits.h>
#include <openssl/sha.h>
#include <stdlib.h>
#include <string.h>

#include <popularity.h>

#include "at.h"
#include "enspool.h"

int main(int argc, char ** argv){
PGconn * pg_conn;
PGresult *pg_result;
int pool_count,spool_count;
size_t path_len=0;
char *path=NULL, hash[2*SHA_DIGEST_LENGTH+1];
pg_conn=PQconnectdb(argv[1]);
if	(PQstatus(pg_conn)!=CONNECTION_OK)
	{ fputs(PQerrorMessage(pg_conn),stderr); WHERE; return 1; }
while(!feof(stdin)){
	if (getdelim(&path,&path_len,'\0',stdin)==-1) break;
	pg_result=PQexecParams(pg_conn,"select count(*) from pool where path=$1",1,NULL,(char const * const []){path},NULL,NULL,0);
	if	(PQresultStatus(pg_result)!=PGRES_TUPLES_OK)
		{ WHERE; PQclear(pg_result); return 1; }
	if(strcmp(PQgetvalue(pg_result,0,0),"1")){
		fprintf(stderr,"warning: unknown or ambiguous path: %s\n",path);
		PQclear(pg_result);
		continue; }
	pg_result=PQexecParams(pg_conn,"select hash from pool where path=$1",1,NULL,(char const * const []){path},NULL,NULL,0);
	if	(PQresultStatus(pg_result)!=PGRES_TUPLES_OK)
		{ WHERE; PQclear(pg_result); return 1; }
	strncpy(hash,PQgetvalue(pg_result,0,0),2*SHA_DIGEST_LENGTH+1);
	PQclear(pg_result);
	hash[2*SHA_DIGEST_LENGTH]=0;
	if	(fetch_counts(pg_conn,&pool_count,&spool_count))
		{ WHERE; return 1; }
	enspool(pg_conn,hash,random_time(pool_count,spool_count),0); }
free(path);
if(ferror(stdin)||fclose(stdin)){ perror("stdin:"); exit(EXIT_FAILURE); }
PQfinish(pg_conn);
return 0; }

/*IN GOD WE TRVST.*/

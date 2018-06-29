#include <libpq-fe.h>
#include <limits.h>
#include <openssl/sha.h>
#include <string.h>
#include <stdlib.h>

#include "at.h"

char enspool(PGconn * pg_conn, char * hash){
	char exit_status=0;
	PGresult * pg_result=PQexecParams(pg_conn,"insert into spool values($1)"
		,1,NULL, (char const * const []){ hash }, NULL,
		(const int []){0},0);
	if      (PQresultStatus(pg_result)!=PGRES_COMMAND_OK)
		{	fputs(PQerrorMessage(pg_conn),stderr); AT_ERR;
			exit_status|=1; }
	PQclear(pg_result);
	return exit_status; }

/* IN GOD WE TRVST */

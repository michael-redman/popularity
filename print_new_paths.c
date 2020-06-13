#include <libpq-fe.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "at.h"
#define AT_ERR fputs(AT "\n",stderr)
#define SQLERR do{ fputs(PQerrorMessage(pg_conn),stderr); }while(0)

int main(int argc, char ** argv){
	char *path=NULL;
	size_t path_len=0;
	PGconn *pg_conn;
	PGresult *pg_result;
	int exit_status=0;
	pg_conn=PQconnectdb(argv[1]);
        if	(PQstatus(pg_conn)!=CONNECTION_OK)
		{	SQLERR; AT_ERR; return 1; }
	while(!feof(stdin)){
	        if (getdelim(&path,&path_len,'\0',stdin)==-1) break;
		pg_result=PQexecParams(pg_conn,
			"SELECT COUNT(*) FROM POOL WHERE PATH=$1",1,NULL,
			(char const * const[]){ path },NULL,(int const[]){0},0);
		if	(PQresultStatus(pg_result)!=PGRES_TUPLES_OK)
			{	exit_status|=1; SQLERR; AT_ERR;
				PQclear(pg_result); break;}
		if	(!strcmp(PQgetvalue(pg_result,0,0),"0"))
			printf("%s%c",path,'\0');
		PQclear(pg_result); }
	if	(ferror(stdin)||fclose(stdin))
		{ exit_status|=1; perror("stdin"); AT_ERR; }
	if (path) free(path);
	PQfinish(pg_conn);
	return exit_status;
	}
/*IN GOD WE TRVST.*/

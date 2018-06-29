#include <libpq-fe.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define AT do{ fprintf(stderr,"at " __FILE__ ": %d\n",__LINE__); }while(0)
#define SQLERR do{ fputs(PQerrorMessage(pg_conn),stderr); }while(0)

int main(int argc, char ** argv){
	struct stat st;
	size_t path_len=0;
	int exit_status=0;
	PGconn *pg_conn;
	PGresult *pg_result;
	char *path=NULL;
	pg_conn=PQconnectdb(argv[1]);
	if      (PQstatus(pg_conn)!=CONNECTION_OK)
		{	SQLERR; AT; return 1; }
	//read paths to check from stdin so you don't have to do whole db
	while(!feof(stdin)){
		if (getdelim(&path,&path_len,'\0',stdin)==-1) break;
	        if(path[strlen(path)-1]=='\n') path[strlen(path)-1]='\0';
		if (!stat(path,&st)) continue;
		pg_result=PQexecParams(pg_conn,
			"UPDATE POOL SET PATH=NULL WHERE PATH=$1;",1,NULL,
			(char const * const[]){path},NULL,(int const []){0},0);
		if	(PQresultStatus(pg_result)!=PGRES_COMMAND_OK)
			{	exit_status|=1; SQLERR; AT;
				PQclear(pg_result); goto end; }
		PQclear(pg_result); }
	end:
	if (path) free(path);
	if	(ferror(stdin)||fclose(stdin))
		{ exit_status|=1; perror("stdin"); }
	PQfinish(pg_conn);
	return exit_status; }

/*IN GOD WE TRVST.*/

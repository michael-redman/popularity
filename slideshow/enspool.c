#define _GNU_SOURCE
#include <libpq-fe.h>
#include <math.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <popularity.h>

#include "enspool.h"

#ifndef RANDOM_MAX
#define RANDOM_MAX RAND_MAX
#endif


#include <libpq-fe.h>
#include <stdio.h>

#include "at.h"
#define SQLERR do{ fputs(PQerrorMessage(pg_conn),stderr); }while(0)

char fetch_counts(PGconn * pg_conn, int * pool_count, int * spool_count)
{	PGresult *pg_result=PQexec(pg_conn,"select known_paths_count(), count_estimate('spool')");
	if	(PQresultStatus(pg_result)!=PGRES_TUPLES_OK)
		{	SQLERR; WHERE;
			PQclear(pg_result); return 1; }
	if	(sscanf(PQgetvalue(pg_result,0,0),"%d",pool_count)!=1)
		{ WHERE; PQclear(pg_result); return 1; }
	if	(sscanf(PQgetvalue(pg_result,0,1),"%d",spool_count)!=1)
		{ WHERE; PQclear(pg_result); return 1; }
	PQclear(pg_result);
	return 0; }

float random_time(unsigned int pool_count, unsigned int spool_count){
	double lambda, uniform,random_time;
	lambda=(1+(double)spool_count)/sqrt(pool_count);
	uniform=random()/(double)RAND_MAX;
	random_time=-log(uniform)/lambda;
	//fprintf(stderr,"random_time: %lg\n",random_time);
	return (float)random_time; }

char enspool(PGconn *pg_conn, char * hash, float display_time, float delta_time){
	PGresult * pg_result;
	char * display_time_s, *delta_time_s;
	if	(asprintf(&display_time_s,"%f",display_time)==-1)
		{ WHERE; return 1; }
	if	(asprintf(&delta_time_s,"%f",delta_time)==-1)
		{	free(display_time_s);
			WHERE; return 1; }
	pg_result=PQexecParams(pg_conn,
		"INSERT INTO SPOOL VALUES ($1,$2,$3);",3,NULL,
		(char const * const[]){ hash, display_time_s, delta_time_s },
		NULL,NULL,0);
	free(display_time_s);
	free(delta_time_s);
	if      (PQresultStatus(pg_result)!=PGRES_COMMAND_OK)
		{ SQLERR; WHERE; PQclear(pg_result); return 1; }
	return 0; }

/* IN GOD WE TRVST */

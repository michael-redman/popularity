#define USE "popularity-mkdist <-r|-p> distfile libpq_conn_string"

#include <errno.h>
#include <libpq-fe.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "popularity.h"

double r_to_rplus(double x){
	if (x>=0) return 1+x;
	else return 1/(1-x); }

#define DIE do{ perror(argv[optind]); exit(EXIT_FAILURE); }while(0)
#define AT do{ fprintf(stderr,"at " __FILE__ ": %d\n",__LINE__); }while(0)
#define SQLERR do{ fputs(PQerrorMessage(pg_conn),stderr); }while(0)

int main(int argc, char ** argv){
	int exit_status=0, iterator, set_count, n_nodes=0;
	PGconn *pg_conn;
	PGresult *pg_result;
	double popularity, age, denominator;
	long pool_count;
	float length=1;
	struct dist_elem node;
	FILE *distfile;
	char *query,*pool_count_s,*avg_age_s, dbtype;
	if (argc!=4) { fputs(USE "\n", stderr); return 1; }
	if	(strcmp(argv[1],"-r") && strcmp(argv[1],"-p"))
		{ fputs(USE "\n", stderr); exit(EXIT_FAILURE); }
	dbtype=argv[1][1];
	node.cumul_density=0;
	if ((distfile=fopen(argv[2],"w+b"))==NULL) DIE;
	pg_conn=PQconnectdb(argv[3]);
	if	(PQstatus(pg_conn)!=CONNECTION_OK)
		{ SQLERR; AT; exit(EXIT_FAILURE); }
	pg_result=PQexec(pg_conn,"select count(*), avg(extract(epoch from now()-last_vote_time)) from pool where path is not null");
	if	(PQresultStatus(pg_result)!=PGRES_TUPLES_OK)
		{ SQLERR; AT; PQclear(pg_result); exit(EXIT_FAILURE); }
	if	(sscanf(PQgetvalue(pg_result,0,0),"%ld",&pool_count)!=1)
		{ AT; PQclear(pg_result); exit(EXIT_FAILURE); }
	if	(!(pool_count_s=strdup(PQgetvalue(pg_result,0,0))))
		{ AT; PQclear(pg_result); exit(EXIT_FAILURE); }
	if	(!(avg_age_s=strdup(PQgetvalue(pg_result,0,1))))
		{ AT; PQclear(pg_result); exit(EXIT_FAILURE); }
	PQclear(pg_result);
	pg_result=PQexecParams(pg_conn,"insert into dictionary values('cache/known_paths_count',$1) on conflict(key) do update set value=excluded.value;",1,NULL,(char const * const []){pool_count_s},NULL,NULL,0); //This value gets used when the pool and spool counts are inputs to the probability logic for drawing from spool vs distfiles and for speeding/slowing display initerval
	free(pool_count_s);
	if	(PQresultStatus(pg_result)!=PGRES_COMMAND_OK)
		{ SQLERR; AT; PQclear(pg_result); exit(EXIT_FAILURE); }
	PQclear(pg_result);
	pg_result=PQexecParams(pg_conn,"insert into dictionary values('cache/average_time_since_last_vote',$1) on conflict(key) do update set value=excluded.value;",1,NULL,(char const * const []){avg_age_s},NULL,NULL,0); //This value gets used by post_delta
	free(avg_age_s);
	if	(PQresultStatus(pg_result)!=PGRES_COMMAND_OK)
		{ SQLERR; AT; PQclear(pg_result); exit(EXIT_FAILURE); }
	PQclear(pg_result);
	pg_result=PQexec(pg_conn,"BEGIN");
	if	(PQresultStatus(pg_result)!=PGRES_COMMAND_OK)
		{ SQLERR; AT; PQclear(pg_result); exit(EXIT_FAILURE); }
	PQclear(pg_result);
	if	(dbtype=='r')
		pg_result=PQexec(pg_conn,"DECLARE foo CURSOR FOR select hash, votes, extract(epoch from now()-last_end_time), duration from pool where path is not null;");
		else pg_result=PQexec(pg_conn,"DECLARE foo CURSOR FOR select hash, votes, extract(epoch from now()-last_end_time), coalesce(count,1) from pool left join set_counts_cache on pool.set=set_counts_cache.set where path is not null;");
	if	(PQresultStatus(pg_result)!=PGRES_COMMAND_OK)
		{ SQLERR; AT; PQclear(pg_result); exit(EXIT_FAILURE); }
	PQclear(pg_result);
	while(1){
		pg_result=PQexec(pg_conn,"fetch foo");
		if	(PQresultStatus(pg_result)!=PGRES_TUPLES_OK)
			{ SQLERR; AT; PQclear(pg_result); exit(EXIT_FAILURE); }
		if	(!PQntuples(pg_result))
			{ PQclear(pg_result); break; }
		for	(iterator=0;iterator<SHA_DIGEST_LENGTH;iterator++)
			if	(sscanf(&PQgetvalue(pg_result,0,0)[2*iterator],"%02hhx",&node.hash[iterator])!=1)
				{ AT; PQclear(pg_result); exit(EXIT_FAILURE); }
		popularity=strtod(PQgetvalue(pg_result,0,1),NULL);
		if	(errno==EINVAL || errno==ERANGE)
			{ perror("strtod"); AT; PQclear(pg_result); exit(EXIT_FAILURE);}
		age=strtod(PQgetvalue(pg_result,0,2),NULL);
		if	(errno==EINVAL || errno==ERANGE)
			{ perror("strtod"); AT; PQclear(pg_result); exit(EXIT_FAILURE);}
		if	(dbtype=='r')
			{if	(sscanf(PQgetvalue(pg_result,0,3),"%f",&length)!=1)
				{ AT; exit_status|=1; exit(EXIT_FAILURE); }}
			else	 if	(sscanf(PQgetvalue(pg_result,0,3),"%d",&set_count)!=1)
					{ AT; PQclear(pg_result); exit(EXIT_FAILURE); }
		PQclear(pg_result);
		if	(dbtype=='r')
			node.cumul_density+=r_to_rplus(popularity*length+age/pool_count)/length;  //popularity is up-votes (count, dimensionless), length is play duration (seconds), age is time since end of last play (seconds), pool_count is a count, dimensionless.
			else node.cumul_density+=r_to_rplus(popularity+age/pool_count)/sqrt(set_count); //Age is seconds since end of last display - in photos DB votes is upvote display time in seconds so this is dimensionally consistent
		if	(fwrite(&node,sizeof(node),1,distfile)!=1)
			{ perror("fwrite"); AT; exit(EXIT_FAILURE); }
		n_nodes++; }
	pg_result=PQexec(pg_conn,"close foo;");
	if	(PQresultStatus(pg_result)!=PGRES_COMMAND_OK)
		{ SQLERR; AT; PQclear(pg_result); exit(EXIT_FAILURE); }
	PQclear(pg_result);
	if	(exit_status)
		query="abort";
		else query="commit";
	pg_result=PQexec(pg_conn,query);
	if	(PQresultStatus(pg_result)!=PGRES_COMMAND_OK)
		{ SQLERR; AT; PQclear(pg_result); exit(EXIT_FAILURE); }
	PQfinish(pg_conn);
	denominator=node.cumul_density;
	if	(fseek(distfile,0,SEEK_SET)==-1)
		{ perror(argv[2]); AT; exit(EXIT_FAILURE); }
	for	(iterator=0;iterator<n_nodes;iterator++)
		{	if	(fread(&node,sizeof(node),1,distfile)!=1)
				{ perror(argv[2]); AT; exit(EXIT_FAILURE); }
			if	(fseek(distfile,-sizeof(node),SEEK_CUR)==-1)
				{ perror(argv[2]); AT; exit(EXIT_FAILURE); }
			node.cumul_density/=denominator;
			if	(fwrite(&node,sizeof(node),1,distfile)!=1)
				{ perror("fwrite"); AT; exit(EXIT_FAILURE); } }
	if (fclose(distfile)) { exit_status|=1; perror("fclose"); AT; }
	return exit_status;
	}
/* IN GOD WE TRVST */

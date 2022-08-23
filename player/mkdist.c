#define USE "popularity-mkdist distfile libpq_conn_string"

#include <errno.h>
#include <libpq-fe.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "popularity.h"

/*
double wf_regular(double popularity, float weight){
	if(popularity>=0) return pow(1+popularity,weight);
	else return pow(1-popularity,-weight); }

double wf_inverse(double popularity, float weight) {
	if	(popularity>=0)
		return pow(1+popularity,-weight);
		else return pow(1-popularity,weight); }

double wf_joint(double votes, double age) {
	if(votes>=0) return votes+age;
	else return 1/(1-votes)+age; }
*/

double r_to_rplus(double x){
	if (x>=0) return 1+x;
	else return 1/(1-x); }

#define DIE do{ perror(argv[optind]); exit(EXIT_FAILURE); }while(0)
#define AT do{ fprintf(stderr,"at " __FILE__ ": %d\n",__LINE__); }while(0)
#define SQLERR do{ fputs(PQerrorMessage(pg_conn),stderr); }while(0)

int main(int argc, char ** argv){
	int exit_status=0, iterator;
	PGconn *pg_conn;
	PGresult *pg_result;
	double popularity, age;
	long pool_count;
	//double (*wf)(double,float);
	struct dist_elem node;
	//float strength=1;
	FILE *distfile;
	char *query,*pool_count_s,*avg_age_s;
	float length=1;
	/*if	(getopt(argc,argv,"v")=='v')
		wf=wf_inverse;
		else wf=wf_regular;*/
	if (argc-optind!=2) { fputs(USE "\n", stderr); return 1; }
	node.cumul_density=0;
	//strength=strtod(argv[optind+2],(char **)NULL);
	if	(errno==ERANGE || errno==EINVAL)
		{ fputs(USE "\n",stderr); AT; return 1; }
	if ((distfile=fopen(argv[optind],"wb"))==NULL) DIE;
	pg_conn=PQconnectdb(argv[optind+1]);
	if	(PQstatus(pg_conn)!=CONNECTION_OK)
		{	exit_status|=1;
			SQLERR;
			AT; goto label0; }
	pg_result=PQexec(pg_conn,"select count(*), avg(extract(epoch from now()-last_vote_time)) from pool where path is not null");
	if	(PQresultStatus(pg_result)!=PGRES_TUPLES_OK)
		{	exit_status|=1; SQLERR; AT;
			PQclear(pg_result); goto label1; }
	if	(sscanf(PQgetvalue(pg_result,0,0),"%ld",&pool_count)!=1)
		{	exit_status|=1; AT;
			PQclear(pg_result); goto label1; }
	//fprintf(stderr,"mkdist: count: %ld\n",pool_count);
	if	(!(pool_count_s=strdup(PQgetvalue(pg_result,0,0))))
		{	exit_status|=1; AT;
			PQclear(pg_result); goto label1; }
	if	(!(avg_age_s=strdup(PQgetvalue(pg_result,0,1))))
		{	exit_status|=1; AT;
			PQclear(pg_result); goto label1; }
	/*avg_votes=strtod(PQgetvalue(pg_result,0,1),NULL);
	if	(errno==EINVAL || errno==ERANGE)
		{	exit_status|=1;
			perror("mkdist: could not read avg_votes"); AT;
			PQclear(pg_result); goto label1; }*/
	//fprintf(stderr,"mkdist: avg_votes: %lf\n",avg_votes);
	/*avg_time_since_last_vote=strtod(PQgetvalue(pg_result,0,2),NULL);
	if	(errno==EINVAL || errno==ERANGE)
		{	exit_status|=1;
			perror("mkdist: could not read avg_time_since_last_vote"); AT;
			PQclear(pg_result); goto label1; }*/
	//fprintf(stderr,"mkdist: avg_time_since_last_vote: %lg\n",avg_time_since_last_vote);
	/*avg_length=strtod(PQgetvalue(pg_result,0,3),NULL);
	if	(errno==EINVAL || errno==ERANGE)
		{	exit_status|=1;
			perror("mkdist: could not read avg_length"); AT;
			PQclear(pg_result); goto label1; }*/
	//fprintf(stderr,"mkdist: avg_length: %lg\n",avg_length);
	PQclear(pg_result);
	pg_result=PQexecParams(pg_conn,"insert into dictionary values('cache/known_paths_count',$1) on conflict(key) do update set value=excluded.value;",1,NULL,(char const * const []){pool_count_s},NULL,NULL,0); //This value gets used when the pool and spool counts are inputs to the probability logic for drawing from spool vs distfiles and for speeding/slowing display initerval
	free(pool_count_s);
	if	(PQresultStatus(pg_result)!=PGRES_COMMAND_OK)
		{	exit_status|=1; SQLERR; AT;
			PQclear(pg_result); goto label1; }
	pg_result=PQexecParams(pg_conn,"insert into dictionary values('cache/average_time_since_last_vote',$1) on conflict(key) do update set value=excluded.value;",1,NULL,(char const * const []){avg_age_s},NULL,NULL,0); //This value gets used by post_delta
	free(avg_age_s);
	if	(PQresultStatus(pg_result)!=PGRES_COMMAND_OK)
		{	exit_status|=1; SQLERR; AT;
			PQclear(pg_result); goto label1; }
	PQclear(pg_result);
	pg_result=PQexec(pg_conn,"BEGIN");
	if	(PQresultStatus(pg_result)!=PGRES_COMMAND_OK)
		{	exit_status|=1; SQLERR; AT;
			PQclear(pg_result); goto label1; }
	PQclear(pg_result);
	pg_result=PQexec(pg_conn,"DECLARE foo CURSOR FOR select hash, duration, votes, extract(epoch from now()-last_vote_time) from pool where path is not null;");
	if	(PQresultStatus(pg_result)!=PGRES_COMMAND_OK)
		{	exit_status|=1; SQLERR; AT;
			PQclear(pg_result); goto finish_transaction; }
	PQclear(pg_result);
	cursor_loop:
	pg_result=PQexec(pg_conn,"fetch foo");
	if	(PQresultStatus(pg_result)!=PGRES_TUPLES_OK)
		{	exit_status|=1; SQLERR; AT;
			PQclear(pg_result); goto close_cursor; }
	if	(!PQntuples(pg_result))
		{ PQclear(pg_result); goto close_cursor; }
	for	(iterator=0;iterator<SHA_DIGEST_LENGTH;iterator++)
		if	(sscanf(&PQgetvalue(pg_result,0,0)[2*iterator],"%02hhx",&node.hash[iterator])!=1)
			{       exit_status|=1; AT;
				PQclear(pg_result); goto close_cursor; }
	//strcpy(node.hash,PQgetvalue(pg_result,0,0));
	if	(sscanf(PQgetvalue(pg_result,0,1),"%f",&length)!=1)
		{ AT; exit_status|=1; goto close_cursor; }
	popularity=strtod(PQgetvalue(pg_result,0,2),NULL);
	if	(errno==EINVAL || errno==ERANGE)
		{	exit_status|=1; perror("strtod"); AT;
			PQclear(pg_result); goto close_cursor; }
	age=strtod(PQgetvalue(pg_result,0,3),NULL);
	if	(errno==EINVAL || errno==ERANGE)
		{	exit_status|=1; perror("strtod"); AT;
			PQclear(pg_result); goto close_cursor; }
	/*if	(file_age(PQgetvalue(pg_result,0,4),&file_age_v))
		{	exit_status|=1;
			fprintf(stderr,"mkdist: could not get file_age() for %s\n",PQgetvalue(pg_result,0,4)); AT;
			PQclear(pg_result); goto close_cursor; }*/
	PQclear(pg_result);
	node.cumul_density+=r_to_rplus(popularity*length+age/pool_count)/length;  //popularity is up-votes (count, dimensionless), length is play duration (seconds), age is time since last vote (seconds), pool_count is a count, dimensionless.
	if	(fwrite(&node,sizeof(node),1,distfile)!=1)
		{ exit_status|=1; perror("fwrite"); AT; goto close_cursor; }
	goto cursor_loop;
	close_cursor:
	pg_result=PQexec(pg_conn,"close foo;");
	if	(PQresultStatus(pg_result)!=PGRES_COMMAND_OK)
		{	exit_status|=1; SQLERR; AT;
			PQclear(pg_result); goto label1; }
	PQclear(pg_result);
	finish_transaction:
	if	(exit_status)
		query="abort";
		else query="commit";
	pg_result=PQexec(pg_conn,query);
	if	(PQresultStatus(pg_result)!=PGRES_COMMAND_OK)
		{	exit_status|=1; SQLERR; AT;
			PQclear(pg_result); }
	label1:	PQfinish(pg_conn);
	label0:	if (fclose(distfile)) { exit_status|=1; perror("fclose"); AT; }
	return exit_status;
	}
/* IN GOD WE TRVST */

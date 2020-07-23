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
	char *query;
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
	pg_result=PQexec(pg_conn,"select count(*) from pool where path is not null");
	if	(PQresultStatus(pg_result)!=PGRES_TUPLES_OK)
		{	exit_status|=1; SQLERR; AT;
			PQclear(pg_result); goto label1; }
	if	(sscanf(PQgetvalue(pg_result,0,0),"%ld",&pool_count)!=1)
		{	exit_status|=1; AT;
			PQclear(pg_result); goto label1; }
	pg_result=PQexec(pg_conn,"BEGIN");
	if	(PQresultStatus(pg_result)!=PGRES_COMMAND_OK)
		{	exit_status|=1; SQLERR; AT;
			PQclear(pg_result); goto label1; }
	PQclear(pg_result);
	pg_result=PQexec(pg_conn,"DECLARE foo CURSOR FOR select hash, duration, votes, extract(epoch from now()-time) from pool where path is not null;");
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
	PQclear(pg_result);
	node.cumul_density+=r_to_rplus(popularity*length+age/pool_count);
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

#ifdef EXTRA_FEATURES
#include <errno.h>
#include <libpq-fe.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>

extern pthread_mutex_t mutex;
extern char hash[], die_flag;
extern pid_t player_pid;
extern int down_votes, delta;

#undef AT
#define AT fprintf(stderr,"at " __FILE__ ": %d\n",__LINE__)
#define SQLERR fputs(PQerrorMessage(db),stderr)

char * convert_connect_string(char * const ecpg_connect_string)
{	char * at, *libpq_connect_string=malloc(
		strlen(ecpg_connect_string)
		+sizeof("dbname= host="));
	if	(!libpq_connect_string)
		{ AT; return NULL; }
	at=strchr(ecpg_connect_string,'@');
	if (!at) at=&ecpg_connect_string[strlen(ecpg_connect_string)];
	strcpy(libpq_connect_string,"dbname=");
	strncpy(
		&libpq_connect_string[7],
		ecpg_connect_string,
		at-ecpg_connect_string);
	libpq_connect_string[7+at-ecpg_connect_string]='\0';
	if	(*at)
		sprintf(&libpq_connect_string[7+at-ecpg_connect_string],
			" host=%s", &at[1]);
	fputs(libpq_connect_string,stderr);
	return libpq_connect_string;
	}

/*int main(int argc, char ** argv)
{	char *string=convert_connect_string("lille");
	if (string) free(string);
	return 0; } */

static void close_db(void *db) { PQfinish(db); }

static char log_vote(PGconn *pg_conn, char const * const vote){
	//mutex for hash already locked when this function called
	char return_value=0;
	#ifdef VOTE_LOG
	PGresult *pg_result;
	pg_result=PQexecParams(pg_conn,
		"insert into vote_log values(now(),$1,$2)",
		2, NULL,
		(char const * const[]){ hash, vote },
		(const int []){ 0,0 },
		(const int []){ 0,0 },
		0);
	if	(PQresultStatus(pg_result)!=PGRES_COMMAND_OK)
		{ AT; return_value|=1; }
	PQclear(pg_result);
	#endif
	return return_value; }

void * pglisten_f(void *db_connect_str)
{	PGconn *db;
	PGresult *result;
	PGnotify *notify;
	int _socket, r0, down_votes_to_kill;
	char notify_err=0;
	fd_set input_mask;
	if	(!db_connect_str)
		{ AT; return (void *)1; }
	db=PQconnectdb(db_connect_str);
	if	(PQstatus(db)!=CONNECTION_OK)
		{ AT; return (void *)1; }
	pthread_cleanup_push(close_db,db);
	result=PQexec(db,"LISTEN voting");
	if	(PQresultStatus(result)!=PGRES_COMMAND_OK)
		{ AT; pthread_exit((void *)1); }
	PQclear(result);
	loop:	_socket=PQsocket(db); //libpq example code re-gets socket id each loop iteration - no memory leak?
		if (_socket<0) { AT; pthread_exit((void *)1); }
		FD_ZERO(&input_mask);
		FD_SET(_socket,&input_mask);
		select_label:
		if (die_flag) pthread_exit((void *)0);
		r0=select(_socket+1,&input_mask,NULL,NULL,NULL);
		if	(r0==-1)
			{	if	(errno==EINTR)
					{	if (die_flag) pthread_exit((void *)0);
						//fputs("EINTR\n",stderr); AT;
						goto select_label; }
				perror("select"); AT;
				PQfinish(db); pthread_exit((void *)1); }
		PQconsumeInput(db);
		while	((notify=PQnotifies(db))!=NULL)
			{	if	(!notify->extra)
					{ notify_err|=1;  AT; goto notify_end; }
				fprintf(stderr,"Received %c from pglisten\n",notify->extra[0]);
				switch	(notify->extra[0])
				{	case 'u':
						if	(pthread_mutex_lock(&mutex))
							{ AT; notify_err|=1; goto notify_end; }
						result=PQexecParams(db,
							"insert into spool values($1)",
							1, NULL,
							(char const * const[]){ hash },
							(const int []){ 0 },
							(const int []){ 0 },
							0);
						if	(PQresultStatus(result)!=PGRES_COMMAND_OK)
							{	AT;
								notify_err|=1; }
						PQclear(result);
						if	(notify_err)
							goto upvote_unlock_mutex;
						if	(log_vote(db,"+"))
							{	AT;
								notify_err|=1; }
						upvote_unlock_mutex:
						if	(pthread_mutex_unlock(&mutex))
							{ AT; notify_err|=1; }
						if (notify_err) goto notify_end;
						break;
					case 'd':
						if	(pthread_mutex_lock(&mutex))
							{ AT; notify_err|=1; goto notify_end; }
						delta--;
						down_votes++;
						result=PQexec(db,"select value from dictionary where key='down_votes_to_kill'");
						if	(PQresultStatus(result)!=PGRES_TUPLES_OK)
							{	SQLERR; AT; notify_err|=1;
								goto downvote_query_end; }
						if	(!PQntuples(result))
							{	fputs("PQntuples: could not read 'down_votes_to_kill' from dictionary table.\n",stderr); AT;
								notify_err|=1; goto downvote_query_end; }
						if	(sscanf(PQgetvalue(result,0,0),"%d",&down_votes_to_kill)!=1)
							{	fputs("sscanf: could not read 'down_votes_to_kill' from dictionary table.\n",stderr); AT;
								notify_err|=1; }
						downvote_query_end:
						PQclear(result);
						if (notify_err) goto downvote_unlock_mutex;
						if	(down_votes>=down_votes_to_kill)
							kill(player_pid,SIGKILL);
						if	(log_vote(db,"-"))
							{ AT; notify_err|=1; }
						downvote_unlock_mutex:
						if	(pthread_mutex_unlock(&mutex))
							{ AT; notify_err|=1; }
						break;
					default: AT; notify_err|=1; }
					notify_end:
					PQfreemem(notify);
					if (notify_err) pthread_exit((void *)1);
				}
		goto loop;
	pthread_cleanup_pop(1);
	}

#endif

//IN GOD WE TRVST.

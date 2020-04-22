#define USE "popularity_player libpq_connection_string distfile [...]"

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <libpq-fe.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <popularity.h>

#include "enspool.h"

extern void * pglisten_f(void *);

char hash[2*SHA_DIGEST_LENGTH+1], die_flag=0;
int delta, control_fd, down_votes;
pid_t player_pid;
sem_t semaphore;
pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;

#define AT do{ fprintf(stderr,"at " __FILE__ ": %d\n",__LINE__); }while(0)
#define SQLERR do{ fputs(PQerrorMessage(pg_conn),stderr); }while(0)

void signal_handler(int signal){ sem_post(&semaphore); }

void * command_loop(void * arg)
{	char c;
	int r0,r1;
	long exit_status=0;
	PGconn * pg_conn;
	pg_conn=PQconnectdb(arg);
	if      (PQstatus(pg_conn)!=CONNECTION_OK)
		{ SQLERR; AT; return (void *)1; }
	while	(!die_flag) {
		sem_wait(&semaphore);
		if (die_flag) break;
		do {	read_label:
			r0=read(control_fd,&c,1);
			if	(r0<0)
				{	if	(errno==EINTR||errno==EAGAIN||errno==EWOULDBLOCK)
						{	if (die_flag) break;
							//fputs("sleeping...\n",stderr);
							//AT;
							sleep(1);
							goto read_label; }
					exit_status|=1;
					perror("read"); AT; goto end;}
			if (!r0) continue;
			switch(c){
				case 'q': die_flag=1; break;
				case 'r':
					trylock:
					r1=pthread_mutex_trylock(&mutex);
					if	(r1==EBUSY)
						{	fputs("sleeping...\n",stderr); AT;
							sleep(2);
							goto trylock; }
					if (r1) {
						exit_status|=1;
						AT;
						die_flag=1;
						goto end; }
					if	(enspool(pg_conn,hash))
						{	exit_status|=1;
							AT;
							if	(pthread_mutex_unlock(&mutex))
								{	perror("pthread_mutex_unlock");
									AT; }
							goto end; }
					if	(pthread_mutex_unlock(&mutex))
						{	exit_status|=1;
							perror("pthread_mutex_unlock");
							AT; goto end; };
					break;
				default: fprintf(stderr,"unknown command from fifo: %c\n",c); }
			} while(r0); }
	end:	PQfinish(pg_conn);
		return (void *)exit_status; }


char fetch_counts(PGconn * pg_conn, int * pool_count, int * spool_count){
	static int pool_count_cache=-1;
	PGresult *pg_result;
	if	(pool_count_cache==-1)
		{	pg_result=PQexec(pg_conn, "SELECT COUNT(*) FROM pool WHERE PATH IS NOT NULL;");
			if	(PQresultStatus(pg_result)!=PGRES_TUPLES_OK)
				{ SQLERR; AT; PQclear(pg_result); return 1; }
			if	(sscanf(PQgetvalue(pg_result,0,0),"%d",&pool_count_cache)!=1)
				{ AT; PQclear(pg_result); return 1; }
			PQclear(pg_result);
			*pool_count=pool_count_cache; }
	pg_result=PQexec(pg_conn, "SELECT COUNT(*) FROM spool where not exists (select * from play_log where play_log.hash=spool.hash and extract(epoch from(now()-end_time)) < (select cast(value as integer) from dictionary where key='replay_delay'))");
	if	(PQresultStatus(pg_result)!=PGRES_TUPLES_OK)
		{ SQLERR; AT; PQclear(pg_result); return 1; }
	if	(sscanf(PQgetvalue(pg_result,0,0),"%d",spool_count)!=1)
		{ AT; PQclear(pg_result); return 1; }
	PQclear(pg_result);
	return 0; }
	/* EXEC SQL SELECT COUNT(*) INTO :spool_count_ FROM SPOOL INNER JOIN PHOTOS ON SPOOL.hash = PHOTOS.hash WHERE PATH IS NOT NULL;
		slow query???
	EXEC SQL SELECT COUNT(*) INTO :spool_count_ FROM SPOOL WHERE EXISTS (SELECT * FROM PHOTOS WHERE PHOTOS.HASH=SPOOL.HASH AND PATH IS NOT NULL);
		if you assume the spool contains only current photos you save a lot of computation: */

char random_from_distfiles_replay_delay
(	int const ndists,
	char const ** const distfiles,
	char * const hash,
	PGconn *pg_conn)
{	int tries, replay_delay;
	PGresult *pg_result;
	pg_result=PQexec(pg_conn,"select value from dictionary where key='replay_delay'");
	if	(PQresultStatus(pg_result)!=PGRES_TUPLES_OK)
		{ SQLERR; AT; PQclear(pg_result); return 1; }
	if	(!PQntuples(pg_result))
		{	fputs("Did not find entry in table 'dictionary' for key 'replay_delay' - please put it back if you deleted it. Fatal.\n",stderr);
			AT; PQclear(pg_result); return 1; }
	if	(sscanf(PQgetvalue(pg_result,0,0),"%d",&replay_delay)!=1)
		{ AT; PQclear(pg_result); return 1; }
	PQclear(pg_result);
	#define REPLAY_DELAY_TRIES 50
	for	(tries=0;tries<REPLAY_DELAY_TRIES;tries++)
		{	if	(random_from_distfiles(ndists,distfiles,hash))
				{ AT; return 1; }
			pg_result=PQexecParams(pg_conn,"select * from play_log where hash=$1 and extract(epoch from (now()-play_log.end_time))<(select cast(value as integer) from dictionary where key='replay_delay') limit 1",
				1,NULL,(char const * const[]){ hash }, NULL,
				(int const[]){0},0);
			if	(PQresultStatus(pg_result)!=PGRES_TUPLES_OK)
				{	SQLERR; AT; PQclear(pg_result);
					return 1; }
			if	(!PQntuples(pg_result))
				{ PQclear(pg_result); break; }
			PQclear(pg_result); }
	if (tries==REPLAY_DELAY_TRIES) fputs("Warning: random_from_distfiles_replay_delay: tried maximum number of times, playing last selection despite not meeting replay_delay\n",stderr);
	return 0; }

static int despool(PGconn *pg_conn)
//Returns: 0 (Status OK, hash despooled); 1 (General err); 2 (Spool empty)
{	PGresult *pg_result=PQexec(pg_conn,"select * from despool()");
	if	(PQresultStatus(pg_result)!=PGRES_TUPLES_OK)
		{ SQLERR; AT; PQclear(pg_result); return 1; }
	if	(!PQntuples(pg_result))
		{ AT; PQclear(pg_result); return 1; }
	if	(!strcmp(PQgetvalue(pg_result,0,0),"Z"))
		{ PQclear(pg_result); return 2; }
	strncpy(hash,PQgetvalue(pg_result,0,0),2*SHA_DIGEST_LENGTH);
	if	(sscanf(PQgetvalue(pg_result,0,1),"%d",&delta)!=1)
		{	fputs("failed reading delta from spool\n",stderr); AT;
			return 1; }
	PQclear(pg_result);
	hash[2*SHA_DIGEST_LENGTH]='\0';
	fprintf(stderr,"popularity_player read from spool: %s %d\n",hash,delta);
	return 0; }

/*
inline void scrobble(){
	EXEC SQL BEGIN DECLARE SECTION;
	char title[1024],artist[1024],length[40],_path[PATH_MAX+1];
	EXEC SQL END DECLARE SECTION;
	int pid,exit_status;
	strcpy(_path,path);
	EXEC SQL SELECT title, artist, cast(length as varchar) into :title, :artist, :length from pool where path=:_path;
	if(title[0]=='\0'||artist[0]=='\0'||length[0]=='\0'){
		printf("warning, could not scrobble %s\n",path);
		return; }
	pid=fork();
	if(!pid) execl("/usr/lib/lastfmsubmitd/lastfmsubmit","/usr/lib/lastfmsubmitd/lastfmsubmit","--artist",artist,"--title",title,"--length",length,NULL);
	wait(&exit_status);
	}
*/

static char log_play_end(PGconn * pg_conn, char const * const hash)
{
	#ifdef PLAY_LOG
	PGresult * pg_result = PQexecParams(pg_conn,
		"insert into play_log values(now(),$1)",1,NULL,
		(char const * const[]){ hash }, NULL, (int const[]){0}, 0);
	if	(PQresultStatus(pg_result)!=PGRES_COMMAND_OK)
		{ SQLERR; AT; PQclear(pg_result); return 1; }
	PQclear(pg_result);
	#endif
	return 0; }

char play(PGconn *pg_conn){
	char path[PATH_MAX+1], path_url[PATH_MAX+8], *_delta;
	struct stat stat_struct;
	int return_value=0, child_exit_status, snprintf_exit_status;
	PGresult * pg_result=PQexecParams(pg_conn,
		"select path from pool where hash=$1",1,NULL,
		(char const * const []){ hash }, NULL,
		(const int []){ 0 }, 0);
	if	(PQresultStatus(pg_result)!=PGRES_TUPLES_OK)
		{ SQLERR; AT; PQclear(pg_result); return 1; }
	if	(!PQntuples(pg_result))
		{ AT; PQclear(pg_result); return 1; }
	if	(!PQgetvalue(pg_result,0,0))
		{	fprintf(stderr,"Warning: path is null for hash %s, skipping play\n",hash);
			return 0; }
	strcpy(path,PQgetvalue(pg_result,0,0));
	PQclear(pg_result);
	if	(puts(path)==EOF)
		{ AT; return 1; }
	if(stat(path,&stat_struct)) { perror(path); return 1; }
	player_pid=fork();
	if(player_pid==-1){
		fprintf(stderr,"could not fork.\n");
		return 1;
	}if(!player_pid){
		if	(	!strcasecmp(&path[strlen(path)-4],".avi")
				|| !strcasecmp(&path[strlen(path)-4],".flv")
				|| !strcasecmp(&path[strlen(path)-4],".mkv")
				|| !strcasecmp(&path[strlen(path)-4],".mpg")
				|| !strcasecmp(&path[strlen(path)-4],".mp4")
				|| !strcasecmp(&path[strlen(path)-4],".mov")
				|| !strcasecmp(&path[strlen(path)-4],".vob")
				|| !strcasecmp(&path[strlen(path)-4],".wmv"))
			{	snprintf_exit_status=snprintf(
					path_url,
					PATH_MAX+1,
					"file://%s",path);
				if	(snprintf_exit_status<0||snprintf_exit_status>=PATH_MAX+1)
					{	AT;
						exit(1); }
					execl(  "/usr/bin/vlc", "/usr/bin/vlc",
						"--play-and-exit","-f",path_url,NULL);
				}
			if	(	!strcmp(&path[strlen(path)-5],".flac")
					||!strcmp(&path[strlen(path)-4],".mp3")
					||!strcmp(&path[strlen(path)-4],".mp2")
					||!strcmp(&path[strlen(path)-4],".ogg"))
				execl(
					SOX_PLAY_PATH,
					SOX_PLAY_PATH,
					"-q",path,NULL);
		fprintf(stderr,"don't know how to play: %s\n",path);
		exit(1); }
	if	(pthread_mutex_unlock(&mutex))
		{ AT; return_value|=1; }
	waitpid(player_pid,&child_exit_status,0);
	if	(WIFEXITED(child_exit_status)&&WEXITSTATUS(child_exit_status))
		{ fputs("child returned nonzero\n",stderr); AT; return 1; }
	if (return_value) return return_value;
	if	(pthread_mutex_lock(&mutex))
		{ AT; return_value|=1; }
	if (return_value) return return_value;
	if(WIFSIGNALED(child_exit_status))
		--delta;
		/* else scrobble(); */
	if	(delta)
		{	if	(asprintf(&_delta,"%d",delta)<0)
				{ AT; return_value|=1; goto play_unlock; }
			fprintf(stderr,"popularity_player posting delta %s %d\n",hash,delta);
			pg_result=PQexecParams(pg_conn,
				"SELECT post_delta($1,$2)",2,NULL,
				(char const * const []){ hash,_delta },
				NULL, NULL, 0 );
			free(_delta);
			if	(PQresultStatus(pg_result)!=PGRES_TUPLES_OK)
				{ SQLERR; AT; return_value|=1; }
			PQclear(pg_result); }
	if (return_value) goto play_unlock;
	if	(log_play_end(pg_conn,hash))
		{ AT; return_value|=1; }
	play_unlock:
	if	(pthread_mutex_unlock(&mutex))
		{ AT; return_value|=1; }
	return return_value;
	}

char next(PGconn *pg_conn, int ndists, char const ** const distfiles){
	int pool_count, spool_count, exit_status=0;
	int r;
	down_votes=0;
	do {	r=pthread_mutex_lock(&mutex);
		if (r==EBUSY) {
			fputs("Waiting for mutex\n",stderr); AT;
			sleep(2);
			continue; }
		if (r) { AT; return 1; }
		} while(r);
	if	(fetch_counts(pg_conn,&pool_count,&spool_count))
		{	exit_status|=1; AT; goto label0; }
	if	(	(	random()/(double)RAND_MAX
				*((float)pool_count+(float)spool_count*(float)spool_count)
				>= ((float)spool_count*(float)spool_count)))
		{	if	(random_from_distfiles_replay_delay(
					ndists,distfiles,
					hash,pg_conn))
				{ exit_status|=1; AT; goto label0; }
			delta=0; }
		else{	r=despool(pg_conn);
			switch(r){
				case 0: break;
				case 1: exit_status|=1; AT; goto label0;
				case 2:	if      (random_from_distfiles(ndists,distfiles,hash))
						{ exit_status=1; AT; goto label0; }
					delta=0;
					break;
				default: exit_status|=1; AT; }}
	label0: if	(exit_status)
			{	if	(pthread_mutex_unlock(&mutex))
					{ exit_status|=1; AT; }
				goto label1; }
	//play unlocks mutex
	if (play(pg_conn)) { exit_status=1; AT; }
	label1: return exit_status; }

char pipe_path[PATH_MAX+1];

int main(int argc, char** argv){
	int r0=0;
	void * r2;
	#ifndef __CYGWIN__
	int r1;
	struct sigaction sa;
	pthread_t command_loop_thread;
	struct stat _stat;
	#endif
	pthread_t pglisten_thread;
	PGconn *pg_conn;
	if	(sem_init(&semaphore,0,0))
		{ AT; r0|=1; goto e_1; }
	#ifndef __CYGWIN__
	r1=snprintf(pipe_path,PATH_MAX+1,"/tmp/popularity-%s.ctl",argv[1]);
	if (r1>=PATH_MAX+1||r1<0) { AT; r0|=1; goto e0; }
	pipe_path[PATH_MAX]='\0';
	if (sigemptyset(&sa.sa_mask)) { AT; r0|=1; goto e0; }
	sa.sa_handler=signal_handler;
	sa.sa_flags=SA_RESTART;
	if (sigaction(SIGIO,&sa,NULL)) { AT; r0|=1; goto e0; }
	if (!stat(pipe_path,&_stat) && S_ISFIFO(_stat.st_mode)) goto open_pipe;
	if 	(mkfifo(pipe_path,0600))
		{ perror(pipe_path); AT; r0|=1; goto e0; }
	open_pipe:
	control_fd=open(pipe_path,O_RDONLY|O_NONBLOCK);
	if	(control_fd==-1)
		{ perror(pipe_path); AT; r0|=1; goto e1; }
	if	(	fcntl(control_fd,F_SETFL,O_ASYNC|O_NONBLOCK)
			|| fcntl(control_fd,F_SETOWN,getpid()))
		{ perror(pipe_path); AT; r0|=1; goto e2; }
	#endif
        pg_conn=PQconnectdb(argv[1]);
        if      (PQstatus(pg_conn)!=CONNECTION_OK)
                {       r0|=1; SQLERR; AT; goto e2; }
	srandom(time(NULL));
	#ifndef __CYGWIN__
	if	(pthread_create(&command_loop_thread,NULL,command_loop,argv[1]))
		{ AT; r0|=1; goto e3; }
	#endif
	if	(pthread_create(
			&pglisten_thread,NULL,
			pglisten_f,argv[1]))
		{ AT; r0|=1; goto e4; }
	do	{	if	(next(pg_conn,argc-2,(char const ** const)&(argv[2])))
				{ AT; r0|=1; die_flag=1; } }
		while(!die_flag);
	pthread_kill(pglisten_thread,SIGIO); //return value not checked because thread might already be dead on its own
	if	(pthread_join(pglisten_thread,&r2))
		{ AT; r0|=1;  goto e4; }
	e4:
		#ifndef __CYGWIN__
		pthread_kill(command_loop_thread,SIGIO); //return value not checked because thread might already be dead on its own
		if	(pthread_join(command_loop_thread,&r2))
			{ AT; r0|=1;  goto e3; }
		if (r2!=(void *)0) { AT; r0|=1; }
	e3:
		#endif
		PQfinish(pg_conn);
	e2:
		#ifndef __CYGWIN__
		if (close(control_fd)) { perror(pipe_path); AT; r0|=1; }
	e1:	if (unlink(pipe_path)) { perror(pipe_path); AT; r0|=1; }
	e0:
		#endif
		if (sem_destroy(&semaphore)) {  perror(pipe_path); AT; r0|=1; }
	e_1:	return r0; }

/*IN GOD WE TRVST.*/

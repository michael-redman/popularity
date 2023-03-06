//Use: popularity_slideshow 'host=abc dbname=def' .cache/popularity/photos.dist

#define _GNU_SOURCE
#include <errno.h>
#include <libpq-fe.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <popularity.h>

#include "enspool.h"
#define NSECS_IN_SEC 1000000000
#define RESOLUTION_NSEC 50000000

#include "at.h"
#define AT __WHERE__
#define AT_ERR fputs(AT "\n",stderr);
#define SQLERR do{ fputs(PQerrorMessage(pg_conn),stderr); }while(0)

int despool
	(	PGconn * pg_conn,
		char * hash,
		float * display_time,
		float * delta_time)
	{	PGresult * pg_result;
		pg_result=PQexec(pg_conn,"select * from despool()");
		if	(PQresultStatus(pg_result)!=PGRES_TUPLES_OK)
			{	SQLERR; WHERE;
				PQclear(pg_result); exit(EXIT_FAILURE); }
		if	(!PQntuples(pg_result))
			{ PQclear(pg_result); exit(EXIT_FAILURE); }
		if	(PQgetisnull(pg_result,0,0))
			{ PQclear(pg_result); return -1; }
		strncpy(hash,PQgetvalue(pg_result,0,0),2*SHA_DIGEST_LENGTH+1);
		hash[2*SHA_DIGEST_LENGTH]=0; //should be redundant
		if	(sscanf(PQgetvalue(pg_result,0,1),"%f",display_time)!=1)
			{ WHERE; PQclear(pg_result); exit(EXIT_FAILURE); }
		if	(sscanf(PQgetvalue(pg_result,0,2),"%f",delta_time)!=1)
			{ WHERE; PQclear(pg_result); exit(EXIT_FAILURE); }
		PQclear(pg_result);
		fprintf(stderr,"despool: read %s %g %g\n",hash,*display_time,*delta_time);
		return 0; }

char get_path(PGconn *pg_conn, const char * const hash, char * path)
{	PGresult *pg_result;
	pg_result=PQexecParams(pg_conn,"select path from pool where hash=$1",1,NULL, (char const * const []){ hash }, NULL, NULL, 0);
	if	(PQresultStatus(pg_result)!=PGRES_TUPLES_OK)
		{	fputs(PQerrorMessage(pg_conn),stderr); AT_ERR;
			PQclear(pg_result);
			return -1 ; }
	strncpy(path,PQgetvalue(pg_result,0,0),PATH_MAX+1);
	path[PATH_MAX]=0;
	PQclear(pg_result);
	return 0; }

char next
	(	PGconn * pg_conn,
		unsigned int ndists,
		char const ** const distfiles,
		char * hash,
		float * display_time,
		float * delta_time,
		char * path)
	{	int pool_count, spool_count;
		if	(fetch_counts(pg_conn,&pool_count,&spool_count))
			{ WHERE; exit(EXIT_FAILURE); }
		if(random()/(double)RAND_MAX*(pool_count+spool_count*spool_count)<(spool_count*spool_count))
			if	(!despool(pg_conn,hash,display_time,delta_time)) //exits prog on err returns nonzero on empty spool
				{	if	(get_path(pg_conn,hash,path))
						{ WHERE; exit(EXIT_FAILURE); }
					return 0; }
		if	(random_from_distfiles(ndists,distfiles,hash))
			{ AT_ERR; return 1; }
		if	(get_path(pg_conn,hash,path))
			{ WHERE; exit(EXIT_FAILURE); }
		*display_time=random_time(pool_count,spool_count);
		*delta_time=0;
		return 0; }

void handlequeuedevents
	(
		PGconn * pg_conn,
		Display *d,
		char * hash,
		float * display_time,
		float * delta_time,
		int * die, int waste)
	{
		int pool_count, spool_count;
		XEvent e;
		XKeyEvent k;
		KeySym ks;
		XComposeStatus cs;
		char c[2];
		float interval;
		c[1]=0;
		while	(XPending(d)){
			XNextEvent(d,&e);
			k=e.xkey;
			XLookupString(&k,c,1,&ks,&cs);
			if(c[0]=='q') *die=1;
			if(waste) continue;
			fetch_counts(pg_conn,&pool_count, &spool_count);
			interval=random_time(pool_count,spool_count);
			switch(c[0]){
			case '-':
				//fputs("- event\n",stderr);
				if	(interval>*display_time)
					interval=*display_time;
				*delta_time-=interval;
				*display_time-=interval;
				break;
			case '=':
				//fputs("+ event\n",stderr);
				/*if(random()%maxtime>=interval){
					*display_time+=interval;
					*delta_time+=interval;
				}else*/
				enspool(pg_conn,hash,interval,interval);
				break;
			/* case '+': request_from_same_scene(hash); */ }}}

void paint(PGconn * pg_conn, char * hash, char * path){
	struct stat st;
	const int bufsize=1920;
	char buf[bufsize];
	int pid,exit_status, stderr_fds[2], stdout_fds[2], fno;
	FILE *xli_stderr, *xli_stdout;
	static char last_path[PATH_MAX+1]="\0";
	size_t bytes;
	PGresult *pg_result;
	if	(!strcmp(path,last_path))
		{	fprintf(stderr,
				"Not repainting repeated image: %s\n",path);
			return; }
	strncpy(last_path,path,PATH_MAX+1);
	last_path[PATH_MAX]='\0';
	if(stat(path,&st)){
		fprintf(stderr,"could not stat %s\n",path);
		exit(EXIT_FAILURE); }
	if	(pipe(stderr_fds))
		{ perror("pipe: "); AT_ERR; exit(EXIT_FAILURE); }
	if	(pipe(stdout_fds))
		{ perror("pipe: "); AT_ERR; exit(EXIT_FAILURE); }
	pid=fork();
	if(pid==-1){
		fprintf(stderr,"could not fork\n");
		exit(-1); }
	if	(!pid)
		{	if	(close(stderr_fds[0]))
				{ perror("close"); AT_ERR; exit(EXIT_FAILURE); }
			fno=fileno(stderr);
			if	(fno==-1)
				{ perror("fileno"); AT_ERR; exit(EXIT_FAILURE);}
			if	(dup2(stderr_fds[1],fno)==-1)
				{ perror("dup2"); AT_ERR; exit(EXIT_FAILURE); }
			if	(close(stdout_fds[0]))
				{ perror("close"); AT_ERR; exit(EXIT_FAILURE); }
			fno=fileno(stdout);
			if	(fno==-1)
				{ perror("fileno"); AT_ERR; exit(EXIT_FAILURE);}
			if	(dup2(stdout_fds[1],fno)==-1)
				{ perror("dup2"); AT_ERR; exit(EXIT_FAILURE); }
			execl(XLI_PATH,XLI_PATH,"-onroot","-fullscreen","-quiet","-at","0,0","-border","black",path,NULL);
			perror("execl"); exit(EXIT_FAILURE); }
	if	(close(stderr_fds[1]))
		{ perror("close"); AT_ERR; exit(EXIT_FAILURE); }
	exit_status=0;
	xli_stderr=fdopen(stderr_fds[0],"r");
	if	(!xli_stderr)
		{ perror("fdopen"); AT_ERR; exit(EXIT_FAILURE);}
	bytes=fread(buf,1,bufsize-1,xli_stderr);
	if	(!bytes)
		{if	(ferror(xli_stderr))
			{	perror("fread: "); AT_ERR;
				exit(EXIT_FAILURE); }
			else goto cleanup_xli_streams;}
	buf[bytes]=0;
	pg_result=PQexecParams(pg_conn,"select errmsg from xli_errs_to_ignore where hash=$1",1,NULL, (char const * const []){ hash }, NULL, NULL, 0);
	if	(PQresultStatus(pg_result)!=PGRES_TUPLES_OK)
		{	fputs(PQerrorMessage(pg_conn),stderr); AT_ERR;
			PQclear(pg_result);
			exit(EXIT_FAILURE); }
	if	(!PQntuples(pg_result)||strcmp(buf,PQgetvalue(pg_result,0,0)))
		{	fputs("Unrecognized xli err:\n",stderr);
			fputs(buf,stderr);
			exit(EXIT_FAILURE); }
	PQclear(pg_result);
	fputs("Ignored xli err matched with database\n",stderr);
	cleanup_xli_streams:
	if	(fclose(xli_stderr))
		{ perror("fclose"); AT_ERR; exit(EXIT_FAILURE); }
	if	(close(stdout_fds[1]))
		{ perror("close"); AT_ERR; exit(EXIT_FAILURE); }
	xli_stdout=fdopen(stdout_fds[0],"r");
	if	(!xli_stdout)
		{ perror("fdopen"); AT_ERR; exit(EXIT_FAILURE);}
	do	{	if	(!fgets(buf,bufsize,xli_stdout))
				{	if	(ferror(xli_stdout))
						{	perror("fgets"); AT_ERR;
							exit(EXIT_FAILURE); }
					break; }
			if	(fputs(buf,stderr)==EOF)
				{ perror("fputs"); AT_ERR; exit(EXIT_FAILURE); }
			exit_status|=1;
		}while(1);
	if	(fclose(xli_stdout))
		{ perror("fclose"); AT_ERR; exit(EXIT_FAILURE); }
	if	(exit_status)
		{	fputs("xli wrote output, considering as err\n",stderr);
			exit(EXIT_FAILURE); }
	//xli returns 0 even on err
	waitpid(pid,&exit_status,0); }

#define DIE do{ \
	fprintf(stderr,__FILE__": error at line %u\n",__LINE__); \
	exit(EXIT_FAILURE); }while(0);

char post_delta(PGconn *pg_conn, const char * const hash, const float delta_time)
{	char * delta_time_s;
	PGresult *pg_result;
	if	(asprintf(&delta_time_s,"%f",delta_time)==-1)
		{ AT_ERR; return -1; }
	pg_result=PQexecParams(pg_conn,"select post_delta($1,$2)",2,NULL, (char const * const []){ hash, delta_time_s }, NULL, NULL, 0);
	free(delta_time_s);
	if	(PQresultStatus(pg_result)!=PGRES_TUPLES_OK)
		{	fputs(PQerrorMessage(pg_conn),stderr); AT_ERR;
			PQclear(pg_result);
			return -1 ; }
	PQclear(pg_result);
	return 0; }

int main(int argc, char** argv){
	char hash[2*SHA_DIGEST_LENGTH+1], *delta_time_s;
	float delta_time=0;
	PGconn *pg_conn;
	PGresult *pg_result;
	Display * d;
	Window w;
	int dieflag=0;
	float display_time=0;
	char path[PATH_MAX+1];
	struct timespec rqt, rmt;
	puts("Version: " VERSION);
	if(!(d=XOpenDisplay(NULL))) DIE;
	w=XCreateSimpleWindow(d,XDefaultRootWindow(d),0,0,4,4,1,0,0xffffff);
	if
		(w==BadAlloc || w==BadColor || w==BadCursor || w==BadMatch || w==BadPixmap || w==BadValue || w==BadWindow)
		DIE;
	XSetIconName(d,w,"slideshow");
	if (XMapWindow(d,w)==BadWindow) DIE;
	if (XSelectInput(d,w,KeyPress)==BadWindow) DIE;
	
	pg_conn=PQconnectdb(argv[1]);
	if	(PQstatus(pg_conn)!=CONNECTION_OK)
		{ fputs(PQerrorMessage(pg_conn),stderr); AT_ERR; return 1; }

	srandom(time(NULL));
	rqt.tv_sec=0;
	rqt.tv_nsec=RESOLUTION_NSEC;
	
	while(1){
		if	(next(	pg_conn,argc-2,(char const ** const)&(argv[2]),hash,
				&display_time,&delta_time,path))
			{ AT_ERR; return 1; }
		printf("%g %g %s %s\n",display_time,delta_time,hash,path);
		paint(pg_conn,hash,path);
		handlequeuedevents(pg_conn,d,hash, &display_time, &delta_time, &dieflag,1);
		while (display_time>0) {
			if	(nanosleep(&rqt,&rmt))
				{	if	(errno!=EINTR)
						{ AT_ERR; return 1; }
					display_time-=(rqt.tv_sec-rmt.tv_sec);
					display_time-=
						(float)(rqt.tv_nsec-rmt.tv_nsec)
						/(float)NSECS_IN_SEC; }
				else 	display_time-=rqt.tv_sec+(float)rqt.tv_nsec/(float)NSECS_IN_SEC;
			handlequeuedevents(pg_conn,d,hash, &display_time, &delta_time, &dieflag,0);
			if(dieflag) goto END; }
		if	(delta_time!=0)
			{	if	(asprintf(&delta_time_s,"%f",delta_time)==-1)
					{ AT_ERR; return -1; }
				pg_result=PQexecParams(pg_conn,"select post_delta($1,$2)",2,NULL, (char const * const []){ hash, delta_time_s }, NULL, NULL, 0);
				free(delta_time_s);
				if	(PQresultStatus(pg_result)!=PGRES_TUPLES_OK)
					{	fputs(PQerrorMessage(pg_conn),stderr); AT_ERR;
						PQclear(pg_result);
						return -1 ; }
				PQclear(pg_result);
				}
			else{	//post_delta updates last_end_time
				if	(update_last_end_time(pg_conn,hash))
					{ AT_ERR; exit(EXIT_FAILURE); }}
		}
	END:	if(display_time>0)
			{if	(enspool(pg_conn,hash,display_time,delta_time))
				{ AT_ERR; exit(EXIT_FAILURE); }}
		else{	if	(delta_time!=0)
				{	if	(post_delta(pg_conn,hash,delta_time)!=0)
						{ AT_ERR; return -1; } }
				else if	(update_last_end_time(pg_conn,hash))
					{ AT_ERR; exit(EXIT_FAILURE); }}
		PQfinish(pg_conn);
		XCloseDisplay(d);
		return 0; }

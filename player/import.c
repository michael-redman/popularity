#define USE "find .... -type f -print0 | popularity_import libpq_connect_string"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

//#include <libavformat/avformat.h>
#include <libpq-fe.h>
#include <limits.h>
#include <openssl/sha.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "at.h"

extern char sha1_of_file (	const char * const path,
				unsigned char sum[SHA_DIGEST_LENGTH]);
/*
char length(char * file, float * length){
	AVFormatContext* pFormatCtx = avformat_alloc_context();
	if	(!pFormatCtx)
		{ fputs("could not alloc avcontext - probably could not open file?\n",stderr); return 1; }
	if	(avformat_open_input(&pFormatCtx, file, NULL, NULL))
		{	fprintf(stderr,"libavformat_open_input: could not open input for %s\n", file);
			if(pFormatCtx) avformat_free_context(pFormatCtx);
			return 1; }
	if	(!pFormatCtx)
		{ fputs("pFormatCtx null after call to open_input\n",stderr); return 1; }
	if	(avformat_find_stream_info(pFormatCtx,NULL))
		{	fprintf(stderr,"avformat_find_stream returned nonzero for %s\n",file);
			if(pFormatCtx) avformat_free_context(pFormatCtx);
			return 1; }
	if	(!pFormatCtx->duration)
		{ fputs("failed reading duration\n",stderr); return 1; }
	*length = pFormatCtx->duration*1.0/AV_TIME_BASE;
	avformat_close_input(&pFormatCtx);
	avformat_free_context(pFormatCtx);
	return 0; } */

int main(int argc, char ** argv){
	PGconn *db;
	PGresult *result;
	unsigned char hash_binary[SHA_DIGEST_LENGTH];
	char *path=NULL, hash[2*SHA_DIGEST_LENGTH+1], length_s[]="0";
	unsigned int i;
	int count,exit_status=0;
	size_t path_len=0;
	//float length_fl;
	if	(argc!=2)
		{ fputs(USE "\n", stderr); return 1; }
	db=PQconnectdb(argv[1]);
	if	(PQstatus(db)!=CONNECTION_OK)
		{ fputs(PQerrorMessage(db),stderr); AT_ERR; return 1; }
	while(!feof(stdin)){
	        if (getdelim(&path,&path_len,'\0',stdin)==-1) break;
		result=PQexecParams(db,
			"select count(*) from pool where path=$1",
			1,NULL,
			(char const * const []){ path }, NULL,
			(const int []){ 0 }, 0);
		if	(PQresultStatus(result)!=PGRES_TUPLES_OK)
			{	exit_status|=1;
				fputs(PQerrorMessage(db),stderr); AT_ERR;
				PQclear(result);
				goto final0; }
		if	(sscanf(PQgetvalue(result,0,0),"%d",&count)!=1)
			{	exit_status|=1;
				AT_ERR;
				PQclear(result);
				goto final0; }
		PQclear(result);
		if (count) continue; //skip known paths
		if	(sha1_of_file(path,hash_binary))
			{ exit_status|=1; AT_ERR; goto final0; }
		for	(i=0;i<SHA_DIGEST_LENGTH;i++)
			if	(sprintf(&hash[2*i],"%02hhx",hash_binary[i])!=2)
				{ exit_status=1; AT_ERR; goto final0; }
		/*if	(length(path,&length_fl))
			{ AT_ERR; exit_status|=1; goto final0; }
		if	(asprintf(&length_s,"%f",length_fl)==-1)
			{ AT_ERR; exit_status|=1; goto final0; }
		fprintf(stderr,"read length %s\n",length_s);*/
		result=PQexecParams(db,
			"select import_upsert($1,$2,$3)",
			3, NULL,
			(char const * const []){ hash, path, length_s }, NULL,
			(const int []){ 0,0,0 }, 0);
		//free(length_s);
		if	(PQresultStatus(result)!=PGRES_TUPLES_OK)
			{	exit_status|=1;
				fputs(PQerrorMessage(db),stderr); AT_ERR;
				PQclear(result);
				goto final0; }
		PQclear(result);
		}
	if (ferror(stdin)) { exit_status=1; perror("getdelim"); AT_ERR;  }
	final0:	if (path) free(path);
		PQfinish(db);
		return exit_status; }

//IN GOD WE TRVST.

#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>

#include "at.h"
#define AT_ERR fputs(AT "\n",stderr)

#define BLOCK_SIZE 1048576

char sha1_of_file
(const char * const path, unsigned char sum[SHA_DIGEST_LENGTH])
{	SHA_CTX sha1;
	FILE * stream;
	unsigned char buf[BLOCK_SIZE];
	unsigned int bytes_read;
	char exit_status=0;
	if (SHA1_Init(&sha1)!=1) { AT_ERR; return 1; }
	stream=fopen(path,"rb");
	if (!stream) { exit_status|=1; AT_ERR; goto label0; }
	while	(!feof(stream))
		{	bytes_read=fread(buf,1,BLOCK_SIZE,stream);
			if (ferror(stream)) { AT_ERR; goto label1; }
			if (!bytes_read) break;
			if	(SHA1_Update(&sha1,buf,bytes_read)!=1)
				{ AT_ERR; goto label1; } }
	label1:	if (fclose(stream)) { exit_status|=1; perror(path); AT_ERR; }
	label0:	if (SHA1_Final(sum,&sha1)!=1) { exit_status|=1; AT_ERR; }
		return exit_status; }

/* Copyright 2017 Michael Redman
IN GOD WE TRVST.*/

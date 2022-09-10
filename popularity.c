#include <libpq-fe.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "at.h"
#include "popularity.h"

#define AT_ERR fputs(AT "\n",stderr);
#define SQLERR do{ fputs(PQerrorMessage(pg_conn),stderr); }while(0)

static char read_density(FILE * stream, unsigned int offset, double * density){
	struct dist_elem e;
	if	(	(fseek(stream,offset*sizeof(struct dist_elem),SEEK_SET) ==-1)
			|| (fread(&e,sizeof(struct dist_elem),1,stream)!=1))
		{ perror("read_density"); return 0; }
	*density=e.cumul_density;
	return 0; }

char random_from_dist_file (char const * const path, char * const hash) {
	FILE * stream;
	long dist;
	unsigned int midpoint,low=0,high;
	double key, density;
	struct dist_elem e;
	int iterator;
	if	(!(stream=fopen(path,"rb")))
		{ AT_ERR; perror(path); return 1; }
	if	(	 fseek(stream,0,SEEK_END)==-1
			|| (dist=ftell(stream))==-1)
		{ AT_ERR; perror(path); goto e0; }
	dist/=sizeof(struct dist_elem);
	/* binary search - var dist now holds number of elements */
	high=dist-1;
	if	(read_density(stream,dist-1,&density))
		{ AT_ERR; goto e0; }
	key=random()*density/RAND_MAX;
	while(high-low>1){
		midpoint=(high+low)/2;
		if	(read_density(stream,midpoint,&density))
			{ AT_ERR; goto e0; }
		if(density>=key) high=midpoint;
		else low=midpoint; }
	if	(read_density(stream,low,&density))
		{ AT_ERR; goto e0; }
	if(density>=key) dist=low;
	else dist=high;
	/* dist now holds offset of the item you want */
	if	(	fseek(stream,dist*sizeof(struct dist_elem),SEEK_SET)==-1
			|| fread(&e,sizeof(struct dist_elem),1,stream)!=1)
		{ AT_ERR; perror(path); goto e0; }
	for	(iterator=0;iterator<SHA_DIGEST_LENGTH;iterator++)
		sprintf(&hash[2*iterator],"%02hhx",e.hash[iterator]);
	hash[2*SHA_DIGEST_LENGTH]='\0';
	fprintf(stderr,"Read hash: %s\n",hash);
	//strcpy(hash,e.hash);
	/* testing code */
		if	(key>e.cumul_density)
			{	fprintf(stderr,"bsearch fault, key>item\n");
				AT_ERR; goto e0; }
		if	(dist>0)
			{	if	(	fseek(stream,(dist-1)*sizeof(struct dist_elem),SEEK_SET)
						|| fread(&e,sizeof(struct dist_elem),1,stream)!=1)
					{	perror("bsearch test");
						AT_ERR; goto e0; }
					else if	(key<=e.cumul_density)
						{	fprintf(stderr,"bsearch fault, key<=(item-1)\n");
							AT_ERR; goto e0; }}
		/*end testing code*/
	if(fclose(stream)){ AT_ERR; perror(path); return 1; }
	return 0;
	e0:	if (fclose(stream)) { AT_ERR; perror(path); }
		return 1; }

char random_from_distfiles
	(	unsigned int const ndists,
		char const ** const distfiles,
		char * const hash){
	unsigned int file;
	file=random()%ndists;
	if	(random_from_dist_file(distfiles[file],hash))
		{ AT_ERR; return 1; }
	return 0; }


char update_last_end_time(PGconn * pg_conn, char const * const hash)
{
	PGresult * pg_result = PQexecParams(pg_conn,
		"update pool set last_end_time=now() where hash=$1",1,NULL,
		(char const * const[]){ hash }, NULL, (int const[]){0}, 0);
	if	(PQresultStatus(pg_result)!=PGRES_COMMAND_OK)
		{ SQLERR; AT_ERR; PQclear(pg_result); return 1; }
	PQclear(pg_result);
	return 0; }

/* IN GOD WE TRVST */

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "popularity.h"

#define DIE do{ perror(argv[1]); exit(EXIT_FAILURE); }while(0)

int main(int argc, char ** argv){
	FILE * stream;
	double p,old_cumul_density=0,entropy=0,sum;
	struct dist_elem e;
	if
		(
			(!(stream=fopen(argv[1],"rb")))
			|| (
				fseek(
					stream,
					-sizeof(struct dist_elem),
					SEEK_END)
				==-1)
			|| (fread(&e,sizeof(struct dist_elem),1,stream)!=1)
			|| (fseek(stream,0,SEEK_SET)==-1))
		DIE;
	sum=e.cumul_density;
	while(!feof(stream)){
		fread(&e,sizeof(struct dist_elem),1,stream);
		if(ferror(stream)) DIE;
		p=(e.cumul_density-old_cumul_density)/sum;
		old_cumul_density=e.cumul_density;
		if(p>0) entropy+=p*log(p);
	}entropy/=-log(2);
	printf("%f bits %f states\n",entropy,pow(2,entropy));
	return 0; }
/*IN GOD WE TRVST.*/

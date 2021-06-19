#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

#define T0 1623391917

char file_age(const char * const path, time_t * age){
	struct stat stat_buf;
	time_t now;
	if	((now=time(NULL))==-1)
		{	fputs("file_age: could not get current time\n",stderr);
			return 1; }
	if	(stat(path,&stat_buf))
		{	fprintf(stderr,"file_age: could not stat %s\n",path);
			return 1; }
	//*age=now-stat_buf.st_mtime;
	*age=now-(stat_buf.st_mtime>T0?stat_buf.st_mtime:T0); //Do not count time before inception of algorithm
	return 0; }

/*int main(int argc, char ** argv){
	time_t age;
	if	(file_age(argv[1],&age))
		{	fputs("file_age main(): file_age() returned nonzero\n",stderr);
			return 1; }
	printf("%ld\n",age);
	return 0; } */

//IN GOD WE TRVST.


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#ifdef WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <unistd.h>
#endif
#include "utilities.h"


char does_it_exist(char *filename)
{
#ifdef WIN32
	return 0;
#else
	struct stat buf;

	/* the function stat returns 0 if the file exists*/

	if (0 == stat(filename, &buf)){
		return 1;
	}
	else return 0;
#endif
}

void gotosleep(int numseconds)
{
#ifdef WIN32
	Sleep(numseconds*1000);
#else
	sleep(numseconds);
#endif
}

void erasefile(char *filename)
{
	remove(filename);
}


double drawnormal_r(unsigned int *prseed)
{
	double U1, U2, drawn, pi;

	pi = 3.141592653589793;

	U1 = (rand_r(prseed)+1)/((double)((unsigned int)RAND_MAX+1)); /** we don't want a value of 0 otherwise drawn = +oo **/
	U2 = (rand_r(prseed)+1)/((double)((unsigned int)RAND_MAX+1));

	drawn = sqrt(-2*log(U1))*cos(2*pi*U2); 
	
	return drawn;
}

/**
 * Safe freeing
 * free an address and set it to NULL to prevent double freeing
 */
void UTLFree(void **paddress)
{
	void *address = *paddress;

	if (address == NULL) goto BACK;

	/**printf("freeing array at %p\n", address);**/
	free(address);
	address = NULL; /** prevents double freeing **/

	BACK:
	*paddress = address;
}


/** print vector **/
void UTLShowVector(int n, double *vector)
{
	int j;

	for (j = 0; j < n; j++){
		printf(" %g", vector[j]);
	}
	printf("\n");
}


char *UTLGetTimeStamp(void)
{
	time_t timestamp;

	timestamp = time(0);
	return (char *) ctime(&timestamp);
}



int UTLTicks_ms()
{
#ifdef WIN32
	return (int)GetTickCount();
#else
	struct tms tm;
	return (int)(times(&tm) * 10);
#endif
}



#ifdef WIN32

int rand_r (unsigned int *seed)
{
	unsigned int result;
	unsigned int next = *seed;

	next = next * 1103515245 + 12345;
	result = (next >> 16) & RAND_MAX; /** result = (next / 65536) % 32768; **/

	*seed = next;
	return (int)result;
}

#endif


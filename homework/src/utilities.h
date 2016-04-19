/*
 * utilities.h
 *
 *  Created on: Mar 16, 2016
 *      Author: jadm
 */

#ifndef UTILITIES_H
#define UTILITIES_H

#define NOMEMORY 100
#define FILEOPENFAIL 99

char does_it_exist(char *filename);
void gotosleep(int numseconds);
void erasefile(char *filename);
double drawnormal_r(unsigned int *prseed);

void UTLFree(void **paddress);
void UTLShowVector(int n, double *vector);
char *UTLGetTimeStamp(void);
int UTLTicks_ms();


#ifdef WIN32
int rand_r (unsigned int *seed);
#endif


#endif

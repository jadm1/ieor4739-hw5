/*
 * pfsimul.c
 *
 *  Created on: Mar 17, 2016
 *      Author: jadm
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include "utilities.h"
#include "pf.h"

/** load optimized portfolio allocation from file **/
int load_initial_positions(char* filename, double **px, int* pn, int **pindices, double nonzero_threshold);
/** load prices matrix from file **/
int load_prices(char* filename, double **pp, int n, int *indices, int *pt, int max_t);
/** compute the average of a double vector **/
double average(int n, double *v);


int main(int argc, char **argv) {
	/**
	 * utility variables
	 */
	int retcode = 0;
	int delta_t;
	int i, j;
	char *x_filename, *p_filename;
	char *pfv_filename, *pfr_filename, *pfvar_filename;
	FILE *pfv_f, *pfr_f, *pfvar_f;
	int ov, or, ovar;
	double s;

	/**
	 * Program variables
	 */
	int verbose;
	double B;
	int num_sim, num_workers, max_t;
	WorkerBag *wbag = NULL;
	pthread_t *pthread = NULL;
	pthread_mutex_t outputmutex;
	double *x = NULL;
	int *indices = NULL;
	double *p = NULL;
	double *delta = NULL;
	double *sigma = NULL;
	int reb_interval;
	GRBenv   *env = NULL;
	/**
	 * arrays for results
	 */
	double *pf_values = NULL;
	double *pf_returns = NULL;
	double *pf_vars = NULL;
	double avg_pf_value;
	double avg_pf_return;
	double avg_pf_var;

	int n; /** number of assets **/
	int t; /** number of periods **/
	Portfolio **ppf = NULL;


	/**
	 * Default parameter values
	 */
	verbose = 0;
	num_sim = 1;
	num_workers = 1;
	max_t = 10000;
	B = 1000000000.0;
	ov = 0;
	or = 0;
	ovar = 0;
	reb_interval = 90;


	/**
	 * Collect parameters from command line
	 */
	if(argc < 3) {
		printf("usage: %s <portfolio positions file> <prices history file> [-q simulations number] [-w workers] [-p max periods] [-v verbose] [-b initial value] [-rp rebalance interval] [ -op avg values output file] [ -or avg returns output file] [ -ov avg vars output file] \n", argv[0]);
		retcode = 1; goto BACK;
	}
	for(j = 3; j < argc; j++){
		if (0 == strcmp(argv[j], "-v")){
			j += 0;
			verbose = 1;
		}
		else if (0 == strcmp(argv[j],"-q")){
			j += 1;
			num_sim = atoi(argv[j]);
		}
		else if (0 == strcmp(argv[j],"-w")){
			j += 1;
			num_workers = atoi(argv[j]);
		}
		else if (0 == strcmp(argv[j],"-p")){
			j += 1;
			max_t = atoi(argv[j]);
		}
		else if (0 == strcmp(argv[j],"-b")){
			j += 1;
			B = atof(argv[j]);
		}
		else if (0 == strcmp(argv[j],"-rp")) {
			j += 1;
			reb_interval = atoi(argv[j]);
		}
		else if (0 == strcmp(argv[j],"-op")){
			j += 1;
			pfv_filename = argv[j];
			ov = 1;
		}
		else if (0 == strcmp(argv[j],"-ov")){
			j += 1;
			pfvar_filename = argv[j];
			ovar = 1;
		}
		else if (0 == strcmp(argv[j],"-or")){
			j += 1;
			pfr_filename = argv[j];
			or = 1;
		}
		else{
			printf("bad option %s\n", argv[j]); retcode = 1; goto BACK;
		}
	}

	if (num_workers > num_sim) {
		num_workers = num_sim;
		printf(" --> reset workers to %d\n", num_workers);
	}

	x_filename = argv[1];
	p_filename = argv[2];


	printf("loading positions from %s\n", x_filename);
	retcode = load_initial_positions(x_filename, &x, &n, &indices, 1e-10);
	if (retcode != 0) {
		printf("positions could not be loaded !\n"); goto BACK;
	}

	printf("Portfolio assets: %d\n", n);

	if (verbose) {
		printf("x:");
		UTLShowVector(n, x);
		s = 0;
		for (j = 0; j < n; j++)
			s += x[j];
		printf("sum of positions : %g %%\n", s*100.0);
	}

	printf("loading prices from %s\n", p_filename);
	retcode = load_prices(p_filename, &p, n, indices, &t, max_t);
	if (retcode != 0) {
		printf("prices could not be loaded !\n"); goto BACK;
	}

	printf("periods: %d\n", t);
	printf("rebalance every %d periods\n", reb_interval);

	if (verbose) {
		printf("prices loaded\n");
	}

	printf("computing vector of averages of changes...\n");
	retcode = compute_avg_changes(p, n, t, &delta);
	if (retcode != 0)
		goto BACK;
	if (verbose) {
		printf("delta:");
		UTLShowVector(n, delta);
	}

	printf("computing vector std's of changes...\n");
	retcode = compute_std_changes(p, n, t, delta, &sigma);
	if (retcode != 0)
		goto BACK;
	if (verbose) {
		printf("sigma:");
		UTLShowVector(n, sigma);
	}



	/** allocating memory for result arrays **/
	pf_values = (double*)calloc(num_sim, sizeof(double));
	pf_returns = (double*)calloc(num_sim, sizeof(double));
	pf_vars = (double*)calloc(num_sim, sizeof(double));
	if (pf_values == NULL || pf_returns == NULL || pf_vars == NULL) {
		retcode = NOMEMORY; goto BACK;
	}


	// load gurobi env
	retcode = GRBloadenv(&env, NULL);
	if (retcode != 0) {
		goto BACK;
	}
	GRBsetparam(env, "OutputFlag", "0");


	/**
	 * Creating portfolio array for every worker (no shared memory so we need to make numworkers copies of 1 portfolio)
	 */
	retcode = portfolio_create_array(num_workers, &ppf, n, t, x, p, delta, sigma, B, pf_values, pf_returns, pf_vars, reb_interval, env);
	if (retcode != 0)
		goto BACK;


	/** check initial portfolio value**/
	s = 0.0;
	for (i = 0; i < n; i++) {
		s += p[i*t + 0] * ppf[0]->q[i];
	}
	printf("Initial portfolio value: %g\n", s);


	/** prepare simple multithreading **/
	wbag = (WorkerBag*)calloc(num_workers, sizeof(WorkerBag));
	if (wbag == NULL) {
		retcode = NOMEMORY; goto BACK;
	}
	pthread = (pthread_t *)calloc(num_workers, sizeof(pthread_t));
	if (pthread == NULL) {
		printf("could not create thread array\n");
		retcode = NOMEMORY; goto BACK;
	}

	delta_t = UTLTicks_ms();
	pthread_mutex_init(&outputmutex, NULL);

	for(j = 0; j < num_workers; j++) {
		wbag[j].ID = j;
		wbag[j].num_sim = num_sim;
		wbag[j].pf = ppf[j];
		wbag[j].poutputmutex = &outputmutex;
		wbag[j].num_workers = num_workers;
		pthread_mutex_lock(&outputmutex);
		printf("Launching thread for worker %d\n", j);
		pthread_mutex_unlock(&outputmutex);
		pthread_create(&pthread[j], NULL, &pfworker, (void *)&wbag[j]);
	}

	pthread_mutex_lock(&outputmutex);
	printf("Waiting for threads...\n");
	pthread_mutex_unlock(&outputmutex);

	for(j = 0; j < num_workers; j++) {
		pthread_join(pthread[j], NULL);

		pthread_mutex_lock(&outputmutex);
		printf("Thread %d joined ...\n", j);
		pthread_mutex_unlock(&outputmutex);
	}

	pthread_mutex_destroy(&outputmutex);
	delta_t = UTLTicks_ms() - delta_t;

	printf("P&L simulations done in %.1f seconds\n", delta_t/1000.0);

	printf("Averaging over all simulations\n");
	avg_pf_value = average(num_sim, pf_values);
	avg_pf_return = average(num_sim, pf_returns);
	avg_pf_var = average(num_sim, pf_vars);
	printf("Average final value: %g\n", avg_pf_value);
	printf("Average daily return: %g %%\n", 100.0*avg_pf_return);
	printf("Average daily variance of return: %g\n", avg_pf_var);


	if (ov) {
		printf("saving values...\n");
		pfv_f = fopen(pfv_filename, "w");
		fprintf(pfv_f, "nsim: %d\n", num_sim);
		for (j = 0; j < num_sim; j++) {
			fprintf(pfv_f, "%g\n", pf_values[j]);
		}
		fclose(pfv_f);
	}
	if (or) {
		printf("saving returns...\n");
		pfr_f = fopen(pfr_filename, "w");
		fprintf(pfr_f, "nsim: %d\n", num_sim);
		for (j = 0; j < num_sim; j++) {
			fprintf(pfr_f, "%g\n", pf_returns[j]);
		}
		fclose(pfr_f);
	}
	if (ovar) {
		printf("saving vars...\n");
		pfvar_f = fopen(pfvar_filename, "w");
		fprintf(pfvar_f, "nsim: %d\n", num_sim);
		for (j = 0; j < num_sim; j++) {
			fprintf(pfvar_f, "%g\n", pf_vars[j]);
		}
		fclose(pfvar_f);
	}
	if (verbose) {
		printf("freeing memory ...\n");
	}
	BACK:

	portfolio_delete_array(num_workers, &ppf);


	// free gurobi env
	GRBfreeenv(env);

	UTLFree((void**)&pf_vars);
	UTLFree((void**)&pf_values);
	UTLFree((void**)&pf_returns);
	UTLFree((void**)&wbag);
	UTLFree((void**)&pthread);
	UTLFree((void**)&sigma);
	UTLFree((void**)&delta);
	UTLFree((void**)&p);
	UTLFree((void**)&indices);
	UTLFree((void**)&x);

	return retcode;
}


int load_initial_positions(char* filename, double **px, int* pn, int** pindices, double nonzero_threshold) {
	int retcode = 0;
	FILE *f;
	char b[100];
	int i, ii;

	int N; /** total number of assets **/
	int n; /** number of assets in the initial portfolio (whose position is non zero)**/
	double *x = NULL;
	int *indices = NULL;

	f = fopen(filename, "r");
	if (f == NULL) {
		retcode = FILEOPENFAIL; goto BACK;
	}

	fscanf(f, "%s", b);
	fscanf(f, "%s", b);
	N = atoi(b);
	x = (double*)calloc(N, sizeof(double));
	indices = (int*)calloc(N, sizeof(int));
	if (x == NULL || indices == NULL) {
		retcode = NOMEMORY; goto BACK;
	}
	for (i = 0; i < N; i++) {
		fscanf(f, "%s", b);
		x[i] = atof(b);
	}
	fclose(f);

	n = 0;
	for (i = 0; i < N; i++) {
		if (x[i] > nonzero_threshold) {
			indices[n] = i;
			n++;
		}
	}

	/** rearrange the quantities using the indices of nonzero quantities **/
	for (ii = 0; ii < n; ii++) {
		x[ii] = x[indices[ii]];
	}

	if (pindices != NULL)
		*pindices = indices;
	if (pn != NULL)
		*pn = n;
	if (px != NULL)
		*px = x;

	BACK:
	if (retcode != 0) {
		UTLFree((void**)&x);
		UTLFree((void**)&indices);
	}
	return retcode;
}


int load_prices(char* filename, double **pp, int n, int *indices, int *pt, int max_t) {
	int retcode = 0;
	FILE *f;
	char b[100];
	int i, ii, j;

	int N, T;
	int t;
	double *p = NULL;

	f = fopen(filename, "r");
	if (f == NULL) {
		retcode = FILEOPENFAIL; goto BACK;
	}

	fscanf(f, "%s", b);
	fscanf(f, "%s", b);
	N = atoi(b);
	fscanf(f, "%s", b);
	fscanf(f, "%s", b);
	T = atoi(b);
	if (T > max_t)
		t = max_t;
	else
		t = T;

	/** skip the dates line **/
	fscanf(f, "%s", b);
	for (j = 0; j < T; j++)
		fscanf(f, "%s", b);

	p = calloc(n*t, sizeof(double));
	if (p == NULL) {
		retcode = NOMEMORY; goto BACK;
	}
	ii = 0;
	for (i = 0; i < N; i++) {
		fscanf(f, "%s", b);/** jump over asset name **/
		fscanf(f, "%s", b);/** jump over Adj_close: **/
		for (j = 0; j < T; j++) {
			fscanf(f, "%s", b);
			if (ii<n && i == indices[ii] && j < t)
				p[ii*t + j] = atof(b);
		}
		if (i == indices[ii])
			ii++;
	}
	fclose(f);


	if (pt != NULL)
		*pt = t;
	if (pp != NULL)
		*pp = p;

	BACK:
	if (retcode != 0) {
		UTLFree((void**)&p);
	}
	return retcode;
}


double average(int n, double *v) {
	int i;
	double r;

	r = 0.0;
	for (i = 0; i < n; i++) {
		r += v[i];
	}

	r /= n;

	return r;
}


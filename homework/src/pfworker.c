#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include "utilities.h"
#include "pf.h"


int RBrebalance_basic(int n, double *xstar, double *p, double *q,
		GRBenv *env, char **grb_names, double* grb_x, double* grb_obj,
		double* grb_ub, double* grb_lb, double* grb_cval, int* grb_cind);



void* pfworker(void * arg) {
	WorkerBag *wbag = (WorkerBag*)arg;
	Portfolio *pf = wbag->pf;
	pthread_mutex_t *poutputmutex = wbag->poutputmutex;
	int ID = wbag->ID;
	int num_sim = wbag->num_sim;
	int num_workers = wbag->num_workers;
	int start, end, num_sim_local;
	int sim;
	unsigned int rseed = ID;

	/** spread the simulations evenly over worker threads **/
	start = ((ID * num_sim)/num_workers);
	end = (((ID+1) * num_sim)/num_workers) - 1;
	num_sim_local = end - start + 1;

	pthread_mutex_lock(poutputmutex);
	printf("Worker %d started. %d sims assigned: from %d to %d \n", ID, num_sim_local, start, end);
	pthread_mutex_unlock(poutputmutex);

	for (sim = start; sim <= end; sim++) {
		portfolio_simulation(sim, &rseed, poutputmutex, pf, ID);
		if ((num_sim_local < 10) || sim % (num_sim_local/10) == 0) {
			pthread_mutex_lock(poutputmutex);
			printf("W %d: simulation %d, portfolio value: %g, avg daily return: %g %%\n", ID, sim, pf->pf_values[sim], pf->pf_returns[sim]*100.0);
			pthread_mutex_unlock(poutputmutex);
		}
	}


	return NULL;
}



int portfolio_simulation(int sim, unsigned int *prseed, pthread_mutex_t *poutputmutex, Portfolio *pf, int threadID) {
	int retcode = 0;
	int n, t;
	double *p, *pT, *q, *delta, *sigma;
	int i, j;
	double pf_v, pf_v_old; /** portfolio value **/
	double pf_return;

	n = pf->n;
	t = pf->t;
	p = pf->p;
	pT = pf->pT;
	q = pf->q;
	delta = pf->delta;
	sigma = pf->sigma;

	/** reset initial q from xstar (because last simulation may have changed q) **/
	portfolio_compute_qty_from_alloc(n, t, pf->B, q, pf->xstar, p);

	pf_return = 0.0;
	for (j = 0; j < t; j++) {
		//if (j > 0 && (j%pf->reb_interval == 0)) {
		if (j == 0) {
			printf("rebalancing at t=%d\n", j);
			printf("q:");UTLShowVector(n, q);
			printf("p^t:");UTLShowVector(n, &pT[j*n]);
			printf("x*:");UTLShowVector(n, pf->xstar);
			retcode = RBrebalance_basic(n, pf->xstar, &pT[j*n], q, pf->env, pf->grb_names, pf->grb_x, pf->grb_obj, pf->grb_ub, pf->grb_lb, pf->grb_cval, pf->grb_cind);
			if (retcode != 0) {
				return retcode;
			}
			printf("q:");UTLShowVector(n, q);
		}

		pf_v = 0.0;
		for (i = 0; i < n; i++) {
			/**pf_v += p[i*t + j] * q[i];**/ /** no perturbations **/
			pf_v += (p[i*t + j] + (sigma[i]*drawnormal_r(prseed) + delta[i])) * q[i]; /** add current value of the i-th asset + the perturbation **/
		}
		if (j > 0) {
			pf_return += (pf_v - pf_v_old) / pf_v_old;
		}
		pf_v_old = pf_v;
	}

	pf_return /= (t - 1);

	/** save results in an external array **/
	pf->pf_values[sim] = pf_v;
	pf->pf_returns[sim] = pf_return;
	pf->pf_vars[sim] = 0.0;


	return retcode;
}







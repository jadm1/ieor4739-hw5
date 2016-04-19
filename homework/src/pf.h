#ifndef PF_H
#define PF_H

#include <pthread.h>
#include <gurobi_c.h>

typedef struct Portfolio {
	int n; /** number of assets (whose quantities are non 0) **/
	int t; /** number of periods **/
	double B; /** initial portfolio value **/
	double *p; /** Matrix of prices (unperturbed) (nxT) **/
	double *pT; /** Transposed matrix (Txn) (to access p^t easily) **/
	double *q; /** Array of inital fixed quantities **/
	double *delta; /** avg changes of prices **/
	double *sigma; /** std changes of prices **/
	double *pf_values; /** pointer to the output array of portfolio values**/
	double *pf_returns; /** pointer to the output array of portfolio returns**/
	double *pf_vars;
	int reb_interval;
	GRBenv *env;

	double *xstar; /** initial x**/
	int grb_nvar;
	char **grb_names;
	double *grb_x, *grb_obj, *grb_ub, *grb_lb, *grb_cval;
	int *grb_cind;
} Portfolio;

typedef struct WorkerBag {
	int ID; /** ID (for worker threads) **/
	int num_sim; /** number of simulations **/
	int num_workers; /** number of workers **/
	pthread_mutex_t *poutputmutex;
	Portfolio *pf; /** worker's portfolio non shared variables **/
} WorkerBag;


/** Functions from pfload.c **/
/** portfolio_create allocates memory and initializes a portfolio bag structure from the given parameters  **/
int portfolio_create(Portfolio **ppf, int n, int t, double *x, double *p, double *delta, double *sigma, double B, double *pf_values, double *pf_returns, double *pf_vars, int reb_interval, GRBenv* env);
/** portfolio_delete deallocates memory of a portfolio bag structure **/
void portfolio_delete(Portfolio **ppf);
/** portfolio_create_array creates an array of number pointers to Portfolio bags and initializes them with the given parameters **/
int portfolio_create_array(int number, Portfolio*** pppf, int n, int t, double *x, double *p, double *delta, double *sigma, double B, double *pf_values, double *pf_returns, double *pf_vars, int reb_interval, GRBenv* env);
/** portfolio_delete_array deletes an array of number pointers to Portfolio bags **/
void portfolio_delete_array(int number, Portfolio ***pppf);

/** compute q from x, B, p (at time 0)**/
void portfolio_compute_qty_from_alloc(int n, int t, double B, double *q, double *x, double *p);

/** This function computes and outputs in pdelta a vector containing
the average of the day-over-day changes in the asset prices  **/
int compute_avg_changes(double *p, int n, int t, double **pdelta);
/** This function computes and outputs in pdelta a psigma containing
the std of the day-over-day changes in the asset prices 
delta must have been constructed with compute_avg_changes before calling this function
 **/
int compute_std_changes(double *p, int n, int t, double *delta, double **psigma);

/** Functions from pfworker.c **/
/** pfworker is the worker thread main function that assigns itselfs a batch of simulations to run 
and it prints some debug data while running
 **/
void* pfworker(void * arg);
/** 
portfolio_simulation runs a single P&L analysis with daily random perturbations to the prices
 **/
int portfolio_simulation(int sim, unsigned int *prseed, pthread_mutex_t *poutputmutex, Portfolio *pf, int threadID);

#endif

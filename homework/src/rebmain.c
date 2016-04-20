
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gurobi_c.h>
#include "utilities.h"

int readdata(char *filename, int *pnumassets, 
		double **ptargetx, double **pprices, double **pshares);
int RBengine(int numassets, double *targetx, double *prices, double *shares);
int RBrebalance_basic(int n, double *xstar, double *p, double *q,
		GRBenv *env, char **grb_names, double* grb_x, double* grb_obj,
		double* grb_ub, double* grb_lb, double* grb_cval, int* grb_cind);
int RBrebalance_approx(int n, double *xstar, double *p, double *q, double* eps, double* pcost,
		GRBenv *env, char **grb_names, double* grb_x, double* grb_obj,
		double* grb_ub, double* grb_lb, double* grb_cval, int* grb_cind);

/**
 * Test rebalancing
 */

int main(int argc, char **argv)
{
	int retcode = 0;
	int n;
	double *xstar = NULL, *p = NULL, *q = NULL;



	if( argc != 2) {
		printf("usage: rebalance filename\n"); retcode = 1; goto BACK;
	}

	retcode = readdata(argv[1], &n, &xstar,
			&p, &q);
	if(retcode) goto BACK;

	printf("number of assets: %d\n", n);


	printf("q: "); UTLShowVector(n, q);
	retcode = RBengine(n, xstar, p, q);
	if (retcode != 0) {
		printf("error rebalance failed !\n");
		goto BACK;
	}
	printf("q: "); UTLShowVector(n, q);


	BACK:
	printf("\nexiting with retcode %d\n", retcode);
	return retcode;
}

int readdata(char *filename, int *pnumassets, double **ptargetx, double **pprices, double **pshares)
{
	int retcode = 0, j;
	FILE *input;
	char buffer[100];
	int numassets = 0;
	double *targetx = NULL, *prices = NULL, *shares = NULL;

	input = fopen(filename, "r");
	if(!input){
		printf("cannot read file %s\n", filename); retcode = 1; goto BACK;
	}
	fscanf(input,"%s", buffer);
	numassets = atoi(buffer);
	//printf("  %d assets\n", numassets);

	targetx = (double *)calloc(3*numassets, sizeof(double));
	if(targetx == NULL){
		retcode = 1; goto BACK;
	}
	prices = targetx + numassets;
	shares = prices + numassets;

	for(j = 0; j < numassets; j++) {
		fscanf(input,"%s", buffer); /** should be the value j **/
		fscanf(input,"%s", buffer); targetx[j] = atof(buffer);
		fscanf(input,"%s", buffer); prices[j] = atof(buffer);
		fscanf(input,"%s", buffer); shares[j] = atof(buffer);
	}

	*pnumassets = numassets;
	*ptargetx = targetx;
	*pprices = prices;
	*pshares = shares;
	BACK:
	return retcode;
}


int RBengine(int n, double *xstar, double *p, double *q)
{
	int retcode = 0;
	int j;
	double value;
	double cost;

	GRBenv   *env = NULL;


	double *eps;
	int grb_nvar;
	char **grb_names;
	double *grb_x, *grb_obj, *grb_ub, *grb_lb, *grb_cval;
	int *grb_cind;





	value = 0;
	for(j = 0; j < n; j++)
		value += p[j]*q[j];
	printf("Portfolio value: %g\n", value);
	for(j = 0; j < n; j++)
		printf("asset %4d at weight %.6f target %.6f\n", j, p[j]*q[j]/value, xstar[j]);





	// load gurobi env
	retcode = GRBloadenv(&env, NULL);
	if (retcode) goto BACK;
	GRBsetparam(env, "OutputFlag", "0");



	/** Initialize rebalancing data **/
	grb_nvar = 2*n; /* total number of variables **/
	grb_names = (char **) calloc(grb_nvar, sizeof(char *));
	grb_x = (double *)calloc(4*grb_nvar, sizeof(double));
	if(!grb_names || !grb_x){
		printf("no memory\n"); retcode = 1; goto BACK;
	}

	grb_obj = grb_x + grb_nvar;
	grb_ub = grb_obj + grb_nvar;
	grb_lb = grb_ub + grb_nvar;

	for(j = 0; j < grb_nvar; j++) {
		grb_names[j] = (char *)calloc(10, sizeof(char));
		if(grb_names[j] == NULL){
			retcode = 1; goto BACK;
		}
		if(j < n)
			sprintf(grb_names[j],"d%dplus", j);
		else
			sprintf(grb_names[j],"d%dminus", j - n);
	}

	grb_cind = (int *) calloc(grb_nvar, sizeof(int));
	grb_cval = (double *) calloc(grb_nvar, sizeof(double));
	if(!grb_cind || !grb_cval){
		printf("no memory\n"); retcode = 1; goto BACK;
	}

	eps = (double*)calloc(n, sizeof(double));
	for (j = 0; j < n; j++) {
		eps[j] = 0.00;
	}



	printf("Rebalancing...\n");
	//retcode = RBrebalance_basic(n, xstar, p, q, env, grb_names, grb_x, grb_obj, grb_ub, grb_lb, grb_cval, grb_cind);
	retcode = RBrebalance_approx(n, xstar, p, q, eps, &cost, env, grb_names, grb_x, grb_obj, grb_ub, grb_lb, grb_cval, grb_cind);
	if (retcode != 0) {
		goto BACK;
	}
	printf("Done. Obj Val : %g\n", cost);



	value = 0;
	for(j = 0; j < n; j++)
		value += p[j]*q[j];
	printf("Rebalanced Portfolio value: %g\n", value);
	for(j = 0; j < n; j++)
		printf("asset %4d at weight %.6f\n", j, p[j]*q[j]/value);





	/** free rebalancing data **/
	for (j = 0; j < grb_nvar; j++)
		free(grb_names[j]);
	free(grb_names);
	free(grb_x);
	free(grb_cind);
	free(grb_cval);



	// free gurobi env
	GRBfreeenv(env);

	BACK:
	return retcode;
}




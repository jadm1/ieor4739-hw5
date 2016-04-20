
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gurobi_c.h>
#include "utilities.h"

int RBrebalance_basic(int n, double *xstar, double *p, double *q,
		GRBenv *env, char **grb_names, double* grb_x, double* grb_obj,
		double* grb_ub, double* grb_lb, double* grb_cval, int* grb_cind) {
	int retcode = 0;
	int i, j;
	int grb_nvar;
	GRBmodel *model = NULL;
	char bigname[100];
	double rhs;
	int status = 0;
	double sum;

	grb_nvar = 2*n; /* total number of variables **/


	/** make sure xstar add up to exactly 1 otherwise we get numerical problems
	 * since all the equality constraints in the basic case will not be satisfied
	 * except for very small values of q's.
	 * This is unstable and we might get very small values of q
	 * **/
	sum = 0;
	for (i=0;i<n;i++)
		sum += xstar[i];
	for (i=0;i<n;i++)
		xstar[i] /= sum;




	/* Create model for rebalancing*/
	/* we can afford to reallocate that model every time since we do the rebalancing typically only every 3 months*/
	retcode = GRBnewmodel(env, &model, "rebalance", grb_nvar,
			NULL, NULL, NULL, NULL, NULL);
	if (retcode) goto BACK;



	for (j = 0; j < n; j++) {
		grb_ub[j] = 1e10; /** overkill **/
		grb_lb[j] = 0.0;
		grb_obj[j] = p[j];
		grb_ub[j+n] = q[j];
		grb_lb[j+n] = 0.0;
		grb_obj[j+n] = p[j];
	}

	/* initialize variables */
	for(j = 0; j < grb_nvar; j++){
		retcode = GRBsetstrattrelement(model, "VarName", j, grb_names[j]);
		if (retcode) goto BACK;

		retcode = GRBsetcharattrelement(model, "VType", j, GRB_CONTINUOUS);
		if (retcode) goto BACK;

		retcode = GRBsetdblattrelement(model, "Obj", j, grb_obj[j]);
		if (retcode) goto BACK;

		retcode = GRBsetdblattrelement(model, "LB", j, grb_lb[j]);
		if (retcode) goto BACK;

		retcode = GRBsetdblattrelement(model, "UB", j, grb_ub[j]);
		if (retcode) goto BACK;

	}

	//retcode = GRBupdatemodel(model); // unnecessary since it is updated later
	//if (retcode) goto BACK;

	/** add constraints **/

	/** constraint 1a **/
	for(j = 0; j < n; j++){
		rhs = 0;
		sprintf(bigname,"1a_%d",j);
		for (i = 0; i < n; i++){
			if(i != j) {
				grb_cval[i] = -xstar[j]*p[i];
				grb_cval[i + n] = xstar[j]*p[i];
				rhs += xstar[j]*p[i]*q[i];
			}
			else {
				grb_cval[j] = p[j]*(1 - xstar[j]);
				grb_cval[j + n] = -p[j]*(1 - xstar[j]);
				rhs += xstar[j]*p[i]*q[i] - p[j]*q[j];
			}
			grb_cind[i] = i;
			grb_cind[i + n] = i + n;
		}
		retcode = GRBaddconstr(model, 2*n, grb_cind, grb_cval, GRB_EQUAL, rhs, bigname);
		if (retcode) goto BACK;
	}

	/** constraint 1b (do not sell more shares than we own) not needed because already in the bounds**/


	retcode = GRBsetintattr(model, GRB_INT_ATTR_MODELSENSE, GRB_MINIMIZE);
	if (retcode) goto BACK;

	retcode = GRBupdatemodel(model);
	if (retcode) goto BACK;


	/** optional: write the problem **/
	//retcode = GRBwrite(model, "rebalance.lp");
	//if (retcode) goto BACK;

	retcode = GRBoptimize(model);
	if (retcode) goto BACK;

	retcode = GRBgetintattr(model, GRB_INT_ATTR_STATUS, &status);
	if (retcode) goto BACK;

	if (status == GRB_INFEASIBLE || status == GRB_INF_OR_UNBD || status == GRB_UNBOUNDED || status == GRB_INTERRUPTED) {
		printf("rebalancing failed, keeping same share values\n");
	}
	else {

		/** get solution **/
		retcode = GRBgetdblattrarray(model,
				GRB_DBL_ATTR_X, 0, grb_nvar,
				grb_x);
		if(retcode) goto BACK;


		/** update share quantities  **/
		for (j = 0; j < n; j++) {
			q[j] += grb_x[j] - grb_x[j+n];
		}

		//printf("d+,d-: ");UTLShowVector(2*n, grb_x);

	}

	GRBfreemodel(model);

	//printf("status: %d\n", status);
	BACK:
	return 0;
}


int RBrebalance_approx(int n, double *xstar, double *p, double *q, double* eps, double* pcost,
		GRBenv *env, char **grb_names, double* grb_x, double* grb_obj,
		double* grb_ub, double* grb_lb, double* grb_cval, int* grb_cind) {
	int retcode = 0;
	int i, j;
	int grb_nvar;
	GRBmodel *model = NULL;
	char bigname[100];
	double rhs;
	int status = 0;
	double sum;

	grb_nvar = 2*n; /* total number of variables **/


	/** make sure xstar add up to exactly 1
	 * **/
	sum = 0;
	for (i=0;i<n;i++)
		sum += xstar[i];
	for (i=0;i<n;i++)
		xstar[i] /= sum;




	/* Create model for rebalancing*/
	/* we can afford to reallocate that model every time since we do the rebalancing typically only every 3 months*/
	retcode = GRBnewmodel(env, &model, "rebalance", grb_nvar,
			NULL, NULL, NULL, NULL, NULL);
	if (retcode) goto BACK;



	for (j = 0; j < n; j++) {
		grb_ub[j] = 1e10; /** overkill **/
		grb_lb[j] = 0.0;
		grb_obj[j] = p[j];
		grb_ub[j+n] = q[j];
		grb_lb[j+n] = 0.0;
		grb_obj[j+n] = p[j];
	}

	/* initialize variables */
	for(j = 0; j < grb_nvar; j++){
		retcode = GRBsetstrattrelement(model, "VarName", j, grb_names[j]);
		if (retcode) goto BACK;

		retcode = GRBsetcharattrelement(model, "VType", j, GRB_CONTINUOUS);
		if (retcode) goto BACK;

		retcode = GRBsetdblattrelement(model, "Obj", j, grb_obj[j]);
		if (retcode) goto BACK;

		retcode = GRBsetdblattrelement(model, "LB", j, grb_lb[j]);
		if (retcode) goto BACK;

		retcode = GRBsetdblattrelement(model, "UB", j, grb_ub[j]);
		if (retcode) goto BACK;

	}

	//retcode = GRBupdatemodel(model); // unnecessary since it is updated later
	//if (retcode) goto BACK;

	/** add constraints **/

	/** constraint 4a **/
	for(j = 0; j < n; j++) {
		rhs = 0;
		sprintf(bigname,"4a_%d",j);
		for (i = 0; i < n; i++){
			if(i != j) {
				grb_cval[i] = (xstar[j] + eps[j])*p[i];
				grb_cval[i + n] = -grb_cval[i];
				rhs += -(xstar[j]+eps[j])*p[i]*q[i];
			}
			else {
				grb_cval[j] = p[j]*((xstar[j] + eps[j]) - 1);
				grb_cval[j + n] = -grb_cval[j];
				rhs += (1-(xstar[j]+eps[j]))*p[j]*q[j];
			}
			grb_cind[i] = i;
			grb_cind[i + n] = i + n;
		}
		retcode = GRBaddconstr(model, 2*n, grb_cind, grb_cval, GRB_GREATER_EQUAL, rhs, bigname);
		if (retcode) goto BACK;
	}

	/** constraint 4b **/
	for(j = 0; j < n; j++) {
		rhs = 0;
		sprintf(bigname,"4b_%d",j);
		for (i = 0; i < n; i++){
			if(i != j) {
				grb_cval[i] = (-xstar[j] + eps[j])*p[i];
				grb_cval[i + n] = -grb_cval[i];
				rhs += -(-xstar[j]+eps[j])*p[i]*q[i];
			}
			else {
				grb_cval[j] = p[j]*((-xstar[j] + eps[j]) + 1);
				grb_cval[j + n] = -grb_cval[j];
				rhs += (-1-(-xstar[j]+eps[j]))*p[j]*q[j];
			}
			grb_cind[i] = i;
			grb_cind[i + n] = i + n;
		}
		retcode = GRBaddconstr(model, 2*n, grb_cind, grb_cval, GRB_GREATER_EQUAL, rhs, bigname);
		if (retcode) goto BACK;
	}


	/** constraint 4c (do not sell more shares than we own) not needed because already in the bounds**/


	retcode = GRBsetintattr(model, GRB_INT_ATTR_MODELSENSE, GRB_MINIMIZE);
	if (retcode) goto BACK;

	retcode = GRBupdatemodel(model);
	if (retcode) goto BACK;


	/** optional: write the problem **/
	//retcode = GRBwrite(model, "rebalance.lp");
	//if (retcode) goto BACK;

	retcode = GRBoptimize(model);
	if (retcode) goto BACK;

	retcode = GRBgetintattr(model, GRB_INT_ATTR_STATUS, &status);
	if (retcode) goto BACK;

	if (status == GRB_INFEASIBLE || status == GRB_INF_OR_UNBD || status == GRB_UNBOUNDED || status == GRB_INTERRUPTED) {
		printf("rebalancing failed, keeping same share values\n");
	}
	else {
		/** get cost **/

		if (pcost != NULL)
			GRBgetdblattr(model, GRB_DBL_ATTR_OBJVAL, pcost);

		/** get solution **/
		retcode = GRBgetdblattrarray(model,
				GRB_DBL_ATTR_X, 0, grb_nvar,
				grb_x);
		if(retcode) goto BACK;


		/** update share quantities  **/
		for (j = 0; j < n; j++) {
			q[j] += grb_x[j] - grb_x[j+n];
		}

		//printf("d+,d-: ");UTLShowVector(2*n, grb_x);

	}

	GRBfreemodel(model);

	//printf("status: %d\n", status);
	BACK:
	return 0;
}




#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <gurobi_c.h>


int robustengine(int numassets, int numfactors, 
		double *ub, double *lb, double *mu, double *sigma2,
		double *V, double *F, double lambda, double gamma, double Gamma, double* x,
		const char* grblogfilename, const char* lpoutputfilename)
{
	int retcode = 0;
	GRBenv   *env = NULL;
	GRBmodel *model = NULL;
	int n, i, j, k;
	int *qrow, *qcol, Nq;
	double *qval;
	int *cind;
	double rhs;
	char sense;
	double *cval;
	int numnonz, count;
	char **names, bigname[100];

	printf("running solver engine\n");

	n = numassets + numfactors + 1 + 3*numassets + 1;

	/* rhe first + 1 is for "rho", then we have the p, q, s variables, and then
     t */

	retcode = GRBloadenv(&env, grblogfilename);
	if (retcode) goto BACK;

	/* Create initial model */
	retcode = GRBnewmodel(env, &model, "factors", n,
			NULL, NULL, NULL, NULL, NULL);
	if (retcode) goto BACK;

	ub = (double *) realloc(ub, n*sizeof(double));

	lb = (double *) realloc(lb, n*sizeof(double));

	if(!ub || !lb ){
		printf("cannot realloc\n"); retcode = 1; goto BACK;
	}

	names = (char **) calloc(n, sizeof(char *));

	/** next we create the remaining attributes for the n columns **/
	// x has been allocated outside
	//x     = (double *) calloc (n, sizeof(double));

	for(j = 0; j < numassets; j++){
		names[j] = (char *)calloc(3, sizeof(char));
		if(names[j] == NULL){
			retcode = 1; goto BACK;
		}
		sprintf(names[j],"x%d", j);
	}
	for(j = numassets; j < numassets + numfactors; j++){
		names[j] = (char *)calloc(3, sizeof(char));
		if(names[j] == NULL){
			retcode = 1; goto BACK;
		}
		sprintf(names[j],"F%d", j - numassets);
	}

	names[numassets + numfactors] = (char *)calloc(4, sizeof(char));
	sprintf(names[numassets + numfactors], "rho");
	ub[numassets + numfactors] = 10000; /* crude */

	count = numassets + numfactors + 1;  /* variable count */
	for(j = 0; j < numassets; j++){
		names[count] = (char *)calloc(3, sizeof(char));
		sprintf(names[count],"p%d", j); ub[count] = 1000;
		names[count + 1] = (char *)calloc(3, sizeof(char));
		sprintf(names[count+1],"q%d", j); ub[count + 1] = 10000;
		names[count + 2] = (char *)calloc(3, sizeof(char));
		sprintf(names[count+2],"s%d", j); ub[count + 2] = 10000;
		/*should check that I got it */
		count += 3;
	}

	names[n-1] = (char *)calloc(2, sizeof(char));
	sprintf(names[n-1], "t");
	ub[n-1] = 100000;


	/* initialize variables */
	for(j = 0; j < n; j++){
		retcode = GRBsetstrattrelement(model, "VarName", j, names[j]);
		if (retcode) goto BACK;

		if (j == numassets + numfactors){

			/** so this is the rho variable **/
			retcode = GRBsetdblattrelement(model, "Obj", j, -1);
			if (retcode) goto BACK;
		}

		retcode = GRBsetdblattrelement(model, "LB", j, lb[j]);
		if (retcode) goto BACK;

		retcode = GRBsetdblattrelement(model, "UB", j, ub[j]);
		if (retcode) goto BACK;


	}

	/** next, the quadratic -- there are numassets + numfactors*numfactors nonzeroes:
      numassets residual variances plus the numfactors x numfactors
      factor covariance matrix**/

	Nq = numassets + numfactors*numfactors;
	qrow = (int *) calloc(Nq, sizeof(int));  /** row indices **/
	qcol = (int *) calloc(Nq, sizeof(int));  /** column indices **/
	qval = (double *) calloc(Nq, sizeof(double));  /** values **/

	if( ( qrow == NULL) || ( qcol == NULL) || (qval == NULL) ){
		printf("could not create quadratic\n");
		retcode = 1; goto BACK;
	}

	for (j = 0; j < numassets; j++){
		qval[j] = lambda*sigma2[j];
		qrow[j] = qcol[j] = j;
	}
	for (i = 0; i < numfactors; i++){
		for (j = 0; j < numfactors; j++){
			k = i*numfactors + j;
			qval[k + numassets] = lambda*F[k];
			qrow[k + numassets] = numassets + i;
			qcol[k + numassets] = numassets + j;
		}
	}
	retcode = GRBaddqpterms(model, Nq, qrow, qcol, qval);
	if (retcode) goto BACK;

	/** now we will add one constraint at a time **/
	/** we need to have a couple of auxiliary arrays **/

	cind = (int *)calloc(n, sizeof(int));  /** n is over the top since no constraint is totally dense;		     but it's not too bad here **/
	cval= (double *)calloc(n, sizeof(double));
	if(!cval){
		printf("cannot allocate cval\n"); retcode = 2; goto BACK;
	}
	for(i = 0; i < numfactors; i++){
		for(j = 0; j < numassets; j++){
			cval[j] = V[i*numassets + j];
			cind[j] = j;
		}
		cind[numassets] = /* j */ numassets + i;
		cval[numassets] = -1;
		numnonz = numassets + 1;
		rhs = 0;
		sense = GRB_EQUAL;

		sprintf(bigname,"factor%d",i);
		retcode = GRBaddconstr(model, numnonz, cind, cval, sense, rhs, bigname);
		if (retcode) goto BACK;

	}

	/** sum of x variables = 1 **/


	for (j = 0; j < numassets; j++){
		cval[j] = 1.0;  cind[j] = j;
	}

	numnonz = numassets;
	rhs = 1.0;
	sense = GRB_EQUAL;

	/* let's reuse some space */
	sprintf(bigname, "sum");

	retcode = GRBaddconstr(model, numnonz, cind, cval, sense, rhs, bigname);
	if (retcode) goto BACK;

	/** constraint (10e) from writeup **/

	retcode = GRBupdatemodel(model);

	cval[0] = 1.0; cind[0] = numassets + numfactors; /** rho variable **/
	count = 1;
	for (j = 0; j < numassets; j++){
		cval[count] = mu[j];
		cind[count] = numassets + numfactors + 1 + 3*j; /** p_j **/
		count++;
		cval[count] = -mu[j];
		cind[count] = numassets + numfactors + 1 + 3*j + 1; /** q_j **/
		count++;
		cval[count] = gamma;
		cind[count] = numassets + numfactors + 1 + 3*j + 2; /** s_j **/
		count++;
	}
	cval[count] = Gamma;  cind[count] = n-1; /** t **/  ++count;

	/*  this might help*/
	for(j = 0; j < count; j++){
		printf("%d %d %s %g\n", j, cind[j], names[cind[j]], cval[j]);
	}
	printf("count is %d\n", count);


	numnonz = count;
	rhs = 0.0;
	sense = GRB_LESS_EQUAL;

	/* let's reuse some space */
	sprintf(bigname, "10e");

	retcode = GRBaddconstr(model, numnonz, cind, cval, sense, rhs, bigname);
	if (retcode) goto BACK;

	/** now constraints 10f and 10g **/

	for (j = 0; j < numassets; j++){
		cind[0] = numassets + numfactors + 1 + 3*j; /** p_j **/
		cval[0] = 1;
		cind[1] = numassets + numfactors + 1 + 3*j + 1; /** q_j **/
		cval[1] = 1;
		cind[2] = numassets + numfactors + 1 + 3*j + 2; /** q_j **/
		cval[2] = -1;
		cind[3] = n-1;
		cval[3] = -1;

		numnonz = 4;
		rhs = 0.0;
		sense = GRB_LESS_EQUAL;

		/* let's reuse some space */
		sprintf(bigname, "10f_%d",j);

		retcode = GRBaddconstr(model, numnonz, cind, cval, sense, rhs, bigname);
		if (retcode) goto BACK;


		cind[0] = numassets + numfactors + 1 + 3*j; /** p_j **/
		cval[0] = -1;
		cind[1] = numassets + numfactors + 1 + 3*j + 1; /** q_j **/
		cval[1] = 1;
		cind[2] = j;
		cval[2] = -1;

		numnonz = 3;
		rhs = 0.0;
		sense = GRB_LESS_EQUAL;

		/* let's reuse some space */
		sprintf(bigname, "10g_%d",j);

		retcode = GRBaddconstr(model, numnonz, cind, cval, sense, rhs, bigname);
		if (retcode) goto BACK;


	}


	retcode = GRBupdatemodel(model);
	if (retcode) goto BACK;

	/** optional: write the problem **/

	if (lpoutputfilename != NULL) {
		retcode = GRBwrite(model, lpoutputfilename);
		if (retcode) goto BACK;
	}


	retcode = GRBoptimize(model);
	if (retcode) goto BACK;


	/** get solution **/
	retcode = GRBgetdblattrarray(model,
			GRB_DBL_ATTR_X, 0, n,
			x);
	if(retcode) goto BACK;


	GRBfreemodel(model);
	GRBfreeenv(env);



	BACK:
	printf("engine exits with code %d\n", retcode);
	return retcode;
}

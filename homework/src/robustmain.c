
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int robustengine(int numassets, int numfactors, 
		double *ub, double *lb, double *mu, double *sigma2,
		double *V, double *F, double lambda, double gamma, double Gamma, double* x,
		const char* grblogfilename, const char* lpoutputfilename);

int readdata(char *filename, int *pnumassets, int *pnumfactors,
		double **pub, double **plb, double **pmu, double **psigma2, double **pV,
		double **pF);

int main(int argc, char **argv)
{
	int retcode = 0;
	int numassets, numfactors;
	double *ub = NULL, *lb = NULL, *mu = NULL, *sigma2 = NULL, *V = NULL,
			*F = NULL, lambda, gamma, Gamma;
	int n;
	double *x;
	double expectedreturnval;
	int j;
	FILE* fx;
	int fx_n; // number of assets with non-zero positions

	if( argc < 5) {
		printf("usage: %s <input file> <lambda> <gamma> <Gamma> [output x file]n", argv[0]); retcode = 1; goto BACK;
	}

	lambda = atof(argv[2]); gamma = atof(argv[3]); Gamma = atof(argv[4]);

	retcode = readdata(argv[1], &numassets, &numfactors,
			&ub, &lb, &mu, &sigma2, &V, &F);
	if(retcode) goto BACK;

	printf("numassets: %d; numfactors: %d; lambda: %g; gamma: %g; Gamma: %g\n", numassets, numfactors, lambda, gamma, Gamma);


	n = numassets + numfactors + 1 + 3*numassets + 1;
	x = (double *)calloc(n, sizeof(double));
	retcode = robustengine(numassets, numfactors, ub, lb, mu, sigma2, V, F, lambda, gamma, Gamma, x, NULL, "robust.lp");

	/** now let's see the values **/

	fx_n = 0;
	expectedreturnval = 0;
	for(j = 0; j < numassets; j++){
		if( x[j] > 1.0e-09){
			fx_n++;
			printf("x%d = %g\n", j, x[j]);
			expectedreturnval += x[j]*mu[j];
		}
	}

	printf("\n*** expected portfolio return: %g; robust: %g\n", expectedreturnval, x[numassets + numfactors]);

	if (argc == 6) {
		fx = fopen(argv[5], "w");
		/*fprintf(fx, "n %d\n", fx_n);
	for (j = 0; j < numassets; j++) {
		if( x[j] > 1.0e-09){
			fprintf(fx, "%g\n", x[j]);
		}
	}*/ // If we want to save only the positions that matter
		fprintf(fx, "n %d\n", numassets);
		for (j = 0; j < numassets; j++) {
			fprintf(fx, "%g\n", x[j]);
		} // save all the positions
		fclose(fx);
	}

	free(x);

	BACK:
	printf("\nexiting with retcode %d\n", retcode);
	return retcode;
}

int readdata(char *filename, int *pnumassets, int *pnumfactors,
		double **pub, double **plb, double **pmu, double **psigma2,
		double **pV, double **pF)
{
	int retcode = 0;
	FILE *input;
	char buffer[100];
	int numassets = 0,  numfactors = 0, i, j, numcols;
	double *ub = NULL, *lb = NULL, *mu = NULL, *sigma2 = NULL, *V = NULL, *F = NULL;

	input = fopen(filename, "r");
	if(!input){
		printf("cannot read file %s\n", filename); retcode = 1; goto BACK;
	}
	fscanf(input,"%s", buffer);   printf("-> %s\n", buffer);
	fscanf(input,"%s", buffer);
	numassets = atoi(buffer);  printf("  %d assets\n", numassets);

	fscanf(input,"%s", buffer);   printf("-> %s\n", buffer);
	fscanf(input,"%s", buffer);
	numfactors = atoi(buffer); printf("  %d factors\n", numfactors);


	numcols = 2*numassets + numfactors;

	mu = NULL;
	mu = (double *)calloc(numassets, sizeof(double));
	if(mu == NULL){
		retcode = 1; goto BACK;
	}
	ub = (double *)calloc(numcols, sizeof(double));
	lb = (double *)calloc(numcols, sizeof(double));
	sigma2 = (double *)calloc(numassets, sizeof(double));
	V = (double *)calloc(numfactors*numassets + numfactors*numfactors, sizeof(double));
	if(V == NULL){
		retcode = 1; goto BACK;
	}

	F = V + numfactors*numassets;


	/* read returns */
	fscanf(input,"%s", buffer);
	printf("-> %s\n", buffer);
	for(j = 0; j < numassets; j++){
		fscanf(input,"%s", buffer); mu[j] = atof(buffer);
	}

	/* read ub */
	fscanf(input,"%s", buffer);
	printf("-> %s\n", buffer);
	for(j = 0; j < numassets; j++){
		fscanf(input,"%s", buffer); ub[j] = atof(buffer); lb[j] = 0;
	}

	/* read sigma2 */
	fscanf(input,"%s", buffer);
	printf("-> %s\n", buffer);
	for(j = 0; j < numassets; j++){
		fscanf(input,"%s", buffer); sigma2[j] = atof(buffer);
	}

	/* read V (from Vtranspose) */
	fscanf(input,"%s", buffer);
	printf("-> %s\n", buffer);
	for(j = 0; j < numassets; j++) {
		for(i = 0; i < numfactors; i++){
			fscanf(input,"%s", buffer);
			V[i*numassets + j] = atof(buffer);
		}
	}

	/* read F */
	fscanf(input,"%s", buffer);
	printf("-> %s\n", buffer);
	for(j = 0; j < numfactors; j++)for(i = 0; i < numfactors; i++){
		fscanf(input,"%s", buffer);
		F[i*numfactors + j] = atof(buffer);
	}

	fscanf(input,"%s", buffer);
	if(0 != strcmp(buffer, "END")){
		printf(" --> no END\n"); retcode = 1; goto BACK;
	}

	if(input) fclose(input);


	/**next, the upper bounds on the factor variables **/
	for(j = 0; j < numfactors; j++){
		ub[j + numassets] = 100000.0;
		lb[j + numassets] = -100000.00;
	}

	*pnumassets = numassets;
	*pnumfactors = numfactors;
	*pub = ub; *plb = lb; *pmu = mu; *psigma2 = sigma2; *pV = V; *pF = F;
	BACK:
	return retcode;
}

int readstructure(char *filename, int *pmax_names, double **ptheta, int numassets,
		char **pparamsfilename)
{
	int retcode = 0;
	FILE *input;
	char buffer[100];
	int max_names = numassets, j, k;
	double *theta = NULL;

	input = fopen(filename, "r");
	if(!input){
		printf("cannot read file %s\n", filename); retcode = 1; goto BACK;
	}

	printf("reading structure file %s\n", filename);
	for(;;){
		fscanf(input,"%s", buffer);
		if(0 == strcmp(buffer, "max_names")){
			fscanf(input,"%s", buffer);
			max_names = atoi(buffer);
			max_names = (max_names < numassets) ? max_names : numassets;
			printf("max no. of names in portfolio: %d\n", max_names);
			*pmax_names = max_names;
		}
		else if(0 == strcmp(buffer, "theta")){
			if(( theta = (double *)calloc(numassets, sizeof(double))) == NULL){
				printf("cannot allocate theta\n"); retcode = 1; goto BACK;
			}
			for(j = 0; j < numassets; j++){
				fscanf(input,"%s", buffer);
				k = atoi(buffer);
				fscanf(input,"%s", buffer);
				theta[k] = atof(buffer);
				printf("theta[%d] = %g\n", k, theta[k]);
			}
			*ptheta = theta;

		}
		else if(0 == strcmp(buffer, "gurobiparamsfile")){
			fscanf(input,"%s", buffer);
			*pparamsfilename = strdup(buffer);
		}
		else if(0 == strcmp(buffer, "END")){
			break;
		}
		else{
			printf("illegal buffer %s in structure file\n", buffer); retcode = 1;
			break;
		}
	}
	fclose(input);
	BACK:
	return retcode;
}


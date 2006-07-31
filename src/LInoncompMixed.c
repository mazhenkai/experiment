#include <stddef.h>
#include <stdio.h>      
#include <string.h>
#include <math.h>
#include <Rmath.h>
#include <R.h>
#include "vector.h"
#include "subroutines.h"
#include "rand.h"
#include "models.h"


/* 
   Bayesian binary probit mixed effects model for clustered randomized
   experiments with noncompliance; Latent Ignorability assumption for
   subsequent missing outcomes 
*/

void LIbprobitMixed(int *Y,         /* binary outcome variable */ 
		    int *R,         /* missingness indicator for Y */
		    int *Z,         /* treatment assignment */
		    int *D,         /* treatment status */ 
		    int *C,         /* compliance status; 
				       for probit, complier = 1,
				       noncomplier = 0
				       for logit, never-taker = 0,
				       complier = 1, always-taker = 2
				    */
		    int *A,         /* always-takers; always-taker = 1, others
				       = 0 */
		    int *grp,       /* group indicator: 0, 1, 2, ... */
		    int *Ymiss,     /* number of missing obs in Y */
		    int *AT,        /* Are there always-takers? */
		    int *Insample,  /* Insample (=1) or population QoI? */
		    double *dXc,    /* fixed effects for compliance model */
		    double *dZc,    /* random effects for compliance model */
		    double *dXo,    /* fixed effects for outcome model */
		    double *dZo,    /* random effects for outcome model */
		    double *dXr,    /* fixed effects for response model */
		    double *dZr,    /* random effects for response model */
		    double *betaC,  /* fixed effects for compliance model */
		    double *betaA,  /* fixed effects for always-takers model */
		    double *gamma,  /* fixed effects for outcome model */
		    double *delta,  /* fixed effects for response model */
		    int *in_samp,   /* # of observations */
		    int *n_gen,     /* # of Gibbs draws */
		    int *in_grp,     /* # of groups */
		    int *n_samp_grp,/* # of obs within group */
		    int *max_samp_grp, /* max # of obs within group */
		    int *in_fixedC, /* # of fixed effects for compliance model */ 
		    int *in_fixedO, /* # of fixed effects for outcome model */
		    int *in_fixedR, /* # of covariates for response model */
		    int *in_randomC,/* # of random effects for compliance model */ 
		    int *in_randomO,/* # of random effects for outcome model */
		    int *in_randomR,/* # of covariates for response model */
		    double *dPsiC,  /* precision for compliance model */
		    double *dPsiA,  /* precision for always-takers model */
		    double *dPsiO,  /* precision for outcome model */
		    double *dPsiR,  /* precision for response model */
		    double *beta0,  /* prior mean for betaC and betaA */ 
		    double *gamma0, /* prior mean for gamma */
		    double *delta0, /* prior mean for delta */
		    double *dA0C,   /* prior precision for betaC and betaA */ 
		    double *dA0O,   /* prior precision for gamma */
		    double *dA0R,   /* prior precision for delta */
		    int *tau0C,     /* prior df for PsiC */
		    int *tau0A,     /* prior df for PsiA */
		    int *tau0O,     /* prior df for PsiO */
		    int *tau0R,     /* prior df for PsiR */
		    double *dT0C,   /* prior scale for PsiC */
		    double *dT0A,   /* prior scale for PsiA */
		    double *dT0O,   /* prior scale for PsiO */
		    double *dT0R,   /* prior scale for PsiR */
		    double *Var,    /* proposal variance */
		    int *logitC,    /* Use logistic regression for the
				       compliance model? */
		    int *param,     /* Want to keep paramters? */
		    int *mda,       /* Want to use marginal data
				       augmentation for probit regressions? */
		    int *burnin,    /* number of burnin */
		    int *iKeep,     /* keep ?th draws */
		    int *verbose,   /* print out messages */
		    double *coefC,  /* Storage for coefficients of the
				       compliance model */
		    double *coefA,  /* Storage for coefficients of the
				       always-takers model */
		    double *coefO,  /* Storage for coefficients of the
				       outcome model */
		    double *coefR,  /* Storage for coefficients of the
				       response model */	
		    double *sPsiC,  /* Storage for precisions of
				       random effects */
		    double *sPsiA,
		    double *sPsiO,
		    double *sPsiR, 
		    double *QoI     /* Storage of quantities of
				       interest */		 
		    ) {
   /** counters **/
  int n_samp = *in_samp; int n_grp = *in_grp;
  int n_fixedC = *in_fixedC; int n_randomC = *in_randomC;
  int n_fixedO = *in_fixedO; int n_randomO = *in_randomO;
  int n_fixedR = *in_fixedR; int n_randomR = *in_randomR;
  int n_miss = *Ymiss;
  int n_obs = n_samp - n_miss;

  /*** data ***/
  int *grp_obs = intArray(n_obs);

  /*** observed Y ***/
  int *Yobs = intArray(n_obs);

  /* covariates for fixed effects in the compliance model */
  double **Xc = doubleMatrix(n_samp+n_fixedC, n_fixedC+1);
  /* covariates for random effects */
  double ***Zc = doubleMatrix3D(n_grp, *max_samp_grp + n_randomC,
				n_randomC +1);

  /* covariates for fixed effects in the outcome model */
  double **Xo = doubleMatrix(n_samp+n_fixedO, n_fixedO+1);    
  /* covariates for random effects */
  double ***Zo = doubleMatrix3D(n_grp, *max_samp_grp + n_randomO,
				n_randomO +1);

  /* covariates for fixed effecs in the outcome model: only units with observed Y */     
  double **Xobs = doubleMatrix(n_obs+n_fixedO, n_fixedO+1);    
  /* covariates for random effects */
  double ***Zobs = doubleMatrix3D(n_grp, *max_samp_grp + n_randomO,
				  n_randomO +1);

  /* covariates for fixed effects in the response model: includes all obs */     
  double **Xr = doubleMatrix(n_samp+n_fixedR, n_fixedR+1);    
  /* covariates for random effects */
  double ***Zr = doubleMatrix3D(n_grp, *max_samp_grp + n_randomR,
				n_randomR +1);

  /*** model parameters ***/
  /* random effects */
  double **xiC = doubleMatrix(n_grp, n_randomC);
  double **xiA = doubleMatrix(n_grp, n_randomC);
  double **xiO = doubleMatrix(n_grp, n_randomO);
  double **xiR = doubleMatrix(n_grp, n_randomR);

  /* covariances for random effects */
  double **PsiC = doubleMatrix(n_randomC, n_randomC);
  double **PsiA = doubleMatrix(n_randomC, n_randomC);
  double **PsiO = doubleMatrix(n_randomO, n_randomO);
  double **PsiR = doubleMatrix(n_randomR, n_randomR);

  /* mean vector for the outcome model */
  double *meano = doubleArray(n_samp);
  /* mean vector for the compliance model */
  double *meanc = doubleArray(n_samp);

  /* probability of Y = 1 for a complier */
  double *pC = doubleArray(n_samp); 
  double *pN = doubleArray(n_samp);

  /* probability of R = 1 */
  double *prC = doubleArray(n_samp);
  double *prN = doubleArray(n_samp);
  double *prA = doubleArray(n_samp);

  /* probability of being a complier and never-taker */
  double *qC = doubleArray(n_samp);
  double *qN = doubleArray(n_samp);

  /* probability of being a always-taker */
  double *pA = doubleArray(n_samp);
  double *meana = doubleArray(n_samp);

  /* prior precision matrices */
  double **A0C = doubleMatrix(n_fixedC*2, n_fixedC*2);
  double **A0O = doubleMatrix(n_fixedO, n_fixedO);
  double **A0R = doubleMatrix(n_fixedR, n_fixedR);
  
  /* prior scale for Psi's */
  double **T0C = doubleMatrix(n_randomC, n_randomC);
  double **T0A = doubleMatrix(n_randomC, n_randomC);
  double **T0O = doubleMatrix(n_randomO, n_randomO);
  double **T0R = doubleMatrix(n_randomR, n_randomR);

  /* subset of the data */
  double **Xtemp = doubleMatrix(n_samp+n_fixedC, n_fixedC+1);
  int *Atemp = intArray(n_samp);
  double ***Ztemp = doubleMatrix3D(n_grp, *max_samp_grp + n_randomC,
				   n_randomC +1);
  int *grp_temp = intArray(n_samp);

  /* quantities of interest: ITT, CACE  */
  double ITT, CACE;
  double Y1barC, Y0barC, YbarN, YbarA;
  int n_comp;          /* number of compliers */
  int n_never;
  double p_comp, p_never; /* prob. of being a particular type */

  /*** storage parameters and loop counters **/
  int *vitemp = intArray(n_grp);
  int *vitemp1 = intArray(n_grp);
  int progress = 1;
  int keep = 1;
  int *accept = intArray(n_fixedC*2);      /* number of acceptance */
  int i, j, k, l, main_loop;
  int itempP = ftrunc((double) *n_gen/10);
  int itemp, itempA, itempC, itempO, itempQ, itempR;
  int itempAv, itempCv, itempOv, itempRv;
  double dtemp;
  double **mtempC = doubleMatrix(n_fixedC, n_fixedC); 
  double **mtempO = doubleMatrix(n_fixedO, n_fixedO); 
  double **mtempR = doubleMatrix(n_fixedR, n_fixedR); 

  /*** get random seed **/
  GetRNGstate();

  /*** pack the fixed effects covariates ***/
  itemp = 0;
  for (j = 0; j < n_fixedC; j++)
    for (i = 0; i < n_samp; i++)
      Xc[i][j] = dXc[itemp++];

  itemp = 0;
  for (j = 0; j < n_fixedO; j++)
    for (i = 0; i < n_samp; i++)
      Xo[i][j] = dXo[itemp++];

  itemp = 0;
  for (j = 0; j < n_fixedR; j++)
    for (i = 0; i < n_samp; i++)
      Xr[i][j] = dXr[itemp++];

  itemp = 0;
  for (i = 0; i < n_samp; i++) 
    if (R[i] == 1) {
      Yobs[itemp] = Y[i];
      grp_obs[itemp] = grp[i];
      for (j = 0; j < n_fixedO; j++)
	Xobs[itemp][j] = Xo[i][j];
      itemp++;
    }
  
  /** pack random effects **/
  itemp = 0;
  for (k = 0; k < n_randomC; k++)
    for (j = 0; j < n_grp; j++)
      xiC[j][k] = 0;

  for (k = 0; k < n_randomC; k++)
    for (j = 0; j < n_grp; j++)
      xiA[j][k] = 0;

  itemp = 0;
  for (k = 0; k < n_randomO; k++)
    for (j = 0; j < n_grp; j++)
      xiO[j][k] = 0;

  itemp = 0;
  for (k = 0; k < n_randomR; k++)
    for (j = 0; j < n_grp; j++)
      xiR[j][k] = 0;

  /** pack random effects covariates **/
  itemp = 0;
  for (j = 0; j < n_grp; j++)
    vitemp[j] = 0;
  for (i = 0; i < n_samp; i++) {
    for (j = 0; j < n_randomC; j++)
      Zc[grp[i]][vitemp[grp[i]]][j] = dZc[itemp++];
    vitemp[grp[i]]++;
  }

  itemp = 0;
  for (j = 0; j < n_grp; j++)
    vitemp[j] = 0;
  for (i = 0; i < n_samp; i++) {
    for (j = 0; j < n_randomO; j++)
      Zo[grp[i]][vitemp[grp[i]]][j] = dZo[itemp++];
    vitemp[grp[i]]++;
  }

  itemp = 0;
  for (j = 0; j < n_grp; j++) {
    vitemp[j] = 0; 
    vitemp1[j] = 0;
  }
  for (i = 0; i < n_samp; i++) { 
    if (R[i] == 1) {
      for (j = 0; j < n_randomO; j++)
	Zobs[grp[i]][vitemp1[grp[i]]][j] = Zo[grp[i]][vitemp[grp[i]]][j];
      vitemp1[grp[i]]++;
    }
    vitemp[grp[i]]++;
  }

  itemp = 0;
  for (j = 0; j < n_grp; j++)
    vitemp[j] = 0;
  for (i = 0; i < n_samp; i++) {
    for (j = 0; j < n_randomR; j++)
      Zr[grp[i]][vitemp[grp[i]]][j] = dZr[itemp++];
    vitemp[grp[i]]++;
  }

  /* covariance parameters and its prior */
  itemp = 0;
  for (k = 0; k < n_randomC; k++)
    for (j = 0; j < n_randomC; j++)
      PsiC[j][k] = dPsiC[itemp++];
  itemp = 0;
  for (k = 0; k < n_randomC; k++)
    for (j = 0; j < n_randomC; j++)
      T0C[j][k] = dT0C[itemp++];

  itemp = 0;
  for (k = 0; k < n_randomC; k++)
    for (j = 0; j < n_randomC; j++)
      PsiA[j][k] = dPsiA[itemp++];
  itemp = 0;
  for (k = 0; k < n_randomC; k++)
    for (j = 0; j < n_randomC; j++)
      T0A[j][k] = dT0A[itemp++];

  itemp = 0;
  for (k = 0; k < n_randomO; k++)
    for (j = 0; j < n_randomO; j++)
      PsiO[j][k] = dPsiO[itemp++];
  itemp = 0;
  for (k = 0; k < n_randomO; k++)
    for (j = 0; j < n_randomO; j++)
      T0O[j][k] = dT0O[itemp++];

  itemp = 0;
  for (k = 0; k < n_randomR; k++)
    for (j = 0; j < n_randomR; j++)
      PsiR[j][k] = dPsiR[itemp++];
  itemp = 0;
  for (k = 0; k < n_randomR; k++)
    for (j = 0; j < n_randomR; j++)
      T0R[j][k] = dT0R[itemp++];

  /* prior for fixed effects as additional data points */ 
  itemp = 0; 
  if ((*logitC == 1) && (*AT == 1))
    for (k = 0; k < n_fixedC*2; k++)
      for (j = 0; j < n_fixedC*2; j++)
	A0C[j][k] = dA0C[itemp++];
  else
    for (k = 0; k < n_fixedC; k++)
      for (j = 0; j < n_fixedC; j++)
	A0C[j][k] = dA0C[itemp++];

  itemp = 0;
  for (k = 0; k < n_fixedO; k++)
    for (j = 0; j < n_fixedO; j++)
      A0O[j][k] = dA0O[itemp++];

  itemp = 0;
  for (k = 0; k < n_fixedR; k++)
    for (j = 0; j < n_fixedR; j++)
      A0R[j][k] = dA0R[itemp++];

  if (*logitC != 1) {
    dcholdc(A0C, n_fixedC, mtempC);
    for (i = 0; i < n_fixedC; i++) {
      Xc[n_samp+i][n_fixedC]=0;
      for (j = 0; j < n_fixedC; j++) {
	Xc[n_samp+i][n_fixedC] += mtempC[i][j]*beta0[j];
	Xc[n_samp+i][j] = mtempC[i][j];
      }
    }
  }

  dcholdc(A0O, n_fixedO, mtempO);
  for (i = 0; i < n_fixedO; i++) {
    Xobs[n_obs+i][n_fixedO]=0;
    for (j = 0; j < n_fixedO; j++) {
      Xobs[n_obs+i][n_fixedO] += mtempO[i][j]*gamma0[j];
      Xobs[n_obs+i][j] = mtempO[i][j];
    }
  }
  
  dcholdc(A0R, n_fixedR, mtempR);
  for (i = 0; i < n_fixedR; i++) {
    Xr[n_samp+i][n_fixedR]=0;
    for (j = 0; j < n_fixedR; j++) {
      Xr[n_samp+i][n_fixedR] += mtempR[i][j]*delta0[j];
      Xr[n_samp+i][j] = mtempR[i][j];
    }
  }
  
  /*** starting values for probabilities ***/
  for (i = 0; i < n_samp; i++) {
    pC[i] = unif_rand(); 
    pN[i] = unif_rand(); 
    pA[i] = unif_rand();
    if (n_miss > 0) {
      prC[i] = unif_rand(); 
      prN[i] = unif_rand(); 
      prA[i] = unif_rand();
    } else { /* no missing values in Y */
      prC[i] = 1;
      prN[i] = 1;
      prA[i] = 1;
    }
  }

  /*** Gibbs Sampler! ***/
  itempA = 0; itempC = 0; itempO = 0; itempQ = 0; itempR = 0;   
  itempAv = 0; itempCv = 0; itempOv = 0; itempRv = 0;   
  for (j = 0; j < n_fixedC*2; j++)
    accept[j] = 0;
  for (main_loop = 1; main_loop <= *n_gen; main_loop++){

    /* Step 1: RESPONSE MODEL */
    if (n_miss > 0) {
      bprobitMixedGibbs(R, Xr, Zr, grp, delta, xiR, PsiR, n_samp, 
			n_fixedR, n_randomR, n_grp, n_samp_grp, 0, 
			delta0, A0R, *tau0R, T0R, *mda, 1);
 
      /* Compute probabilities of R = Robs */ 
      for (j = 0; j < n_grp; j++) vitemp[j] = 0;
      for (i = 0; i < n_samp; i++) {
	dtemp = 0;
	if (*AT == 1) { /* always-takers */
	  for (j = 3; j < n_fixedR; j++)
	    dtemp += Xr[i][j]*delta[j];
	  for (j = 3; j < n_randomR; j++)
	    dtemp += Zr[grp[i]][vitemp[grp[i]]][j]*xiR[grp[i]][j];
	  if ((Z[i] == 0) && (D[i] == 0)) {
	    prC[i] = R[i]*pnorm(dtemp+delta[1]+xiR[grp[i]][1], 0, 1, 1, 0) +
	      (1-R[i])*pnorm(dtemp+delta[1]+xiR[grp[i]][1], 0, 1, 0, 0);
	    prN[i] = R[i]*pnorm(dtemp, 0, 1, 1, 0) +
	      (1-R[i])*pnorm(dtemp, 0, 1, 0, 0);
	  } 
	  if ((Z[i] == 1) && (D[i] == 1)) {
	    prC[i] = R[i]*pnorm(dtemp+delta[0]+xiR[grp[i]][0], 0, 1, 1, 0) +
	      (1-R[i])*pnorm(dtemp+delta[0]+xiR[grp[i]][0], 0, 1, 0, 0);
	    prA[i] = R[i]*pnorm(dtemp+delta[2]+xiR[grp[i]][2], 0, 1, 1, 0) +
	      (1-R[i])*pnorm(dtemp+delta[2]+xiR[grp[i]][2], 0, 1, 0, 0);
	  }
	} else { /* no always-takers */
	  for (j = 2; j < n_fixedR; j++)
	    dtemp += Xr[i][j]*delta[j];
	  for (j = 2; j < n_randomR; j++)
	    dtemp += Zr[grp[i]][vitemp[grp[i]]][j]*xiR[grp[i]][j];
	  if (Z[i] == 0) {
	    prC[i] = R[i]*pnorm(dtemp+delta[1]+xiR[grp[i]][1], 0, 1, 1, 0) + 
	      (1-R[i])*pnorm(dtemp+delta[1]+xiR[grp[i]][1], 0, 1, 0, 0);
	    prN[i] = R[i]*pnorm(dtemp, 0, 1, 1, 0) +
	      (1-R[i])*pnorm(dtemp, 0, 1, 0, 0);
	  }
	} 
	vitemp[grp[i]]++;
      }
    }

    /** Step 2: COMPLIANCE MODEL **/
    if (*logitC == 1) 
      if (*AT == 1) 
	logitMetro(C, Xc, betaC, n_samp, 2, n_fixedC, beta0, A0C, Var, 1,
		   accept); 
      else 
	logitMetro(C, Xc, betaC, n_samp, 1, n_fixedC, beta0, A0C, Var, 1,
		   accept);  
    else {
      /* complier vs. noncomplier */
      bprobitMixedGibbs(C, Xc, Zc, grp, betaC, xiC, PsiC, n_samp,
			n_fixedC, n_randomC, n_grp, n_samp_grp, 0, 
			beta0, A0C, *tau0C, T0C, *mda, 1);
      if (*AT == 1) {
	/* never-taker vs. always-taker */
	/* subset the data */
	itemp = 0;
	for (j = 0; j < n_grp; j++) {
	  vitemp[j] = 0; vitemp1[j] = 0;
	}
	for (i = 0; i < n_samp; i++) {
	  if (C[i] == 0) {
	    Atemp[itemp] = A[i]; grp_temp[itemp] = grp[i];
	    for (j = 0; j < n_fixedC; j++)
	      Xtemp[itemp][j] = Xc[i][j];
	    for (j = 0; j < n_randomC; j++)
	      Ztemp[grp[i]][vitemp1[grp[i]]][j] = Zc[grp[i]][vitemp[grp[i]]][j];
	    itemp++; vitemp1[grp[i]]++;
	  }
	  vitemp[grp[i]]++;
	}
	for (i = n_samp; i < n_samp + n_fixedC; i++) {
	  for (j = 0; j <= n_fixedC; j++)
	    Xtemp[itemp][j] = Xc[i][j];
	  itemp++;
	}
	bprobitMixedGibbs(Atemp, Xtemp, Ztemp, grp_temp, betaA, xiA,
			  PsiA, itemp-n_fixedC, n_fixedC, n_randomC,
			  n_grp, vitemp1, 0, beta0, A0C, *tau0A, T0A, 
			  *mda, 1); 
      }      
    }    

    /* Step 3: SAMPLE COMPLIANCE COVARITE */
    itemp = 0;
    for (j = 0; j < n_grp; j++) {
      vitemp[j] = 0;
      vitemp1[j] = 0;
    }
    for (i = 0; i < n_samp; i++) {
      meanc[i] = 0;
      for (j = 0; j < n_fixedC; j++) 
	meanc[i] += Xc[i][j]*betaC[j];
      for (j = 0; j < n_randomC; j++)
	meanc[i] += Zc[grp[i]][vitemp[grp[i]]][j]*xiC[grp[i]][j];
      if (*AT == 1) { /* some always-takers */
	meana[i] = 0;
	if (*logitC == 1) { /* if logistic regression is used */
	  for (j = 0; j < n_fixedC; j++) 
	    meana[i] += Xc[i][j]*betaC[j+n_fixedC];
	  qC[i] = exp(meanc[i])/(1 + exp(meanc[i]) + exp(meana[i]));
	  qN[i] = 1/(1 + exp(meanc[i]) + exp(meana[i]));
	} else { /* double probit regressions */
	  for (j = 0; j < n_fixedC; j++) 
	    meana[i] += Xc[i][j]*betaA[j];
	  for (j = 0; j < n_randomC; j++)
	    meana[i] += Zc[grp[i]][vitemp[grp[i]]][j]*xiA[grp[i]][j];
	  qC[i] = pnorm(meanc[i], 0, 1, 1, 0);
	  qN[i] = (1-qC[i])*pnorm(meana[i], 0, 1, 0, 0);
	}
	if ((Z[i] == 0) && (D[i] == 0)){
	  if (R[i] == 1)
	    dtemp = qC[i]*pC[i]*prC[i] / 
	      (qC[i]*pC[i]*prC[i]+qN[i]*pN[i]*prN[i]);
	  else 
	    dtemp = qC[i]*prC[i]/(qC[i]*prC[i]+qN[i]*prN[i]);
	  if (unif_rand() < dtemp) {
	    C[i] = 1; Xr[i][1] = 1; Zr[grp[i]][vitemp[grp[i]]][1] = 1;
	    if (R[i] == 1) {
	      Xobs[itemp][1] = 1; Zobs[grp[i]][vitemp1[grp[i]]][1] = 1;
	    }
	  }
	  else {
	    C[i] = 0; Xr[i][1] = 0; Zr[grp[i]][vitemp[grp[i]]][1] = 0;
	    if (R[i] == 1) {
	      Xobs[itemp][1] = 0; Zobs[grp[i]][vitemp1[grp[i]]][1] = 0; 
	    }
	  }  
	}
	if ((Z[i] == 1) && (D[i] == 1)){
	  if (R[i] == 1)
	    dtemp = qC[i]*pC[i]*prC[i] / 
	      (qC[i]*pC[i]*prC[i]+(1-qC[i]-qN[i])*pA[i]*prA[i]);
	  else
	    dtemp = qC[i]*prC[i]/(qC[i]*prC[i]+(1-qC[i]-qN[i])*prA[i]);
	  if (unif_rand() < dtemp) {
	    C[i] = 1; Xr[i][0] = 1; Zr[grp[i]][vitemp[grp[i]]][0] = 1;
	    A[i] = 0; Xr[i][2] = 0; Zr[grp[i]][vitemp[grp[i]]][2] = 0;
	    if (R[i] == 1) {
	      Xobs[itemp][0] = 1; Zobs[grp[i]][vitemp1[grp[i]]][0] = 1;
	      Xobs[itemp][2] = 0; Zobs[grp[i]][vitemp1[grp[i]]][2] = 0;
	    }
	  }
	  else {
	    if (*logitC == 1)
	      C[i] = 2;
	    else
	      C[i] = 0; 
	    A[i] = 1; 
	    Xr[i][0] = 0; Zr[grp[i]][vitemp[grp[i]]][0] = 0; 
	    Xr[i][2] = 1; Zr[grp[i]][vitemp[grp[i]]][2] = 1;
	    if (R[i] == 1) {
	      Xobs[itemp][0] = 0; Zobs[grp[i]][vitemp1[grp[i]]][0] = 0;
	      Xobs[itemp][2] = 1; Zobs[grp[i]][vitemp1[grp[i]]][2] = 1;
	    } 
	  }  
	}
      } else { /* no always-takers */
	if (Z[i] == 0){
	  if (*logitC == 1)
	    qC[i] = 1/(1+exp(-meanc[i]));
	  else
	    qC[i] = pnorm(meanc[i], 0, 1, 1, 0);
	  if (R[i] == 1)
	    dtemp = qC[i]*pC[i]*prC[i] / 
	      (qC[i]*pC[i]*prC[i]+(1-qC[i])*pN[i]*prN[i]);
	  else
	    dtemp = qC[i]*prC[i]/(qC[i]*prC[i]+(1-qC[i])*prN[i]);
	  if (unif_rand() < dtemp) {
	    C[i] = 1; Xr[i][1] = 1;
	    Zr[grp[i]][vitemp[grp[i]]][1] = 1;
	    if (R[i] == 1) {
	      Xobs[itemp][1] = 1; Zobs[grp[i]][vitemp1[grp[i]]][1] = 1;
	    } 
	  }
	  else {
	    C[i] = 0; Xr[i][1] = 0;
	    Zr[grp[i]][vitemp[grp[i]]][1] = 0;
	    if (R[i] == 1) {
	      Xobs[itemp][1] = 0; Zobs[grp[i]][vitemp1[grp[i]]][1] = 0;
	    } 
	  }
	}
      }
      if (R[i] == 1) {
	itemp++; vitemp1[grp[i]]++;
      }
      vitemp[grp[i]]++;
    }

    /** Step 4: OUTCOME MODEL **/
    bprobitMixedGibbs(Yobs, Xobs, Zobs, grp_obs, gamma, xiO, PsiO,
		      n_obs, n_fixedO, n_randomO, n_grp, vitemp1, 0,
		      gamma0, A0O, *tau0O, T0O, *mda, 1); 
    
    /** Compute probabilities of Y = 1 **/
    for (j = 0; j < n_grp; j++)
      vitemp[j] = 0;
    for (i = 0; i < n_samp; i++) {
      meano[i] = 0;
      if (*AT == 1) { /* always-takers */
	for (j = 3; j < n_fixedO; j++)
	  meano[i] += Xo[i][j]*gamma[j];
	for (j = 3; j < n_randomO; j++)
	  meano[i] += Zo[grp[i]][vitemp[grp[i]]][j]*xiO[grp[i]][j];
	if (R[i] == 1) {
	  if ((Z[i] == 0) && (D[i] == 0)) {
	    pC[i] = Y[i]*pnorm(meano[i]+gamma[1]+xiO[grp[i]][1], 0, 1, 1, 0) +
	      (1-Y[i])*pnorm(meano[i]+gamma[1]+xiO[grp[i]][1], 0, 1, 0, 0);
	    pN[i] = Y[i]*pnorm(meano[i], 0, 1, 1, 0) +
	      (1-Y[i])*pnorm(meano[i], 0, 1, 0, 0);
	  } 
	  if ((Z[i] == 1) && (D[i] == 1)) {
	    pC[i] = Y[i]*pnorm(meano[i]+gamma[0]+xiO[grp[i]][0], 0, 1, 1, 0) +
	      (1-Y[i])*pnorm(meano[i]+gamma[0]+xiO[grp[i]][0], 0, 1, 0, 0);
	    pA[i] = Y[i]*pnorm(meano[i]+gamma[2]+xiO[grp[i]][2], 0, 1, 1, 0) +
	      (1-Y[i])*pnorm(meano[i]+gamma[2]+xiO[grp[i]][2], 0, 1, 0, 0);
	  }
	}
      } else { /* no always-takers */
	for (j = 2; j < n_fixedO; j++)
	  meano[i] += Xo[i][j]*gamma[j];
	for (j = 2; j < n_randomO; j++)
	  meano[i] += Zo[grp[i]][vitemp[grp[i]]][j]*xiO[grp[i]][j];
	if (R[i] == 1)
	  if (Z[i] == 0) {
	    pC[i] = Y[i]*pnorm(meano[i]+gamma[1]+xiO[grp[i]][1], 0, 1, 1, 0) + 
	      (1-Y[i])*pnorm(meano[i]+gamma[1]+xiO[grp[i]][1], 0, 1, 0, 0);
	    pN[i] = Y[i]*pnorm(meano[i], 0, 1, 1, 0) +
	      (1-Y[i])*pnorm(meano[i], 0, 1, 0, 0);
	  }
      } 
    }
    
    /** storing the results **/
    if (main_loop > *burnin) {
      if (keep == *iKeep) {
	/** Computing Quantities of Interest **/
	n_comp = 0; n_never = 0; p_comp = 0; p_never = 0; 
	Y1barC = 0; Y0barC = 0; YbarN = 0; YbarA = 0;
	for (i = 0; i < n_samp; i++){
	  p_comp += qC[i];
	  p_never += qN[i];
	  if (C[i] == 1) { /* ITT effects */
	    n_comp++;
	    if (*Insample == 1) { /* insample QoI */
	      if ((Z[i] == 1) && (R[i] == 1))
		Y1barC += (double)Y[i];
	      else if (Z[i] == 1 && (R[i] == 0))
		Y1barC += (double)((meano[i]+gamma[0]+xiO[grp[i]][0]+norm_rand()) > 0);
	      else if ((Z[i] == 0) && (R[i] == 1))
		Y0barC += (double)Y[i];
	      else
		Y0barC += (double)((meano[i]+gamma[1]+xiO[grp[i]][1]+norm_rand()) > 0);
	    } else { /* population QoI */
	      Y1barC += pnorm(meano[i]+gamma[0]+xiO[grp[i]][0], 0, 1, 1, 0);
	      Y0barC += pnorm(meano[i]+gamma[1]+xiO[grp[i]][1], 0, 1, 1, 0); 
	    }
	  } else { /* Estimates for always-takers and never-takers */
	    if (A[i] == 1)
	      if (*Insample == 1)
		if (R[i] == 1)
		  YbarA += (double)Y[i];
		else
		  YbarA += (double)((meano[i]+gamma[2]+xiO[grp[i]][2]+norm_rand()) > 0);
	      else
		YbarA += pnorm(meano[i]+gamma[2]+xiO[grp[i]][2], 0, 1, 1, 0);
	    else {
	      n_never++;
	      if (*Insample == 1)
		if (R[i] == 1)
		  YbarN += (double)Y[i];
		else
		  YbarN += (double)((meano[i]+norm_rand()) > 0);
	      else
		YbarN += pnorm(meano[i], 0, 1, 1, 0);
	    }
	  }
	}
	ITT = (Y1barC-Y0barC)/(double)n_samp;     /* ITT effect */
	CACE = (Y1barC-Y0barC)/(double)n_comp;     /* CACE */
	if (*Insample == 1) { 
	  p_comp = (double)n_comp/(double)n_samp;
	  p_never = (double)n_never/(double)n_samp;
	} else {
	  p_comp /= (double)n_samp;  /* ITT effect on D; Prob. of being
					a complier */ 
	  p_never /= (double)n_samp; /* Prob. of being a never-taker */
	}
	Y1barC /= (double)n_comp;  /* E[Y_i(j)|C_i=1] for j=0,1 */ 
	Y0barC /= (double)n_comp; 
	YbarN /= (double)n_never;
	if (*AT == 1)
	  YbarA /= (double)(n_samp-n_comp-n_never);
	
	QoI[itempQ++] = ITT;   
	QoI[itempQ++] = CACE;   
	QoI[itempQ++] = p_comp; 	  
	QoI[itempQ++] = p_never;
	QoI[itempQ++] = Y1barC;
	QoI[itempQ++] = Y0barC;
	QoI[itempQ++] = YbarN;
	if (*AT == 1)
	  QoI[itempQ++] = YbarA;

	if (*param == 1) {
	  for (j = 0; j < n_fixedC; j++)
	    coefC[itempC++] = betaC[j];
	  /* for (j = 0; j < n_randomC; j++)
	     for (k = j; k < n_randomC; k++)
	     sPsiC[itempCv++] = PsiC[j][k]; */
	  if (*AT == 1) {
	    if (*logitC == 1)
	      for (j = 0; j < n_fixedC; j++)
		coefA[itempA++] = betaC[j+n_fixedC];
	    else
	      for (j = 0; j < n_fixedC; j++)
		coefA[itempA++] = betaA[j];
	    /* for (j = 0; j < n_randomC; j++)
	      for (k = j; k < n_randomC; k++)
	      sPsiA[itempAv++] = PsiA[j][k]; */
	  }

	  for (j = 0; j < n_fixedO; j++)
	    coefO[itempO++] = gamma[j];
	  /* for (j = 0; j < n_randomO; j++)
	    for (k = j; k < n_randomO; k++)
	    sPsiO[itempOv++] = PsiO[j][k]; */
	  
	  if (n_miss > 0) { 
	    for (j = 0; j < n_fixedR; j++)
	      coefR[itempR++] = delta[j];
	    /* for (j = 0; j < n_randomR; j++)
	       for (k = j; k < n_randomR; k++)
	       sPsiR[itempRv++] = PsiR[j][k]; */
	  }
	}
	keep = 1;
      }
      else
	keep++;
    }

    if (*verbose) {
      if (main_loop == itempP) {
	Rprintf("%3d percent done.\n", progress*10);
	if (*logitC == 1) {
	  Rprintf("  Current Acceptance Ratio:");
	  if (*AT == 1)
	    for (j = 0; j < n_fixedC*2; j++)
	      Rprintf("%10g", (double)accept[j]/(double)main_loop);
	  else
	    for (j = 0; j < n_fixedC; j++)
	      Rprintf("%10g", (double)accept[j]/(double)main_loop);
	  Rprintf("\n");
	}
	itempP += ftrunc((double) *n_gen/10); 
	progress++;
	R_FlushConsole(); 
      }
    }
    R_FlushConsole();
    R_CheckUserInterrupt();
  } /* end of Gibbs sampler */

  /** write out the random seed **/
  PutRNGstate();

  /** freeing memory **/
  free(grp_obs);
  free(Yobs);
  FreeMatrix(Xc, n_samp+n_fixedC);
  Free3DMatrix(Zc, n_grp, *max_samp_grp + n_randomC);
  FreeMatrix(Xo, n_samp+n_fixedO);
  Free3DMatrix(Zo, n_grp, *max_samp_grp + n_randomO);
  FreeMatrix(Xobs, n_obs+n_fixedO);
  Free3DMatrix(Zobs, n_grp, *max_samp_grp + n_randomO);
  FreeMatrix(Xr, n_samp+n_fixedO);
  Free3DMatrix(Zr, n_grp, *max_samp_grp + n_randomR);
  FreeMatrix(xiC, n_grp);
  FreeMatrix(xiA, n_grp);
  FreeMatrix(xiO, n_grp);
  FreeMatrix(xiR, n_grp);
  FreeMatrix(PsiC, n_randomC);
  FreeMatrix(PsiA, n_randomC);
  FreeMatrix(PsiO, n_randomO);
  FreeMatrix(PsiR, n_randomR);
  free(meano);
  free(meanc);
  free(pC);
  free(pN);
  free(pA);
  free(prC);
  free(prN);
  free(prA);
  free(meana);
  FreeMatrix(A0C, n_fixedC*2);
  FreeMatrix(A0O, n_fixedO);
  FreeMatrix(A0R, n_fixedO);
  FreeMatrix(T0C, n_randomC);
  FreeMatrix(T0A, n_randomC);
  FreeMatrix(T0O, n_randomO);
  FreeMatrix(T0R, n_randomR);
  FreeMatrix(Xtemp, n_samp+n_fixedC);
  Free3DMatrix(Ztemp, n_grp, *max_samp_grp + n_randomC);
  free(grp_temp);
  free(Atemp);
  free(vitemp);
  free(vitemp1);
  free(accept);
  FreeMatrix(mtempC, n_fixedC);
  FreeMatrix(mtempO, n_fixedO);
  FreeMatrix(mtempR, n_fixedR);

} /* main */


/******************************************************************
  This file is a part of MNP: R Package for Estimating the 
  Multinomial Probit Models by Kosuke Imai and  David A. van Dyk.
  Copyright: GPL version 2 or later.
*******************************************************************/

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <R_ext/Utils.h>
#include <R.h>

int* intArray(int num) {
  int *iArray = (int *)malloc(num * sizeof(int));
  if (iArray)
    return iArray;
  else 
    error("Out of memory error in intArray\n");
}

void PintArray(int *ivector, int length) {
  int i;
  for (i = 0; i < length; i++)
    Rprintf("%5d\n", ivector[i]);
}

int** intMatrix(int row, int col) {
  int i;
  int **iMatrix = (int **)malloc(row * sizeof(int *));
  if (iMatrix) {
    for (i = 0; i < row; i++) {
      iMatrix[i] = (int *)malloc(col *  sizeof(int));
      if (!iMatrix[i]) 
	error("Out of memory error in intMatrix\n");
    }
    return iMatrix;
  }
  else 
    error("Out of memory error in intMatrix\n");
}

void PintMatrix(int **imatrix, int row, int col) {
  int i, j;
  for (i = 0; i < row; i++) {
    for (j = 0; j < col; j++)
      Rprintf("%5d", imatrix[i][j]);
    Rprintf("\n");
  }
}


double* doubleArray(int num) {
  double *dArray = (double *)malloc(num * sizeof(double));
  if (dArray)
    return dArray;
  else
    error("Out of memory error in doubleArray\n");
}

void PdoubleArray(double *dvector, int length) {
  int i;
  for (i = 0; i < length; i++)
    Rprintf("%14g\n", dvector[i]);
}

double** doubleMatrix(int row, int col) {
  int i;
  double **dMatrix = (double **)malloc((size_t)(row * sizeof(double *)));
  if (dMatrix) {
    for (i = 0; i < row; i++) {
      dMatrix[i] = (double *)malloc((size_t)(col * sizeof(double)));
      if (!dMatrix[i])
	error("Out of memory error in doubleMatrix\n");
    }
    return dMatrix;
  }
  else
    error("Out of memory error in doubleMatrix\n");
}

void PdoubleMatrix(double **dmatrix, int row, int col) {
  int i, j;
  for (i = 0; i < row; i++) {
    for (j = 0; j < col; j++)
      Rprintf("%14g", dmatrix[i][j]);
    Rprintf("\n");
  }
}

double*** doubleMatrix3D(int x, int y, int z) {
  int i;
  double ***dM3 = (double ***)malloc(x * sizeof(double **));
  if (dM3) {
    for (i = 0; i < x; i++) 
      dM3[i] = doubleMatrix(y, z);
    return dM3;
  }
  else 
    error("Out of memory error in doubleMatrix3D\n");
}

void PdoubleMatrix3D(double ***dmatrix3D, int x, int y, int z) {
  int i, j, k;
  for (i = 0; i < x; i++) {
    Rprintf("Fist dimension = %5d\n", i);
    for (j = 0; j < y; j++) {
      for (k = 0; k < z; k++)
	Rprintf("%14g", dmatrix3D[i][j][k]);
      Rprintf("\n");
    }
  }
}

long* longArray(int num) {
  long *lArray = (long *)malloc(num * sizeof(long));
  if (lArray)
    return lArray;
  else 
    error("Out of memory error in longArray\n");
}

void FreeMatrix(double **Matrix, int row) {
  int i;
  for (i = 0; i < row; i++)
    free(Matrix[i]);
  free(Matrix);
}

void FreeintMatrix(int **Matrix, int row) {
  int i;
  for (i = 0; i < row; i++)
    free(Matrix[i]);
  free(Matrix);
}

void Free3DMatrix(double ***Matrix, int index, int row) {
  int i;
  for (i = 0; i < index; i++)
    FreeMatrix(Matrix[i], row);
  free(Matrix);
}
		
	
		
			

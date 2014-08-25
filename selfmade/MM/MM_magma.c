/* basd on code from  Evgenii B. Rudnyi, http://MatrixProgramming.com */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/resource.h>
#include <stdint.h>

#include <math.h>
#include <cuda_runtime_api.h>
#include "cublas_v2.h"
#include "magma.h"
// #include "magma_lapack.h"

#define dim1 50
#define dim2 50
#define dim3 50

#define BILLION 1000000000

int64_t timespecDiff(struct timespec *timeA_p, struct timespec *timeB_p)
{
  return ((timeA_p->tv_sec * BILLION) + timeA_p->tv_nsec) -
           ((timeB_p->tv_sec * BILLION) + timeB_p->tv_nsec);
}


int main()
{
//    const rlim_t kStackSize =  16 * 1024 * 1024;   // min stack size = 16 MB
//   struct rlimit rl;
//   int result;

//    result = getrlimit(RLIMIT_STACK, &rl);
//    printf("getrlimit returned result = %d\n", result);
//    if (result == 0)
//    {
//        if (rl.rlim_cur < kStackSize)
//        {
//            rl.rlim_cur = kStackSize;
//            result = setrlimit(RLIMIT_STACK, &rl);
//            fprintf(stdout,"setrlimit returned result = %d\n", result);
//            if (result != 0)
//            {
//                fprintf(stdout,"setrlimit returned result = %d\n", result);
//            }
//        }
//    }


        printf("newdim=%d\n", (int)dim1*(int)dim2);
	double A[dim1][dim2], B[dim2][dim3], C[dim1][dim3];
        double Anew[dim1*dim2];
        double Bnew[dim2*dim3];
        double Cnew[dim1*dim3];
	int i, j, k, nthreads, tid, chunk, NO_WRITE;
	double maxr, timesec;
        struct timespec start, end;
        uint64_t timeElapsed;
        FILE *fp;

        double alpha = 1.;
	double beta = 0.;

        char ch ='N';
        NO_WRITE = 1;

	srand(86456);
	maxr = (double)RAND_MAX;


        printf("dim1 = %d\n",dim1);
        printf("dim2 = %d\n",dim2);
        printf("dim3 = %d\n",dim3);
        printf("max random = %f\n",maxr);

        printf("Initializing matrices...\n");
        /*** Initialize matrices ***/
        printf("Creating M1\n");
        clock_gettime(CLOCK_MONOTONIC, &start);
	for (i = 0; i < dim1; i++)
		for (j = 0; j < dim2; j++)
			A[i][j] = rand()/maxr;

             clock_gettime(CLOCK_MONOTONIC, &end);
             timeElapsed = timespecDiff(&end, &start);
             timesec = (double)timeElapsed/(double)BILLION;
             printf("time for creation of M1  is %llu ns = %fs\n", (long long unsigned int)timeElapsed, timesec);
             if(NO_WRITE) {
             printf("Writing M1 to file M1.dat\n");
   
             fp = fopen("M1.dat", "w");
             if (fp == NULL) {
                printf("I couldn't open M1.dat for writing.\n");
                exit(0);
             }
             for (i = 0; i < dim1; i++)
             for (j = 0; j < dim2; j++)
                fprintf(fp, "%f\t", A[i][j]);
             fclose(fp);
        }

        printf("Creating M2\n");
        clock_gettime(CLOCK_MONOTONIC, &start);
	for (i = 0; i < dim2; i++)
		for (j = 0; j < dim3; j++)
			B[i][j] = rand()/maxr;
            clock_gettime(CLOCK_MONOTONIC, &end);
            timeElapsed = timespecDiff(&end, &start);
            timesec = (double)timeElapsed/(double)BILLION;
            printf("time for creation of M2  is %llu ns = %fs\n", (long long unsigned int)timeElapsed, timesec);
            if(NO_WRITE) {
            printf("Writing M2 to file M2.dat\n");

            fp = fopen("M2.dat", "w");
            if (fp == NULL) {
               printf("I couldn't open M2.dat for writing.\n");
               exit(0);
            }
            for (i = 0; i < dim2; i++)
                for (j = 0; j < dim3; j++)
                        fprintf(fp, "%f\t", B[i][j]);
            fclose(fp);
            }
        printf("Starting M1*M2\n");
	clock_gettime(CLOCK_MONOTONIC, &start);

  /*** Do matrix multiply sharing iterations on outer loop ***/
	for (i = 0; i < dim1; i++)
	{
		for (j = 0; j < dim3; j++)
			C[i][j] = 0.;
		for (k = 0; k < dim2; k++)
			for (j = 0; j < dim3; j++)
				C[i][j] += A[i][k]*B[k][j];
	}

           clock_gettime(CLOCK_MONOTONIC, &end);
           printf("Done\n");
           timeElapsed = timespecDiff(&end, &start);
           timesec = (double)timeElapsed/(double)BILLION;
	   printf("time for C(%d,%d) = A(%d,%d) B(%d,%d) is %llu ns = %fs \n", dim1, dim3, dim1, dim2, dim2, dim3, (long long unsigned int)timeElapsed, timesec);	
           if(NO_WRITE) {
           printf("Writing M1*M2 to file M1M2.dat\n");

           fp = fopen("M1M2.dat", "w");
           if (fp == NULL) {
             printf("I couldn't open M1M2.dat for writing.\n");
             exit(0);
           }
           for (i = 0; i < dim2; i++)
                for (j = 0; j < dim3; j++)
                        fprintf(fp, "%f\t", C[i][j]);
           fclose(fp);
           }



        /* Compute C = A B with magma */
      printf("Compute with MAGMA\n");
      printf("Array initialization\n");
      for (i = 0; i < dim1; i++)
         for (j = 0; j < dim2; j++) 
            Anew[i + j * dim1] = A[j][i];
      for (i = 0; i < dim2; i++)
         for (j = 0; j < dim3; j++)
            Bnew[i + j * dim2] = B[j][i];
      for (i = 0; i < dim1; i++)
         for (j = 0; j < dim3; j++)
            //Cnew[i + j * dim1] = C[j][i];
            Cnew[i + j * dim1] = 0.;

      printf("Start computation..\n");
      clock_gettime(CLOCK_MONOTONIC, &start);
      //cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, dim1, dim3, dim2, alpha, Anew, dim1, Bnew, dim2, beta, Cnew, dim1);
      magmablas_dgemm(ch, ch, dim1, dim3, dim2, alpha, Anew, dim1, Bnew, dim2, beta, Cnew, dim1);
      clock_gettime(CLOCK_MONOTONIC, &end);
      timeElapsed = timespecDiff(&end, &start);
      timesec = (double)timeElapsed/(double)BILLION;
      printf("Done.\n");
      printf("time for C(%d,%d) = A(%d,%d) B(%d,%d) with DGEMM is %llu ns = %fs \n", dim1, dim3, dim1, dim2, dim2, dim3, (long long unsigned int)timeElapsed, timesec);
      
      if(NO_WRITE) {
        printf("Writing M1*M2 to file M1M2_MAGMA.dat\n");
        fp = fopen("M1M2_MAGMA.dat", "w");
        if (fp == NULL) {
           printf("I couldn't open M1M2_MAGMA.dat for writing.\n");
           exit(0);
        }
        for (i = 0; i < dim1; i++)
           for (j = 0; j < dim3; j++)
             fprintf(fp, "%f\t", Cnew[i + j * dim1]);
        fclose(fp);
      }


	return 0;
}

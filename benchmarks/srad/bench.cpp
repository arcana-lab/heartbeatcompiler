#include "bench.hpp"
#include <cstdint>
#include <cstdlib>
#include <cmath>
#if !defined(USE_HB_MANUAL) && !defined(USE_HB_COMPILER)
#include "utility.hpp"
#include <functional>
#include <taskparts/benchmark.hpp>
#endif

namespace srad {

#if defined(INPUT_BENCHMARKING)
  int rows=10000;
  int cols=10000;
#elif defined(INPUT_TPAL)
  int rows=4000;
  int cols=4000;
#elif defined(INPUT_TESTING)
  int rows=4000;
  int cols=4000;
#elif defined(INPUT_USER)
  int rows;
  int cols;
#else
  #error "Need to select input size, e.g., INPUT_{BENCHMARKING, TPAL, TESTING, USER}"
#endif
int size_I, size_R;
float *I, *J, q0sqr, sum, sum2, tmp, meanROI,varROI ;
int *iN, *iS, *jE, *jW;
float *dN, *dS, *dW, *dE;
int r1, r2, c1, c2;
float *c, D;
float lambda;
#if defined(TEST_CORRECTNESS)
float *J_ref;
#endif

#if !defined(USE_HB_MANUAL) && !defined(USE_HB_COMPILER)
void run_bench(std::function<void()> const &bench_body,
               std::function<void()> const &bench_start,
               std::function<void()> const &bench_end) {
#if defined(USE_BASELINE)
  taskparts::benchmark_nativeforkjoin([&] (auto sched) {
    bench_body();
  }, [&] (auto sched) {
    bench_start();
  }, [&] (auto sched) {
    bench_end();
  });
#else
  utility::run([&] {
    bench_body();
  }, [&] {
    bench_start();
  }, [&] {
    bench_end();
  });
#endif
}
#endif

void random_matrix(float *I, int rows, int cols){
  srand(7);
	for( int i = 0 ; i < rows ; i++){
		for ( int j = 0 ; j < cols ; j++){
		  I[i * cols + j] = rand()/(float)RAND_MAX ;
		}
	}
}

void setup() {
  r1   = 0;
  r2   = 31;
  c1   = 0;
  c2   = 31;
  lambda = 0.5;

  size_I = cols * rows;
  size_R = (r2-r1+1)*(c2-c1+1);   

  I = (float *)malloc( size_I * sizeof(float) );
  J = (float *)malloc( size_I * sizeof(float) );
#if defined(TEST_CORRECTNESS)
  J_ref = (float *)malloc( size_I * sizeof(float) );
#endif
  c  = (float *)malloc(sizeof(float)* size_I) ;

  iN = (int *)malloc(sizeof(unsigned int*) * rows) ;
  iS = (int *)malloc(sizeof(unsigned int*) * rows) ;
  jW = (int *)malloc(sizeof(unsigned int*) * cols) ;
  jE = (int *)malloc(sizeof(unsigned int*) * cols) ;    


  dN = (float *)malloc(sizeof(float)* size_I) ;
  dS = (float *)malloc(sizeof(float)* size_I) ;
  dW = (float *)malloc(sizeof(float)* size_I) ;
  dE = (float *)malloc(sizeof(float)* size_I) ;    

  for (int i=0; i< rows; i++) {
    iN[i] = i-1;
    iS[i] = i+1;
  }    
  for (int j=0; j< cols; j++) {
    jW[j] = j-1;
    jE[j] = j+1;
  }
  iN[0]    = 0;
  iS[rows-1] = rows-1;
  jW[0]    = 0;
  jE[cols-1] = cols-1;

  random_matrix(I, rows, cols);

  for (int k = 0;  k < size_I; k++ ) {
    J[k] = (float)exp((double)I[k]) ;
#if defined(TEST_CORRECTNESS)
    J_ref[k] = J[k];
#endif
  }

  sum=0; sum2=0;     
  for (int i=r1; i<=r2; i++) {
    for (int j=c1; j<=c2; j++) {
      tmp   = J[i * cols + j];
      sum  += tmp ;
      sum2 += tmp*tmp;
    }
  }
  meanROI = sum / size_R;
  varROI  = (sum2 / size_R) - meanROI*meanROI;
  q0sqr   = varROI / (meanROI*meanROI);
}

void finishup() {
  free(I);
  free(J);
  free(iN); free(iS); free(jW); free(jE);
  free(dN); free(dS); free(dW); free(dE);
  free(c);
#if defined(TEST_CORRECTNESS)
  free(J_ref);
#endif
}

#if defined(USE_BASELINE) || defined(TEST_CORRECTNESS)

void srad_serial(int rows, int cols, int size_I, int size_R, float* I, float* J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float* c, int* iN, int* iS, int* jE, int* jW, float lambda) {
  for (int i = 0 ; i < rows ; i++) {
    for (int j = 0; j < cols; j++) { 
		
      int k = i * cols + j;
      float Jc = J[k];
 
      // directional derivates
      dN[k] = J[iN[i] * cols + j] - Jc;
      dS[k] = J[iS[i] * cols + j] - Jc;
      dW[k] = J[i * cols + jW[j]] - Jc;
      dE[k] = J[i * cols + jE[j]] - Jc;
			
      float G2 = (dN[k]*dN[k] + dS[k]*dS[k] 
		  + dW[k]*dW[k] + dE[k]*dE[k]) / (Jc*Jc);

      float L = (dN[k] + dS[k] + dW[k] + dE[k]) / Jc;

      float num  = (0.5*G2) - ((1.0/16.0)*(L*L)) ;
      float den  = 1 + (.25*L);
      float qsqr = num/(den*den);
 
      // diffusion coefficent (equ 33)
      den = (qsqr-q0sqr) / (q0sqr * (1+q0sqr)) ;
      c[k] = 1.0 / (1.0+den) ;
                
      // saturate diffusion coefficent
      if (c[k] < 0) {c[k] = 0;}
      else if (c[k] > 1) {c[k] = 1;}
   
    }
  
  }
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {        

      // current index
      int k = i * cols + j;
                
      // diffusion coefficent
      float cN = c[k];
      float cS = c[iS[i] * cols + j];
      float cW = c[k];
      float cE = c[i * cols + jE[j]];

      // divergence (equ 58)
      float D = cN * dN[k] + cS * dS[k] + cW * dW[k] + cE * dE[k];
                
      // image update (equ 61)
      J[k] = J[k] + 0.25*lambda*D;
    }
  }
}

#endif

#if defined(USE_OPENCILK)

#include <cilk/cilk.h>

void srad_opencilk(int rows, int cols, int size_I, int size_R, float* I, float* J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float* c, int* iN, int* iS, int* jE, int* jW, float lambda) {
  cilk_for (int i = 0 ; i < rows ; i++) {
    cilk_for (int j = 0; j < cols; j++) { 
		
      int k = i * cols + j;
      float Jc = J[k];
 
      // directional derivates
      dN[k] = J[iN[i] * cols + j] - Jc;
      dS[k] = J[iS[i] * cols + j] - Jc;
      dW[k] = J[i * cols + jW[j]] - Jc;
      dE[k] = J[i * cols + jE[j]] - Jc;
			
      float G2 = (dN[k]*dN[k] + dS[k]*dS[k] 
		  + dW[k]*dW[k] + dE[k]*dE[k]) / (Jc*Jc);

      float L = (dN[k] + dS[k] + dW[k] + dE[k]) / Jc;

      float num  = (0.5*G2) - ((1.0/16.0)*(L*L)) ;
      float den  = 1 + (.25*L);
      float qsqr = num/(den*den);
 
      // diffusion coefficent (equ 33)
      den = (qsqr-q0sqr) / (q0sqr * (1+q0sqr)) ;
      c[k] = 1.0 / (1.0+den) ;
                
      // saturate diffusion coefficent
      if (c[k] < 0) {c[k] = 0;}
      else if (c[k] > 1) {c[k] = 1;}
   
    }
  
  }
  cilk_for (int i = 0; i < rows; i++) {
    cilk_for (int j = 0; j < cols; j++) {        

      // current index
      int k = i * cols + j;
                
      // diffusion coefficent
      float cN = c[k];
      float cS = c[iS[i] * cols + j];
      float cW = c[k];
      float cE = c[i * cols + jE[j]];

      // divergence (equ 58)
      float D = cN * dN[k] + cS * dS[k] + cW * dW[k] + cE * dE[k];
                
      // image update (equ 61)
      J[k] = J[k] + 0.25*lambda*D;
    }
  }
}

#elif defined(USE_CILKPLUS)

#include <cilk/cilk.h>

void srad_cilkplus(int rows, int cols, int size_I, int size_R, float* I, float* J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float* c, int* iN, int* iS, int* jE, int* jW, float lambda) {
  cilk_for (int i = 0 ; i < rows ; i++) {
    cilk_for (int j = 0; j < cols; j++) { 
		
      int k = i * cols + j;
      float Jc = J[k];
 
      // directional derivates
      dN[k] = J[iN[i] * cols + j] - Jc;
      dS[k] = J[iS[i] * cols + j] - Jc;
      dW[k] = J[i * cols + jW[j]] - Jc;
      dE[k] = J[i * cols + jE[j]] - Jc;
			
      float G2 = (dN[k]*dN[k] + dS[k]*dS[k] 
		  + dW[k]*dW[k] + dE[k]*dE[k]) / (Jc*Jc);

      float L = (dN[k] + dS[k] + dW[k] + dE[k]) / Jc;

      float num  = (0.5*G2) - ((1.0/16.0)*(L*L)) ;
      float den  = 1 + (.25*L);
      float qsqr = num/(den*den);
 
      // diffusion coefficent (equ 33)
      den = (qsqr-q0sqr) / (q0sqr * (1+q0sqr)) ;
      c[k] = 1.0 / (1.0+den) ;
                
      // saturate diffusion coefficent
      if (c[k] < 0) {c[k] = 0;}
      else if (c[k] > 1) {c[k] = 1;}
   
    }
  
  }
  cilk_for (int i = 0; i < rows; i++) {
    cilk_for (int j = 0; j < cols; j++) {        

      // current index
      int k = i * cols + j;
                
      // diffusion coefficent
      float cN = c[k];
      float cS = c[iS[i] * cols + j];
      float cW = c[k];
      float cE = c[i * cols + jE[j]];

      // divergence (equ 58)
      float D = cN * dN[k] + cS * dS[k] + cW * dW[k] + cE * dE[k];
                
      // image update (equ 61)
      J[k] = J[k] + 0.25*lambda*D;
    }
  }
}

#elif defined(USE_OPENMP)

#include <omp.h>

void srad_openmp(int rows, int cols, int size_I, int size_R, float* I, float* J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float* c, int* iN, int* iS, int* jE, int* jW, float lambda) {
#if defined(OMP_NESTED_SCHEDULING)
  omp_set_max_active_levels(2);
#endif
#if defined(OMP_SCHEDULE_STATIC)
  #pragma omp parallel for schedule(static)
#elif defined(OMP_SCHEDULE_DYNAMIC)
  #pragma omp parallel for schedule(dynamic)
#elif defined(OMP_SCHEDULE_GUIDED)
  #pragma omp parallel for schedule(guided)
#endif
  for (int i = 0 ; i < rows ; i++) {
#if defined(OMP_NESTED_SCHEDULING)
#if defined(OMP_SCHEDULE_STATIC)
    #pragma omp parallel for schedule(static)
#elif defined(OMP_SCHEDULE_DYNAMIC)
    #pragma omp parallel for schedule(dynamic)
#elif defined(OMP_SCHEDULE_GUIDED)
    #pragma omp parallel for schedule(guided)
#endif
#endif
    for (int j = 0; j < cols; j++) { 
		
      int k = i * cols + j;
      float Jc = J[k];
 
      // directional derivates
      dN[k] = J[iN[i] * cols + j] - Jc;
      dS[k] = J[iS[i] * cols + j] - Jc;
      dW[k] = J[i * cols + jW[j]] - Jc;
      dE[k] = J[i * cols + jE[j]] - Jc;
			
      float G2 = (dN[k]*dN[k] + dS[k]*dS[k] 
		  + dW[k]*dW[k] + dE[k]*dE[k]) / (Jc*Jc);

      float L = (dN[k] + dS[k] + dW[k] + dE[k]) / Jc;

      float num  = (0.5*G2) - ((1.0/16.0)*(L*L)) ;
      float den  = 1 + (.25*L);
      float qsqr = num/(den*den);
 
      // diffusion coefficent (equ 33)
      den = (qsqr-q0sqr) / (q0sqr * (1+q0sqr)) ;
      c[k] = 1.0 / (1.0+den) ;
                
      // saturate diffusion coefficent
      if (c[k] < 0) {c[k] = 0;}
      else if (c[k] > 1) {c[k] = 1;}
   
    }
  
  }
#if defined(OMP_NESTED_SCHEDULING)
  omp_set_max_active_levels(2);
#endif
#if defined(OMP_SCHEDULE_STATIC)
  #pragma omp parallel for schedule(static)
#elif defined(OMP_SCHEDULE_DYNAMIC)
  #pragma omp parallel for schedule(dynamic)
#elif defined(OMP_SCHEDULE_GUIDED)
  #pragma omp parallel for schedule(guided)
#endif
  for (int i = 0; i < rows; i++) {
#if defined(OMP_NESTED_SCHEDULING)
#if defined(OMP_SCHEDULE_STATIC)
    #pragma omp parallel for schedule(static)
#elif defined(OMP_SCHEDULE_DYNAMIC)
    #pragma omp parallel for schedule(dynamic)
#elif defined(OMP_SCHEDULE_GUIDED)
    #pragma omp parallel for schedule(guided)
#endif
#endif
    for (int j = 0; j < cols; j++) {        

      // current index
      int k = i * cols + j;
                
      // diffusion coefficent
      float cN = c[k];
      float cS = c[iS[i] * cols + j];
      float cW = c[k];
      float cE = c[i * cols + jE[j]];

      // divergence (equ 58)
      float D = cN * dN[k] + cS * dS[k] + cW * dW[k] + cE * dE[k];
                
      // image update (equ 61)
      J[k] = J[k] + 0.25*lambda*D;
    }
  }
}

#endif

#if defined(TEST_CORRECTNESS)

#include <stdio.h>

void test_correctness() {
  srad_serial(rows, cols, size_I, size_R, I, J_ref, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
  uint64_t num_diffs = 0;
  double epsilon = 0.01;
  for (uint64_t i = 0; i != size_I; i++) {
    auto diff = std::abs(J[i] - J_ref[i]);
    if (diff > epsilon) {
      // printf("diff=%f J[i]=%f J_ref[i]=%f at i=%ld\n", diff, J[i], J_ref[i], i);
      num_diffs++;
    }
  }
  if (num_diffs > 0) {
    printf("\033[0;31mINCORRECT!\033[0m");
    printf("  num_diffs = %lu\n", num_diffs);
  } else {
    printf("\033[0;32mCORRECT!\033[0m\n");
  }
  // printf("num_diffs %ld\n", num_diffs);
}

#endif

} // namespace srad

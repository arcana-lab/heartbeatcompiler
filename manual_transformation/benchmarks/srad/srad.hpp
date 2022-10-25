#include <cstdint>
#include <cstdlib>
#include <cmath>
#if defined(USE_OPENCILK)
#include <cilk/cilk.h>
#endif
#if defined(USE_OPENMP)
#include <omp.h>
#endif
#if defined(TEST_CORRECTNESS)
#include <cstdio>
#endif

namespace srad {

int rows=4000;
int cols=4000, size_I, size_R;
float *I, *J, q0sqr, sum, sum2, tmp, meanROI,varROI ;
int *iN, *iS, *jE, *jW;
float *dN, *dS, *dW, *dE;
int r1, r2, c1, c2;
float *c, D;
float lambda;

uint64_t hash64(uint64_t u) {
  uint64_t v = u * 3935559000370003845ul + 2691343689449507681ul;
  v ^= v >> 21;
  v ^= v << 37;
  v ^= v >>  4;
  v *= 4768777513237032717ul;
  v ^= v << 20;
  v ^= v >> 41;
  v ^= v <<  5;
  return v;
}

void random_matrix(float *I, int rows, int cols){
  for( int i = 0 ; i < rows ; i++) {
    for ( int j = 0 ; j < cols ; j++) {
      I[i * cols + j] = (float)hash64(i+j)/(float)RAND_MAX ;
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
}

#if defined(USE_OPENCILK)
void srad_opencilk(int rows, int cols, int size_I, int size_R, float *I, float *J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float *c, int *iN, int *iS, int *jE, int *jW, float lambda) {
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
void srad_openmp(int rows, int cols, int size_I, int size_R, float *I, float *J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float *c, int *iN, int *iS, int *jE, int *jW, float lambda) {
#if defined(OMP_DYNAMIC)
  #pragma omp parallel for schedule(dynamic)
#elif defined(OMP_GUIDED) 
  #pragma omp parallel for schedule(guided)
#else  
  #pragma omp parallel for
#endif
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
  #pragma omp parallel for
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
#else
void srad_serial(int rows, int cols, int size_I, int size_R, float *I, float *J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float *c, int *iN, int *iS, int *jE, int *jW, float lambda) {
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

}
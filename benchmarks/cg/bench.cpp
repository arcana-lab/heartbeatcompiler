#include "bench.hpp"
#include <cstdlib>
#include <cstdint>
#include <cassert>
#include <mm/mm.hpp> // matrix market loader: https://github.com/cwpearson/matrix-market
#if !defined(USE_HB_MANUAL) && !defined(USE_HB_COMPILER)
#include "utility.hpp"
#include <functional>
#endif

// benchmark adapted from https://github.com/benchmark-subsetting/NPB3.0-omp-C/blob/master/CG/cg.c
namespace cg {

uint64_t nb_vals;
uint64_t n;
uint64_t *col_ind;
uint64_t *row_ptr;
scalar *x;
scalar *z;
scalar *val;
scalar *p;
scalar *q;
scalar *r;
scalar rnorm;

#if !defined(USE_HB_MANUAL) && !defined(USE_HB_COMPILER)
void run_bench(std::function<void()> const &bench_body,
               std::function<void()> const &bench_start,
               std::function<void()> const &bench_end) {
  utility::run([&] {
    bench_body();
  }, [&] {
    bench_start();
  }, [&] {
    bench_end();
  });
}
#endif

inline
uint64_t hash(uint64_t u) {
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

auto rand_float(size_t i) -> scalar {
  int m = 1000000;
  scalar v = hash(i) % m;
  return v / m;
}

template <typename T>
void zero_init(T* a, std::size_t n) {
  volatile T* b = (volatile T*)a;
  for (std::size_t i = 0; i < n; i++) {
    b[i] = 0;
  }
}

void setup() {
  std::string fname = "Trefethen_20/Trefethen_20.mtx";
  if (const auto env_p = std::getenv("MATRIX_MARKET_FILE")) {
    fname = env_p;
  }
  typedef size_t Offset;
  typedef uint64_t Ordinal;
  typedef scalar Scalar;
  typedef MtxReader<Ordinal, Scalar> reader_t;
  typedef typename reader_t::coo_type coo_t;
  typedef typename coo_t::entry_type entry_t;
  typedef CSR<Ordinal, Scalar, Offset> csr_type;

  // read matrix as coo
  reader_t reader(fname);
  coo_t coo = reader.read_coo();
  csr_type csr(coo);
  nb_vals = csr.nnz();
  val = (scalar*)malloc(sizeof(scalar) * nb_vals);
  std::copy(csr.val().begin(), csr.val().end(), val);
  auto nb_rows = csr.num_rows();
  auto nb_cols = csr.num_cols();
  assert(nb_rows == nb_cols && "input matrix must be a symmetric positive definite floating point matrix");
  n = nb_rows;
  row_ptr = (Ordinal*)malloc(sizeof(Ordinal) * (n + 1));
  std::copy(csr.row_ptr().begin(), csr.row_ptr().end(), row_ptr);
  col_ind = (Offset*)malloc(sizeof(Offset) * nb_vals);
  std::copy(&csr.col_ind()[0], (&csr.col_ind()[nb_cols-1]) + 1, col_ind);

  x = (scalar*)malloc(sizeof(scalar) * n);
  z = (scalar*)malloc(sizeof(scalar) * n);
  p = (scalar*)malloc(sizeof(scalar) * n);
  q = (scalar*)malloc(sizeof(scalar) * n);
  r = (scalar*)malloc(sizeof(scalar) * n);

  if ((val == nullptr) || (row_ptr == nullptr) || (col_ind == nullptr) || (x == nullptr) || (z == nullptr)) {
    exit(1);
  }
/*--------------------------------------------------------------------
c  set starting vector to (1, 1, .... 1)
c-------------------------------------------------------------------*/
  for (size_t i = 0; i < n; i++) {
    x[i] = 1.0;
  }
  zero_init(z, n);
  zero_init(p, n);
  zero_init(q, n);
  zero_init(r, n);

#if defined(STATS)
  printf("nb_rows=%lu n_nonzeros=%lu\n", n, nb_vals);
#endif
}

void finishup() {
  free(val);
  free(row_ptr);
  free(col_ind);
  free(x);
  free(z);
  free(p);
  free(q);
  free(r);
}


#if defined(USE_BASELINE) || defined(TEST_CORRECTNESS)

scalar dotp(uint64_t n, scalar* r, scalar* q) {
  scalar sum = 0.0;
  // parallel for reduction(+:sum)
  for (uint64_t j = 0; j < n; j++) {
    sum += r[j] * q[j];
  }
  return sum;
}

scalar norm(uint64_t n, scalar* r) {
  return dotp(n, r, r);
}

void spmv_serial(
  scalar* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  scalar* x,
  scalar* y,
  uint64_t n) {
  // parallel for
  for (uint64_t i = 0; i < n; i++) { // row loop
    scalar r = 0.0;
    // parallel for reduction(+:r)
    for (uint64_t k = row_ptr[i]; k < row_ptr[i + 1]; k++) { // col loop
      r += val[k] * x[col_ind[k]];
    }
    y[i] = r;
  }
}

scalar conj_grad_serial(
    uint64_t n,
    uint64_t* col_ind,
    uint64_t* row_ptr,
    scalar* x,
    scalar* z,
    scalar* a,
    scalar* p,
    scalar* q,
    scalar* r) {
  int cgitmax = 25;

/*--------------------------------------------------------------------
c  Initialize the CG algorithm:
c-------------------------------------------------------------------*/
  // parallel for
  for (uint64_t j = 0; j < n; j++) {
    q[j] = 0.0;
    z[j] = 0.0;
    r[j] = x[j];
    p[j] = r[j];
  }

/*--------------------------------------------------------------------
c  rho = r.r
c  Now, obtain the norm of r: First, sum squares of r elements locally...
c-------------------------------------------------------------------*/
  scalar rho = norm(n, r);

/*--------------------------------------------------------------------
c---->
c  The conj grad iteration loop
c---->
c-------------------------------------------------------------------*/
  for (uint64_t cgit = 0; cgit < cgitmax; cgit++) {
    scalar rho0 = rho;
    rho = 0.0;

/*--------------------------------------------------------------------
c  q = A.p
c-------------------------------------------------------------------*/
    spmv_serial(a, row_ptr, col_ind, p, q, n);

/*--------------------------------------------------------------------
c  Obtain alpha = rho / (p.q)
c-------------------------------------------------------------------*/
    scalar alpha = rho0 / dotp(n, p, q);

/*---------------------------------------------------------------------
c  Obtain z = z + alpha*p
c  and    r = r - alpha*q
c---------------------------------------------------------------------*/
    // parallel for reduction(:+rho)
    for (uint64_t j = 0; j < n; j++) {
      z[j] = z[j] + alpha * p[j];
      r[j] = r[j] - alpha * q[j];
      rho += r[j] * r[j];
    }
    scalar beta = rho / rho0;

/*--------------------------------------------------------------------
c  p = r + beta*p
c-------------------------------------------------------------------*/
    // parallel for
    for (uint64_t j = 0; j < n; j++) {
      p[j] = r[j] + beta * p[j];
    }
  }

  spmv_serial(a, row_ptr, col_ind, z, r, n);

/*--------------------------------------------------------------------
c  At this point, r contains A.z
c-------------------------------------------------------------------*/
  scalar sum = 0.0;
  // parallel for reduction(+:sum)
  for (uint64_t j = 0; j < n; j++) {
    scalar d = x[j] - r[j];
    sum += d * d;
  }
  return sqrt(sum);
}

#endif

#if defined(USE_OPENCILK)

#include <cilk/cilk.h>

int nworkers = -1;
auto ceiling_div(uint64_t x, uint64_t y) -> uint64_t {
  return (x + y - 1) / y;
}
auto get_grainsize(uint64_t n) -> uint64_t {
  return std::min<uint64_t>(2048, ceiling_div(n, 8 * nworkers));
}

auto sparse_dotp_rec(int64_t lo, int64_t hi, scalar* val, scalar* x, uint64_t* col_ind,
		     uint64_t grainsize) -> scalar {
  auto n = hi - lo;
  if (n <= grainsize) {
    scalar t = 0.0;
    for (int64_t k = lo; k < hi; k++) { // col loop
      t += val[k] * x[col_ind[k]];
    }
    return t;
  }
  auto mid = (lo + hi) / 2;
  scalar r1, r2;
  r1 = cilk_spawn sparse_dotp_rec(lo, mid, val, x, col_ind, grainsize);
  r2 = sparse_dotp_rec(mid, hi, val, x, col_ind, grainsize);
  cilk_sync;
  return r1 + r2;
}

auto sparse_dotp(int64_t lo, int64_t hi, scalar* val, scalar* x, uint64_t* col_ind) -> scalar {
  return sparse_dotp_rec(lo, hi, val, x, col_ind, get_grainsize(hi - lo));
}

void spmv_cilk(scalar* val,
               uint64_t* row_ptr,
               uint64_t* col_ind,
               scalar* x,
               scalar* y,
               int64_t n) {
  cilk_for (int64_t i = 0; i < n; i++) {  // row loop
    y[i] = sparse_dotp(row_ptr[i], row_ptr[i+1], val, x, col_ind);
  }
}

auto dotp_rec(int64_t lo, int64_t hi, scalar* r, scalar* q, uint64_t grainsize) -> scalar {
  auto n = hi - lo;
  if (n <= grainsize) {
    scalar sum = 0.0;
    for (uint64_t j = lo; j < hi; j++) {
      sum += r[j] * q[j];
    }
    return sum;
  }
  auto mid = (lo + hi) / 2;
  scalar r1, r2;
  r1 = cilk_spawn dotp_rec(lo, mid, r, q, grainsize);
  r2 = dotp_rec(mid, hi, r, q, grainsize);
  cilk_sync;
  return r1 + r2;
}

scalar dotp(uint64_t n, scalar* r, scalar* q) {
  return dotp_rec(0, n, r, q, get_grainsize(n));
}

scalar norm(uint64_t n, scalar* r) {
  return dotp(n, r, r);
}

auto loop1_rec(int64_t lo, int64_t hi, scalar* z, scalar* p, scalar* q, scalar* r, scalar alpha, uint64_t grainsize) -> scalar {
  auto n = hi - lo;
  if (n <= grainsize) {
    scalar sum = 0.0;
    for (uint64_t j = 0; j < n; j++) {
      z[j] = z[j] + alpha * p[j];
      r[j] = r[j] - alpha * q[j];
      sum += r[j] * r[j];
    }
    return sum;
  }
  auto mid = (lo + hi) / 2;
  scalar r1, r2;
  r1 = cilk_spawn loop1_rec(lo, mid, z, p, q, r, alpha, grainsize);
  r2 = loop1_rec(mid, hi, z, p, q, r, alpha, grainsize);
  cilk_sync;
  return r1 + r2;
}

auto loop1(int64_t n, scalar* z, scalar* p, scalar* q, scalar* r, scalar alpha) -> scalar {
  return loop1_rec(0, n, z, p, q, r, alpha, get_grainsize(n));
}
  
auto loop2_rec(int64_t lo, int64_t hi, scalar* x, scalar* r, uint64_t grainsize) -> scalar {
  auto n = hi - lo;
  if (n <= grainsize) {
    scalar sum = 0.0;
    for (uint64_t j = lo; j < hi; j++) {
      scalar d = x[j] - r[j];
      sum += d * d;
    }
    return sum;
  }
  auto mid = (lo + hi) / 2;
  scalar r1, r2;
  r1 = cilk_spawn loop2_rec(lo, mid, x, r, grainsize);
  r2 = loop2_rec(mid, hi, x, r, grainsize);
  cilk_sync;
  return r1 + r2;
}

auto loop2(uint64_t n, scalar* x, scalar* r) -> scalar {
  return loop2_rec(0, n, x, r, get_grainsize(n));
}

scalar conj_grad_opencilk(
    uint64_t n,
    uint64_t* col_ind,
    uint64_t* row_ptr,
    scalar* x,
    scalar* z,
    scalar* a,
    scalar* p,
    scalar* q,
    scalar* r) {
  cilk_for (uint64_t j = 0; j < n; j++) {
    q[j] = 0.0;
    z[j] = 0.0;
    r[j] = x[j];
    p[j] = r[j];
  }
  scalar rho = norm(n, r);
  for (uint64_t cgit = 0; cgit < cgitmax; cgit++) {
    scalar rho0 = rho;
    rho = 0.0;
    spmv_cilk(a, row_ptr, col_ind, p, q, n);
    scalar alpha = rho0 / dotp(n, p, q);
    rho += loop1(n, z, p, q, r, alpha);
    scalar beta = rho / rho0;
    cilk_for (uint64_t j = 0; j < n; j++) {
      p[j] = r[j] + beta * p[j];
    }
  }
  spmv_cilk(a, row_ptr, col_ind, z, r, n);
  return sqrt(loop2(n, x, r));
}

#elif defined(USE_OPENMP)

#include <omp.h>

scalar dotp_openmp(uint64_t n, scalar* r, scalar* q) {
  scalar sum = 0.0;
#pragma omp parallel default(shared)
{
  #pragma omp for reduction(+:sum)
  for (uint64_t j = 0; j < n; j++) {
    sum += r[j] * q[j];
  }
}
  return sum;
}

scalar norm_openmp(uint64_t n, scalar* r) {
  return dotp_openmp(n, r, r);
}

void spmv_openmp(
  scalar* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  scalar* x,
  scalar* y,
  uint64_t n) {
  #pragma omp for
  for (uint64_t i = 0; i < n; i++) { // row loop
    scalar r = 0.0;
    for (uint64_t k = row_ptr[i]; k < row_ptr[i + 1]; k++) { // col loop
      r += val[k] * x[col_ind[k]];
    }
    y[i] = r;
  }
}

scalar conj_grad_openmp(
  uint64_t n,
  uint64_t* col_ind,
  uint64_t* row_ptr,
  scalar* x,
  scalar* z,
  scalar* a,
  scalar* p,
  scalar* q,
  scalar* r) {
  int cgitmax = 25;

/*--------------------------------------------------------------------
c  Initialize the CG algorithm:
c-------------------------------------------------------------------*/
  #pragma omp for
  for (uint64_t j = 0; j < n; j++) {
    q[j] = 0.0;
    z[j] = 0.0;
    r[j] = x[j];
    p[j] = r[j];
  }
/*--------------------------------------------------------------------
c  rho = r.r
c  Now, obtain the norm of r: First, sum squares of r elements locally...
c-------------------------------------------------------------------*/
  scalar rho = norm_openmp(n, r);
/*--------------------------------------------------------------------
c---->
c  The conj grad iteration loop
c---->
c-------------------------------------------------------------------*/
  for (uint64_t cgit = 0; cgit < cgitmax; cgit++) {
    scalar rho0 = rho;
    rho = 0.0;
/*--------------------------------------------------------------------
c  q = A.p
c-------------------------------------------------------------------*/
    spmv_openmp(a, row_ptr, col_ind, p, q, n);
/*--------------------------------------------------------------------
c  Obtain alpha = rho / (p.q)
c-------------------------------------------------------------------*/
    scalar alpha = rho0 / dotp_openmp(n, p, q);
/*---------------------------------------------------------------------
c  Obtain z = z + alpha*p
c  and    r = r - alpha*q
c---------------------------------------------------------------------*/
#pragma omp parallel default(shared)
{
    #pragma omp for reduction(+:rho)
    for (uint64_t j = 0; j < n; j++) {
      z[j] = z[j] + alpha * p[j];
      r[j] = r[j] - alpha * q[j];
      rho += r[j] * r[j];
    }
    scalar beta = rho / rho0;
/*--------------------------------------------------------------------
c  p = r + beta*p
c-------------------------------------------------------------------*/
    #pragma omp for
    for (uint64_t j = 0; j < n; j++) {
      p[j] = r[j] + beta * p[j];
    }
}
  }

  spmv_openmp(a, row_ptr, col_ind, z, r, n);

/*--------------------------------------------------------------------
c  At this point, r contains A.z
c-------------------------------------------------------------------*/
  scalar sum = 0.0;
#pragma omp parallel default(shared)
{
  #pragma omp for reduction(+:sum)
  for (uint64_t j = 0; j < n; j++) {
    scalar d = x[j] - r[j];
    sum += d * d;
  }
}

  return sqrt(sum);
}

#endif

#if defined(TEST_CORRECTNESS)

#include <stdio.h>

void test_correctness() {
  scalar *x_ref = (scalar*)malloc(sizeof(scalar) * n);
  scalar *z_ref = (scalar*)malloc(sizeof(scalar) * n);
  scalar *p_ref = (scalar*)malloc(sizeof(scalar) * n);
  scalar *q_ref = (scalar*)malloc(sizeof(scalar) * n);
  scalar *r_ref = (scalar*)malloc(sizeof(scalar) * n);

  if ((x_ref == nullptr) || (z_ref == nullptr)) {
    exit(1);
  }
  for (size_t i = 0; i < n; i++) {
    x_ref[i] = 1.0;
  }
  zero_init(z_ref, n);
  zero_init(p_ref, n);
  zero_init(q_ref, n);
  zero_init(r_ref, n);

  scalar rnorm_ref = conj_grad_serial(n, col_ind, row_ptr, x_ref, z_ref, val, p_ref, q_ref, r_ref);

  uint64_t num_diffs = 0;
  double epsilon = 0.01;
  auto diff = std::abs(rnorm - rnorm_ref);
  printf("diff=%f rnorm=%f rnorm_ref=%f\n", diff, rnorm, rnorm_ref);
  if (diff > epsilon) {
    printf("diff=%f rnorm=%f rnorm_ref=%f\n", diff, rnorm, rnorm_ref);
    num_diffs++;
  }

  if (num_diffs > 0) {
    printf("\033[0;31mINCORRECT!\033[0m");
    printf("  num_diffs = %lu\n", num_diffs);
  } else {
    printf("\033[0;32mCORRECT!\033[0m\n");
  }

  free(x_ref);
  free(z_ref);
  free(p_ref);
  free(q_ref);
  free(r_ref);
}

#endif

} // namespace cg

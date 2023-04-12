#include "bench.hpp"
// benchmark adapted from https://github.com/benchmark-subsetting/NPB3.0-omp-C/blob/master/CG/cg.c

namespace cg {

auto rand_float(size_t i) -> scalar {
  int m = 1000000;
  scalar v = taskparts::hash(i) % m;
  return v / m;
}

template <typename T>
void zero_init(T* a, std::size_t n) {
  volatile T* b = (volatile T*)a;
  for (std::size_t i = 0; i < n; i++) {
    b[i] = 0;
  }
}

auto setup() -> void {
  std::string fname = taskparts::cmdline::parse_or_default_string("fname", "1138_bus.mtx");
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
  if (nb_rows != nb_cols) {  // Houson, we have a problem...
    taskparts::die("input matrix must be *symmetric* positive definite");
  }
  n = nb_rows;
  row_ptr = (Ordinal*)malloc(sizeof(Ordinal) * (n + 1));
  std::copy(csr.row_ptr().begin(), csr.row_ptr().end(), row_ptr);
  col_ind = (Offset*)malloc(sizeof(Offset) * nb_vals);
  std::copy(&csr.col_ind()[0], (&csr.col_ind()[nb_cols-1]) + 1, col_ind);

  x = (scalar*)malloc(sizeof(scalar) * n);
  y = (scalar*)malloc(sizeof(scalar) * n);
  p = (scalar*)malloc(sizeof(scalar) * n);
  q = (scalar*)malloc(sizeof(scalar) * n);
  r = (scalar*)malloc(sizeof(scalar) * n);

  if ((val == nullptr) || (row_ptr == nullptr) || (col_ind == nullptr) || (x == nullptr) || (y == nullptr)) {
    exit(1);
  }
  // initialize x and y vectors
  {
    for (size_t i = 0; i < n; i++) {
      x[i] = rand_float(i);
    }
    zero_init(y, n);
    zero_init(p, n);
    zero_init(q, n);
    zero_init(r, n);
  }  

  printf("nbr=%lu nnz=%lu\n",n,nb_vals);
}

auto finishup() {
  free(val);
  free(row_ptr);
  free(col_ind);
  free(x);
  free(y);
  free(p);
  free(q);
  free(r);
}


#if defined(USE_BASELINE) || defined(TEST_CORRECTNESS)

scalar dotp(uint64_t n, scalar* r, scalar* q) {
  scalar sum = 0.0;
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
  for (uint64_t i = 0; i < n; i++) { // row loop
    scalar r = 0.0;
#pragma clang loop vectorize_width(4) interleave_count(4)
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
    scalar* r,
    uint64_t cgitmax) {
  for (uint64_t j = 0; j < n; j++) {
    q[j] = 0.0;
    z[j] = 0.0;
    r[j] = x[j];
    p[j] = r[j];
  }
  scalar rho = norm(n, r);
  for (uint64_t cgit = 0; cgit < cgitmax; cgit++) {
    scalar rho0 = rho;
    rho = 0.0;
    spmv_serial(a, row_ptr, col_ind, p, q, n);
    scalar alpha = rho0 / dotp(n, p, q);
    for (uint64_t j = 0; j < n; j++) {
      z[j] = z[j] + alpha * p[j];
      r[j] = r[j] - alpha * q[j];
      rho += r[j] * r[j];
    }
    scalar beta = rho / rho0;
    for (uint64_t j = 0; j < n; j++) {
      p[j] = r[j] + beta * p[j];
    }
  }
  spmv_serial(a, row_ptr, col_ind, z, r, n);
  scalar sum = 0.0;
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

scalar conj_grad_cilk(
    uint64_t n,
    uint64_t* col_ind,
    uint64_t* row_ptr,
    scalar* x,
    scalar* z,
    scalar* a,
    scalar* p,
    scalar* q,
    scalar* r,
    uint64_t cgitmax) {
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

#endif

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

#if defined(TEST_CORRECTNESS)

#include <stdio.h>

void test_correctness() {
}
  
#endif

} // namespace cg

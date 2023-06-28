#include "bench.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <vector>
#include <unistd.h>
#if !defined(USE_HB_MANUAL) && !defined(USE_HB_COMPILER)
#include "utility.hpp"
#include <functional>
#endif
#if defined(SPMV_MATRIX_MARKET)
#include <mm/mm.hpp> // matrix market loader: https://github.com/cwpearson/matrix-market
#endif

namespace spmv {

#if defined(INPUT_BENCHMARKING)
  #if defined(SPMV_RANDOM)
    uint64_t n_bigrows = 6000000;
    uint64_t degree_bigrows = 100;
  #elif defined(SPMV_POWERLAW)
    uint64_t n_bigcols = 24;
  #elif defined(SPMV_ARROWHEAD)
    uint64_t n_arrowhead = 150000000;
  #elif defined(SPMV_DENSE)
    uint64_t n_dense = 100000;
  #elif defined(SPMV_DIAGONAL)
    uint64_t n_diagonal = 5000000000;
  #elif defined(SPMV_NORMAL)
    uint64_t n_normal = 50000;
  #elif defined(SPMV_MATRIX_MARKET)
  // nothing to do
  #else
    #error "Need to select input class: SPMV_{RANDOM, POWERLAW, ARROWHEAD, DENSE, DIAGONAL, NORMAL, MATRIX_MARKET}"
  #endif
#elif defined(INPUT_TPAL)
  #if defined(SPMV_RANDOM)
    uint64_t n_bigrows = 3000000;
    uint64_t degree_bigrows = 100;
  #elif defined(SPMV_POWERLAW)
    uint64_t n_bigcols = 23;
  #elif defined(SPMV_ARROWHEAD)
    uint64_t n_arrowhead = 100000000;
  #elif defined(SPMV_MATRIX_MARKET)
  // nothing to do
  #else
    #error "Need to select input class: SPMV_{RANDOM, POWERLAW, ARROWHEAD, MATRIX_MARKET}"
  #endif
#elif defined(INPUT_TESTING)
  #if defined(SPMV_RANDOM)
    uint64_t n_bigrows = 300000;
    uint64_t degree_bigrows = 10;
  #elif defined(SPMV_POWERLAW)
    uint64_t n_bigcols = 22;
  #elif defined(SPMV_ARROWHEAD)
    uint64_t n_arrowhead = 10000000;
  #elif defined(SPMV_DENSE)
    uint64_t n_dense = 10000;
  #elif defined(SPMV_DIAGONAL)
    uint64_t n_diagonal = 50000000;
  #elif defined(SPMV_NORMAL)
    uint64_t n_normal = 400;
  #elif defined(SPMV_MATRIX_MARKET)
  // nothing to do
  #else
    #error "Need to select input class: SPMV_{RANDOM, POWERLAW, ARROWHEAD, DENSE, DIAGONAL, NORMAL, MATRIX_MARKET}"
  #endif
#else
  #error "Need to select input size, e.g., INPUT_{BENCHMARKING, TPAL, TESTING}"
#endif

uint64_t row_len = 1000;
uint64_t degree = 4;
uint64_t dim = 5;
uint64_t nb_rows;
uint64_t nb_vals;
double* val;
uint64_t* row_ptr;
uint64_t* col_ind;
double* x;
double* y;

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

auto rand_double(size_t i) -> double {
  int m = 1000000;
  double v = hash64(i) % m;
  return v / m;
}

using edge_type = std::pair<uint64_t, uint64_t>;

using edgelist_type = std::vector<edge_type>;

auto csr_of_edgelist(edgelist_type& edges) {
  std::sort(edges.begin(), edges.end(), std::less<edge_type>());
  {
    edgelist_type edges2;
    edge_type prev = edges.back();
    for (auto& e : edges) {
      if (e != prev) {
	      edges2.push_back(e);
      }
      prev = e;
    }
    edges2.swap(edges);
    edges2.clear();
  }
  nb_vals = edges.size();
  row_ptr = (uint64_t*)malloc(sizeof(uint64_t) * (nb_rows + 1));
  for (size_t i = 0; i < nb_rows + 1; i++) {
    row_ptr[i] = 0;
  }
  for (auto& e : edges) {
    assert((e.first >= 0) && (e.first < nb_rows));
    row_ptr[e.first]++;
  }
  size_t max_col_len = 0;
  size_t tot_col_len = 0;
  {
    for (size_t i = 0; i < nb_rows + 1; i++) {
      max_col_len = std::max(max_col_len, row_ptr[i]);
      tot_col_len += row_ptr[i];
    }
  }
  { // initialize column indices
    col_ind = (uint64_t*)malloc(sizeof(uint64_t) * nb_vals);
    uint64_t i = 0;
    for (auto& e : edges) {
      col_ind[i++] = e.second;
    }
  }
  edges.clear();
  { // initialize row pointers
    uint64_t a = 0;
    for (size_t i = 0; i < nb_rows; i++) {
      auto e = row_ptr[i];
      row_ptr[i] = a;
      a += e;
    }
    row_ptr[nb_rows] = a;
  }
  { // initialize nonzero values array
    val = (double*)malloc(sizeof(double) * nb_vals);
    for (size_t i = 0; i < nb_vals; i++) {
      val[i] = rand_double(i);
    }
  }
}

void read_matrix_from_file(const char *filename) {
  FILE *infile;

  infile = fopen(filename, "r");
  if (infile == NULL) {
    fprintf(stderr, "\nError opening file\n");
    exit(1);
  }

  fread(&nb_rows, sizeof(uint64_t), 1, infile);
  fread(&nb_vals, sizeof(uint64_t), 1, infile);

  row_ptr = (uint64_t*)malloc(sizeof(uint64_t) * (nb_rows + 1));
  col_ind = (uint64_t*)malloc(sizeof(uint64_t) * nb_vals);
  val = (double*)malloc(sizeof(double) * nb_vals);

  fread(row_ptr, sizeof(uint64_t), nb_rows + 1, infile);
  fread(col_ind, sizeof(uint64_t), nb_vals, infile);
  fread(val, sizeof(double), nb_vals, infile);

  fclose(infile);

  return;
}

void write_matrix_to_file(const char *filename) {
  FILE *outfile;

  outfile = fopen(filename, "w");
  if (outfile == NULL) {
    fprintf(stderr, "\nError opening file\n");
    exit(1);
  }

  fwrite(&nb_rows, sizeof(uint64_t), 1, outfile);
  fwrite(&nb_vals, sizeof(uint64_t), 1, outfile);
  fwrite(row_ptr, sizeof(uint64_t), nb_rows + 1, outfile);
  fwrite(col_ind, sizeof(uint64_t), nb_vals, outfile);
  fwrite(val, sizeof(double), nb_vals, outfile);

  fclose(outfile);

  return;
}

template <typename Gen_matrix>
auto bench_pre_shared(const Gen_matrix& gen_matrix) {
  gen_matrix();
  x = (double*)malloc(sizeof(double) * nb_rows);
  y = (double*)malloc(sizeof(double) * nb_rows);
  if ((val == nullptr) || (row_ptr == nullptr) || (col_ind == nullptr) || (x == nullptr) || (y == nullptr)) {
    exit(1);
  }
  // initialize x and y vectors
  {
    for (size_t i = 0; i < nb_rows; i++) {
      x[i] = rand_double(i);
      y[i] = 0.0;
    }
    // tpalrts::zero_init(y, nb_rows);
  }
};

#if defined(SPMV_RANDOM)

auto mk_random_local_edgelist(size_t dim, size_t degree, size_t num_rows)
  -> edgelist_type {
  size_t non_zeros = num_rows*degree;
  edgelist_type edges;
  for (size_t k = 0; k < non_zeros; k++) {
    size_t i = k / degree;
    size_t j;
    if (dim==0) {
      size_t h = k;
      do {
	      j = ((h = hash64(h)) % num_rows);
      } while (j == i);
    } else {
      size_t _pow = dim+2;
      size_t h = k;
      do {
        while ((((h = hash64(h)) % 1000003) < 500001)) _pow += dim;
        j = (i + ((h = hash64(h)) % (((long) 1) << _pow))) % num_rows;
      } while (j == i);
    }
    edges.push_back(std::make_pair(i, j));
  }
  return edges;
}

auto bench_pre_bigrows() {
  nb_rows = n_bigrows;
  degree = degree_bigrows;
  bench_pre_shared([&] {
#if defined(INPUT_BENCHMARKING)
    const char* filename = "matrix_random_benchmarking.dat";
#elif defined(INPUT_TPAL)
    const char* filename = "matrix_random_tpal.dat";
#elif defined(INPUT_TESTING)
    const char* filename = "matrix_random_testing.dat";
#else
  #error "Need to select input size, e.g., INPUT_{BENCHMARKING, TPAL, TESTING}"
#endif
    if (!access(filename, F_OK)) {
      printf("read matrix from %s\n", filename);
      read_matrix_from_file(filename);
    } else {
      auto edges = mk_random_local_edgelist(dim, degree, nb_rows);
      csr_of_edgelist(edges);
      write_matrix_to_file(filename);
      printf("write matrix to %s\n", filename);
    }
  });
}

#elif defined(SPMV_POWERLAW)

auto mk_powerlaw_edgelist(size_t lg) {
  edgelist_type edges;
  size_t o = 0;
  size_t m = 1 << lg;
  size_t n = 1;
  size_t tot = m;
  for (size_t i = 0; i < lg; i++) {
    for (size_t j = 0; j < n; j++) {
      for (size_t k = 0; k < m; k++) {
        auto d = hash64(o + k) % tot;
        edges.push_back(std::make_pair(o, d));
      }
      o++;
    }
    n *= 2;
    m /= 2;
  }

  return edges;
}

auto bench_pre_bigcols() {
  nb_rows = 1<<n_bigcols;
  bench_pre_shared([&] {
#if defined(INPUT_BENCHMARKING)
    const char* filename = "matrix_powerlaw_benchmarking.dat";
#elif defined(INPUT_TPAL)
    const char* filename = "matrix_powerlaw_tpal.dat";
#elif defined(INPUT_TESTING)
    const char* filename = "matrix_powerlaw_testing.dat";
#else
  #error "Need to select input size, e.g., INPUT_{BENCHMARKING, TPAL, TESTING}"
#endif
    if (!access(filename, F_OK)) {
      printf("read matrix from %s\n", filename);
      read_matrix_from_file(filename);
    } else {
      auto edges = mk_powerlaw_edgelist(n_bigcols);
      csr_of_edgelist(edges);
      write_matrix_to_file(filename);
      printf("write matrix to %s\n", filename);
    }
  });
}

#elif defined(SPMV_ARROWHEAD)

auto mk_arrowhead_edgelist(size_t n) {
  edgelist_type edges;
  for (size_t i = 0; i < n; i++) {
    edges.push_back(std::make_pair(i, 0));
  }
  for (size_t i = 0; i < n; i++) {
    edges.push_back(std::make_pair(0, i));
  }
  for (size_t i = 0; i < n; i++) {
    edges.push_back(std::make_pair(i, i));
  }
  return edges;
}

auto bench_pre_arrowhead() {
  nb_rows = n_arrowhead;
  bench_pre_shared([&] {
#if defined(INPUT_BENCHMARKING)
    const char* filename = "matrix_arrowhead_benchmarking.dat";
#elif defined(INPUT_TPAL)
    const char* filename = "matrix_arrowhead_tpal.dat";
#elif defined(INPUT_TESTING)
    const char* filename = "matrix_arrowhead_testing.dat";
#else
  #error "Need to select input size, e.g., INPUT_{BENCHMARKING, TPAL, TESTING}"
#endif
    if (!access(filename, F_OK)) {
      printf("read matrix from %s\n", filename);
      read_matrix_from_file(filename);
    } else {
      auto edges = mk_arrowhead_edgelist(nb_rows);
      csr_of_edgelist(edges);
      write_matrix_to_file(filename);
      printf("write matrix to %s\n", filename);
    }
  });
}

#elif defined(SPMV_DENSE)

auto mk_dense_edgelist(size_t n) {
  edgelist_type edges;
  for (size_t i = 0; i < n; i++) {
    for (size_t j = 0; j < n; j++) {
      edges.push_back(std::make_pair(i, j));
    }
  }
  return edges;
}

auto bench_pre_dense() {
  nb_rows = n_dense;
  bench_pre_shared([&] {
#if defined(INPUT_BENCHMARKING)
    const char* filename = "matrix_dense_benchmarking.dat";
#elif defined(INPUT_TPAL)
    const char* filename = "matrix_dense_tpal.dat";
#elif defined(INPUT_TESTING)
    const char* filename = "matrix_dense_testing.dat";
#else
  #error "Need to select input size, e.g., INPUT_{BENCHMARKING, TPAL, TESTING}"
#endif
    if (!access(filename, F_OK)) {
      printf("read matrix from %s\n", filename);
      read_matrix_from_file(filename);
    } else {
      auto edges = mk_dense_edgelist(nb_rows);
      csr_of_edgelist(edges);
      write_matrix_to_file(filename);
      printf("write matrix to %s\n", filename);
    }
  });
}

#elif defined(SPMV_DIAGONAL)

auto mk_diagonal_edgelist(size_t n) {
  edgelist_type edges;
  for (size_t i = 0; i < n; i++) {
    edges.push_back(std::make_pair(i, i));
  }
  return edges;
}

auto bench_pre_diagonal() {
  nb_rows = n_diagonal;
  bench_pre_shared([&] {
#if defined(INPUT_BENCHMARKING)
    const char* filename = "matrix_disgonal_benchmarking.dat";
#elif defined(INPUT_TPAL)
    const char* filename = "matrix_diagonal_tpal.dat";
#elif defined(INPUT_TESTING)
    const char* filename = "matrix_diagonal_testing.dat";
#else
  #error "Need to select input size, e.g., INPUT_{BENCHMARKING, TPAL, TESTING}"
#endif
    if (!access(filename, F_OK)) {
      printf("read matrix from %s\n", filename);
      read_matrix_from_file(filename);
    } else {
      auto edges = mk_diagonal_edgelist(nb_rows);
      csr_of_edgelist(edges);
      write_matrix_to_file(filename);
      printf("write matrix to %s\n", filename);
    }
  });
}

#elif defined(SPMV_NORMAL)

#include <math.h>
#define _USE_MATH_DEFINES

auto mk_normal_edgelist(size_t n) {
  edgelist_type edges;
  double mean = n/2;
  double stdev = n/6.1;
  // double stdev = n/10;   // steeper curve
  // double stdev = n/5;    // flatter curve
  for (size_t i = 0; i < n; i++) {
    double y = 1.0 / (stdev * sqrt(2.0 * M_PI)) * exp(-(pow((i - mean)/stdev, 2)/2.0));
    uint64_t max = std::min(n, (size_t) (y * n * (n/2.4)));   // scaling and ceiling
    for (size_t j = 0; j < max; j++) {
      edges.push_back(std::make_pair(i, j));
    }
  }
  return edges;
}

auto bench_pre_normal() {
  nb_rows = n_normal;
  bench_pre_shared([&] {
#if defined(INPUT_BENCHMARKING)
    const char* filename = "matrix_normal_benchmarking.dat";
#elif defined(INPUT_TPAL)
    const char* filename = "matrix_normal_tpal.dat";
#elif defined(INPUT_TESTING)
    const char* filename = "matrix_normal_testing.dat";
#else
  #error "Need to select input size, e.g., INPUT_{BENCHMARKING, TPAL, TESTING}"
#endif
    if (!access(filename, F_OK)) {
      printf("read matrix from %s\n", filename);
      read_matrix_from_file(filename);
    } else {
      auto edges = mk_normal_edgelist(nb_rows);
      csr_of_edgelist(edges);
      write_matrix_to_file(filename);
      printf("write matrix to %s\n", filename);
    }
  });
}

#elif defined(SPMV_MATRIX_MARKET)

auto bench_pre_matrix_market() -> void {
  using nonzero_type = double;
  auto dflt_fname = "twitter7/twitter7.mtx";
  std::string fname = dflt_fname;
  if (const auto env_p = std::getenv("MATRIX_MARKET_FILE")) {
    fname = env_p;
  }
  typedef size_t Offset;
  typedef uint64_t Ordinal;
  typedef nonzero_type Scalar;
  typedef MtxReader<Ordinal, Scalar> reader_t;
  typedef typename reader_t::coo_type coo_t;
  typedef typename coo_t::entry_type entry_t;
  typedef CSR<Ordinal, Scalar, Offset> csr_type;

  bench_pre_shared([&] {
    size_t pos = fname.find('.');
    std::string datname = fname.substr(0, pos) + ".dat";
    if (!access(datname.c_str(), F_OK)) {
      printf("read matrix from %s\n", datname.c_str());
      read_matrix_from_file(datname.c_str());
    } else {
      reader_t reader(fname);
      coo_t coo = reader.read_coo();
      csr_type csr(coo);
      nb_vals = csr.nnz();
      val = (nonzero_type*)malloc(sizeof(nonzero_type) * nb_vals);
      std::copy(csr.val().begin(), csr.val().end(), val);
      nb_rows = csr.num_rows();
      uint64_t nb_cols = csr.num_cols();
      row_ptr = (Ordinal*)malloc(sizeof(Ordinal) * (nb_rows + 1));
      std::copy(csr.row_ptr().begin(), csr.row_ptr().end(), row_ptr);
      col_ind = (Offset*)malloc(sizeof(Offset) * nb_vals);
      std::copy(&csr.col_ind()[0], (&csr.col_ind()[nb_cols-1]) + 1, col_ind);

      write_matrix_to_file(datname.c_str());
      printf("write matrix to %s\n", datname.c_str());
    }
  });
}

#else

  #error "Need to select input class: SPMV_{RANDOM, POWERLAW, ARROWHEAD, DENSE, DIAGONAL, NORMAL, MATRIX_MARKET}"

#endif

void setup() {
#if defined(SPMV_RANDOM)
  bench_pre_bigrows();
#elif defined(SPMV_POWERLAW)
  bench_pre_bigcols();
#elif defined(SPMV_ARROWHEAD)
  bench_pre_arrowhead();
#elif defined(SPMV_DENSE)
  bench_pre_dense();
#elif defined(SPMV_DIAGONAL)
  bench_pre_diagonal();
#elif defined(SPMV_NORMAL)
  bench_pre_normal();
#elif defined(SPMV_MATRIX_MARKET)
  bench_pre_matrix_market();
#else
  #error "Need to select input class: SPMV_{RANDOM, POWERLAW, ARROWHEAD, DENSE, DIAGONAL, NORMAL, MATRIX_MARKET}"
#endif
}

void finishup() {
  free(val);
  free(row_ptr);
  free(col_ind);
  free(x);
  free(y);
}

#if defined(USE_BASELINE) || defined(TEST_CORRECTNESS)

void spmv_serial(
  double* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  double* x,
  double* y,
  uint64_t n) {
  for (uint64_t i = 0; i < n; i++) { // row loop
    double r = 0.0;
    for (uint64_t k = row_ptr[i]; k < row_ptr[i + 1]; k++) { // col loop
      r += val[k] * x[col_ind[k]];
    }
    y[i] = r;
  }
}

#endif

#if defined(USE_OPENCILK)

#include <cilk/cilk.h>

#if defined(OPENCILK_REDUCER)

void zero_double(void *view) {
  *(double *)view = 0.0;
}
void add_double(void *left, void *right) {
  *(double *)left += *(double *)right;
}
void spmv_opencilk(
  double* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  double* x,
  double* y,
  uint64_t n) {
  cilk_for (int64_t i = 0; i < n; i++) {  // row loop
    double cilk_reducer(zero_double, add_double) sum;
    cilk_for (int64_t k = row_ptr[i]; k < row_ptr[i+1]; k++) { // col loop
      sum += val[k] * x[col_ind[k]];
    }
    y[i] = sum;
  }
}

#elif defined(OPENCILK_MANUAL_GRANULARITY)

static constexpr
uint64_t threshold = 1024;
template <typename F>
void map_reduce_cilk(const F& f, uint64_t lo, uint64_t hi, double* dst) {
  double r = 0.0;
  if ((hi - lo) < threshold) {
    for (auto i = lo; i < hi; i++) {
      r += f(i);
    }
  } else {
    double r1, r2;
    auto mid = (lo + hi) / 2;
    cilk_spawn map_reduce_cilk(f, lo, mid, &r1);
    map_reduce_cilk(f, mid, hi, &r2);
    cilk_sync;
    r = r1 + r2;
  }
  *dst = r;
}

void spmv_opencilk(
  double* val,
	uint64_t* row_ptr,
	uint64_t* col_ind,
	double* x,
	double* y,
	uint64_t n) {
  cilk_for (int64_t i = 0; i < n; i++) {  // row loop
    map_reduce_cilk([=] (uint64_t k) {
      return val[k] * x[col_ind[k]];
    }, row_ptr[i], row_ptr[i+1], &y[i]);
  }
}

#elif defined(OPENCILK_OUTER_FOR)

void spmv_opencilk(
  double* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  double* x,
  double* y,
  uint64_t n) {
  cilk_for (uint64_t i = 0; i < n; i++) { // row loop
    double r = 0.0;
    for (uint64_t k = row_ptr[i]; k < row_ptr[i + 1]; k++) { // col loop
      r += val[k] * x[col_ind[k]];
    }
    y[i] = r;
  }
}

#elif defined(OPENCILK_HEURISTICS)

#include <cilk/cilk_api.h>

int nworkers = __cilkrts_get_nworkers();
auto ceiling_div(uint64_t x, uint64_t y) -> uint64_t {
  return (x + y - 1) / y;
}
auto get_grainsize(uint64_t n) -> uint64_t {
  return std::min<uint64_t>(2048, ceiling_div(n, 8 * nworkers));
}

auto dotprod_rec(int64_t lo, int64_t hi, double* val, double* x, uint64_t* col_ind,
		 uint64_t grainsize) -> double {
  auto n = hi - lo;
  if (n <= grainsize) {
    double t = 0.0;
    for (int64_t k = lo; k < hi; k++) { // col loop
      t += val[k] * x[col_ind[k]];
    }
    return t;
  }
  auto mid = (lo + hi) / 2;
  double r1, r2;
  r1 = cilk_spawn dotprod_rec(lo, mid, val, x, col_ind, grainsize);
  r2 = dotprod_rec(mid, hi, val, x, col_ind, grainsize);
  cilk_sync;
  return r1 + r2;
}

auto dotprod(int64_t lo, int64_t hi, double* val, double* x, uint64_t* col_ind) -> double {
  return dotprod_rec(lo, hi, val, x, col_ind, get_grainsize(hi - lo));
}

void spmv_opencilk(
  double* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  double* x,
  double* y,
  uint64_t n) {
  cilk_for (int64_t i = 0; i < n; i++) {  // row loop
    y[i] = dotprod(row_ptr[i], row_ptr[i+1], val, x, col_ind);
  }
}
#endif

#elif defined(USE_CILKPLUS)

#include <cilk/cilk.h>
#include <cilk/reducer_opadd.h>

void spmv_cilkplus(
  double* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  double* x,
  double* y,
  uint64_t n) {
  cilk_for (int64_t i = 0; i < n; i++) {  // row loop
    cilk::reducer_opadd<double> sum(0.0);
    cilk_for (int64_t k = row_ptr[i]; k < row_ptr[i+1]; k++) { // col loop
      *sum += val[k] * x[col_ind[k]];
    }
    y[i] = sum.get_value();
  }
}

#elif defined(USE_OPENMP)

#include <omp.h>

void spmv_openmp(
  double* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  double* x,
  double* y,
  uint64_t n) {
#if defined(OMP_SCHEDULE_STATIC)
  #pragma omp parallel for schedule(static)
#elif defined(OMP_SCHEDULE_DYNAMIC)
  #pragma omp parallel for schedule(dynamic)
#elif defined(OMP_SCHEDULE_GUIDED)
  #pragma omp parallel for schedule(guided)
#endif
  for (uint64_t i = 0; i < n; i++) { // row loop
    double r = 0.0;
#if defined(OMP_NESTED_SCHEDULING)
#if defined(OMP_SCHEDULE_STATIC)
    #pragma omp parallel for schedule(static) reduction(+:r)
#elif defined(OMP_SCHEDULE_DYNAMIC)
    #pragma omp parallel for schedule(dynamic) reduction(+:r)
#elif defined(OMP_SCHEDULE_GUIDED)
    #pragma omp parallel for schedule(guided) reduction(+:r)
#endif
#endif
    for (uint64_t k = row_ptr[i]; k < row_ptr[i + 1]; k++) { // col loop
      r += val[k] * x[col_ind[k]];
    }
    y[i] = r;
  }
}

#endif

#if defined(TEST_CORRECTNESS)

#include <stdio.h>

void test_correctness() {
  double* yref = (double*)malloc(sizeof(double) * nb_rows);
  {
    for (uint64_t i = 0; i != nb_rows; i++) {
      yref[i] = 1.0;
    }
    spmv_serial(val, row_ptr, col_ind, x, yref, nb_rows);
  }
  uint64_t num_diffs = 0;
  double epsilon = 0.01;
  for (uint64_t i = 0; i != nb_rows; i++) {
    auto diff = std::abs(y[i] - yref[i]);
    if (diff > epsilon) {
      printf("diff=%f y[i]=%f yref[i]=%f at i=%ld\n", diff, y[i], yref[i], i);
      num_diffs++;
    }
  }
  if (num_diffs > 0) {
    printf("\033[0;31mINCORRECT!\033[0m");
    printf("  num_diffs = %lu\n", num_diffs);
  } else {
    printf("\033[0;32mCORRECT!\033[0m\n");
  }
  //printf("num_diffs %ld\n", num_diffs);
  free(yref);

}

#endif

} // namespace spmv

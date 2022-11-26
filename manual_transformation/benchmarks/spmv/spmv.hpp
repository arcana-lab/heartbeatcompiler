#include <cstdint>
#include <algorithm>
#include <vector>
#include <functional>
#include <cassert>
#if defined(USE_OPENCILK)
#include <cilk/cilk.h>
#endif
#if defined(USE_OPENMP)
#include <omp.h>
#endif
#if defined(TEST_CORRECTNESS)
#include <cstdio>
#endif

namespace spmv {

#if defined(INPUT_BENCHMARKING)
  #if defined(SPMV_RANDOM)
    uint64_t n_bigrows = 3000000;
    uint64_t degree_bigrows = 100;
  #elif defined(SPMV_POWERLAW)
    uint64_t n_bigcols = 23;
  #elif defined(SPMV_ARROWHEAD)
    uint64_t n_arrowhead = 1500000000;
  #else
    #error "Need to select input class, e.g., SPMV_RANDOM, SPMV_POWERLAW, SPMV_ARROWHEAD"
  #endif
#elif defined(INPUT_TESTING)
  #if defined(SPMV_RANDOM)
    uint64_t n_bigrows = 3000000;
    uint64_t degree_bigrows = 100;
  #elif defined(SPMV_POWERLAW)
    uint64_t n_bigcols = 23;
  #elif defined(SPMV_ARROWHEAD)
    uint64_t n_arrowhead = 10000000;
  #else
    #error "Need to select input class, e.g., SPMV_RANDOM, SPMV_POWERLAW, SPMV_ARROWHEAD"
  #endif
#else
  #error "Need to select input size, e.g., INPUT_BENCHMARKING, INPUT_TESTING"
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
    auto edges = mk_random_local_edgelist(dim, degree, nb_rows);
    csr_of_edgelist(edges);
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
    auto edges = mk_powerlaw_edgelist(n_bigcols);
    csr_of_edgelist(edges);
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
    auto edges = mk_arrowhead_edgelist(nb_rows);
    csr_of_edgelist(edges);
  });
}

#else

  #error "Need to select input class, e.g., SPMV_RANDOM, SPMV_POWERLAW, SPMV_ARROWHEAD"

#endif

void setup() {
#if defined(SPMV_RANDOM)
  bench_pre_bigrows();
#elif defined(SPMV_POWERLAW)
  bench_pre_bigcols();
#elif defined(SPMV_ARROWHEAD)
  bench_pre_arrowhead();
#else
  #error "Need to select input class, e.g., SPMV_RANDOM, SPMV_POWERLAW, SPMV_ARROWHEAD"
#endif
}

void finishup() {
  free(val);
  free(row_ptr);
  free(col_ind);
  free(x);
  free(y);
}

#if defined(USE_OPENCILK)

// // spmv reducer implementation
// void zero_double(void *view) {
//   *(double *)view = 0.0;
// }
// void add_double(void *left, void *right) {
//   *(double *)left += *(double *)right;
// }
// void spmv_opencilk(
//   double* val,
//   uint64_t* row_ptr,
//   uint64_t* col_ind,
//   double* x,
//   double* y,
//   int64_t n) {
//   cilk_for (int64_t i = 0; i < n; i++) {  // row loop
//     double cilk_reducer(zero_double, add_double) sum;
//     cilk_for (int64_t k = row_ptr[i]; k < row_ptr[i+1]; k++) { // col loop
//       sum += val[k] * x[col_ind[k]];
//     }
//     y[i] = sum;
//   }
// }

// spmv map-reduce implementation
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
	int64_t n) {
  cilk_for (int64_t i = 0; i < n; i++) {  // row loop
    map_reduce_cilk([=] (uint64_t k) {
      return val[k] * x[col_ind[k]];
    }, row_ptr[i], row_ptr[i+1], &y[i]);
  }
}

// // spmv sequential inner loop implementation
// void spmv_opencilk(
//   double* val,
// 	uint64_t* row_ptr,
// 	uint64_t* col_ind,
// 	double* x,
// 	double* y,
// 	int64_t n) {
//   cilk_for (int64_t i = 0; i < n; i++) {  // row loop
//     double t = 0.0;
//     for (int64_t k = row_ptr[i]; k < row_ptr[i+1]; k++) { // col loop
//       t += val[k] * x[col_ind[k]];
//     }
//     y[i] = t;
//   }
// }
#elif defined(USE_OPENMP)
void spmv_openmp(
  double* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  double* x,
  double* y,
  int64_t n) {
#if defined(OMP_DYNAMIC)
  #pragma omp parallel for schedule(dynamic)
#elif defined(OMP_GUIDED) 
  #pragma omp parallel for schedule(guided)
#else  
  #pragma omp parallel for
#endif
  for (int64_t i = 0; i < n; i++) {  // row loop
    double r = 0.0;
    for (int64_t k = row_ptr[i]; k < row_ptr[i+1]; k++) { // col loop
      r += val[k] * x[col_ind[k]];
    }
    y[i] = r;
  }
}
#else
void spmv_serial(
  double *val,
  uint64_t *row_ptr,
  uint64_t *col_ind,
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

#if defined(TEST_CORRECTNESS)
void test_correctness(double* y) {
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
    printf("INCORRECT!");
    printf("  num_diffs = %lu\n", num_diffs);
  } else {
    printf("CORRECT!\n");
  }
  //printf("num_diffs %ld\n", num_diffs);
  free(yref);
}
#endif

}

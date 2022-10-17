#include <algorithm>
#include <climits>

#define SUB(array, row_sz, i, j) (array[i * row_sz + j])

#define INF           INT_MAX-1

int vertices = 1024;
int* dist = nullptr;

auto init_input(int vertices) {
  int* dist = (int*)malloc(sizeof(int) * vertices * vertices);
  for(int i = 0; i < vertices; i++) {
    for(int j = 0; j < vertices; j++) {
      SUB(dist, vertices, i, j) = ((i == j) ? 0 : INF);
    }
  }
  for (int i = 0 ; i < vertices ; i++) {
    for (int j = 0 ; j< vertices; j++) {
      if (i == j) {
	      SUB(dist, vertices, i, j) = 0;
      } else {
	      int num = i + j;
        if (num % 3 == 0) {
          SUB(dist, vertices, i, j) = num / 2;
        } else if (num % 2 == 0) {
          SUB(dist, vertices, i, j) = num * 2;
        } else {
          SUB(dist, vertices, i, j) = num;
        }
      }
    }
  }
  return dist;
};

void floyd_warshall(int* dist, int vertices) {
  for(int via = 0; via < vertices; via++) {
    for(int from = 0; from < vertices; from++) {
      for(int to = 0; to < vertices; to++) {
        if ((from != to) && (from != via) && (to != via)) {
          SUB(dist, vertices, from, to) = 
            std::min(SUB(dist, vertices, from, to), 
                     SUB(dist, vertices, from, via) + SUB(dist, vertices, via, to));
        }
      }
    }
  }
}

int main() {

  dist = init_input(vertices);

  floyd_warshall(dist, vertices);

  free(dist);

  return 0;
}
#include "merge.h"
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // memcpy
#include <time.h>
int MPI_Odd_even(int n, double *a, int root, MPI_Comm comm);
int MPI_Exchange(int n, double *a, int rank1, int rank2, MPI_Comm comm);
int MPI_Shell_sort(int n, double *a, int root, MPI_Comm comm);

// helpers
double *createarray(int rank, int n) {
  int i;
  double m = 10.0;
  double *array = (double *)calloc(n, sizeof(double));
  if (rank == 0) {
    // initialise the array with random values, then scatter to all processors
    srand(((unsigned)time(NULL) + rank));
    for (i = 0; i < n; i++) {
      array[i] = ((double)rand() / RAND_MAX) * m;
    }
  }
  return array;
}

double timefunc(int (*f)(int, double *, int, MPI_Comm), int n, double *a,
                int root, MPI_Comm comm) {
  MPI_Barrier(comm); // everyone starts together
  double begin = MPI_Wtime();

  f(n, a, root, comm);

  double local = MPI_Wtime() - begin;
  double slowest;
  MPI_Reduce(&local, &slowest, 1, MPI_DOUBLE, MPI_MAX, root, comm);
  return slowest; // only meaningful on root
}

int MPI_Exchange(int n, double *a, int rank1, int rank2, MPI_Comm comm) {
  int rank;
  MPI_Comm_rank(comm, &rank);

  int partner = (rank == rank1) ? rank2 : rank1;
  double *b = (double *)malloc(n * sizeof(double));

  // swap blocks with the partner
  MPI_Sendrecv(a, n, MPI_DOUBLE, partner, 0, b, n, MPI_DOUBLE, partner, 0, comm,
               MPI_STATUS_IGNORE);

  // merge both sorted blocks into 2n sorted values
  double *c = merge_array(n, a, n, b);

  // rank1 keeps the lower half, rank2 keeps the upper half
  if (rank == rank1)
    memcpy(a, c, n * sizeof(double));
  else
    memcpy(a, c + n, n * sizeof(double));

  free(b);
  free(c);
  return MPI_SUCCESS;
}

int MPI_Issorted(int n, double *array, int root, MPI_Comm comm) {
  int rank, size;
  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);

  int answer = 1; // 1 = sorted, 0 = not sorted

  // 1. check this rank's own block is sorted
  for (int i = 0; i < n - 1; i++) {
    if (array[i] > array[i + 1]) {
      answer = 0;
      break;
    }
  }

  // 2. gather every block's first and last element on root
  double *firsts = NULL, *lasts = NULL;
  if (rank == root) {
    firsts = (double *)calloc(size, sizeof(double));
    lasts = (double *)calloc(size, sizeof(double));
  }
  MPI_Gather(&array[0], 1, MPI_DOUBLE, firsts, 1, MPI_DOUBLE, root, comm);
  MPI_Gather(&array[n - 1], 1, MPI_DOUBLE, lasts, 1, MPI_DOUBLE, root, comm);

  // 3. root checks the boundaries between neighbouring blocks
  if (rank == root) {
    for (int i = 0; i < size - 1; i++) {
      if (firsts[i + 1] < lasts[i]) {
        answer = 0;
        break;
      }
    }
    free(firsts);
    free(lasts);
  }

  // 4. combine everyone's answer and give the result to all ranks
  int global;
  MPI_Allreduce(&answer, &global, 1, MPI_INT, MPI_LAND, comm);
  return global;
}

int main(int argc, char **argv) {
  int rank, size;
  int n = 100;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  double *array = createarray(rank, n);

  // timing the sorts
  // double MPI_Odd_even_time =
  //     timefunc(MPI_Odd_even, n, array, 0, MPI_COMM_WORLD);

  double MPI_Shell_sort_time =
      timefunc(MPI_Shell_sort, n, array, 0, MPI_COMM_WORLD);

  // results
  if (rank == 0)
    // printf("sorted %d values on %d procs in %f s\n", n, size,
    //        MPI_Odd_even_time);

    printf("sorted %d values on %d procs in %f s\n", n, size,
           MPI_Shell_sort_time);

  MPI_Barrier(MPI_COMM_WORLD);

  printf("rank %d of %d\n", rank, size);
  MPI_Finalize();
  return 0;
}

int MPI_Odd_even(int n, double *a, int root, MPI_Comm comm) {
  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  double *localArray = calloc(n / size, sizeof(double));

  MPI_Scatter(a, n / size, MPI_DOUBLE, localArray, n / size, MPI_DOUBLE, root,
              comm);
  merge_sort(n / size, localArray);
  for (int i = 0; i < size; i++) {
    int partner = (i % 2 == rank % 2) ? rank + 1 : rank - 1;

    if (partner >= 0 && partner < size) {
      int low = rank < partner ? rank : partner;
      int high = rank < partner ? partner : rank;
      MPI_Exchange(n / size, localArray, low, high, comm);
    }
  }
  MPI_Barrier(comm);

  MPI_Gather(localArray, n / size, MPI_DOUBLE, a, n / size, MPI_DOUBLE, root,
             comm);
  // if (rank == 0)
  //   for (int i = 0; i < n; i++) {
  //     printf("array[%d] = %f\n\n", i, a[i]);
  //   }
  return MPI_SUCCESS;
}

int MPI_Shell_sort(int n, double *a, int root, MPI_Comm comm) {
  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  double *localArray = calloc(n / size, sizeof(double));
  // each proceessor gets an array to insert sort on its own
  //
  MPI_Scatter(a, n / size, MPI_DOUBLE, localArray, n / size, MPI_DOUBLE, root,
              comm);
  // Start with a large gap, then reduce it step by step
  for (int gap = size / 2; gap > 0; gap /= 2) {
    // Perform a "gapped" insertion sort for this gap size
    for (int i = 0; i < size - gap; i++) {
      int low = rank + i;
      int high = rank + i + gap;
      MPI_Exchange(n / size, localArray, low, high, comm);
    }

    // for (int i = gap; i < n; i++) {
    //   MPI_Exchange(n / size, localArray, i - gap, i, comm);
    // }
  }

  MPI_Gather(localArray, n / size, MPI_DOUBLE, a, n / size, MPI_DOUBLE, root,
             comm);
  return MPI_Odd_even(n, a, root, comm);
}

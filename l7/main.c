/**
 * MPI Program that performs simple sorting
 *
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

double *merge_array(int n, double *a, int m, double *b);
void merge_sort(int n, double *a);
void bubble_sort(int n, double *a);
void swap(double *a, double *b);
int MPI_Sort_direct(int n, double *a, int root, MPI_Comm comm);
int MPI_Sort_bucket(int n, double *a, double max, int root, MPI_Comm comm);
int MPI_Sort_odd_even(int n, double *a, int root, MPI_Comm comm);
int MPI_Exchange(int n, double *a, int root, int dest, MPI_Comm comm);
int MPI_Issorted(int n, double *array, int root, MPI_Comm comm);

int main(int argc, char *argv[]) {
  int rank, size;
  int n = 100, i, j, k, x, q, l, shell, pair, *nr;
  double m = 10.0;
  double *array = (double *)calloc(n, sizeof(double));
  // Init + rank + size
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  if (rank == 0) {
    // initialise the array with random values, then scatter to all processors
    srand(((unsigned)time(NULL) + rank));
    for (i = 0; i < n; i++) {
      array[i] = ((double)rand() / RAND_MAX) * m;
    }
  }
  // call and time evaluate MPI_Sort_direct
  double time1 = MPI_Wtime();

  MPI_Sort_odd_even(n, array, 0, MPI_COMM_WORLD);
  // MPI_Sort_direct(n, array, 0, MPI_COMM_WORLD);
  double time2 = MPI_Wtime();
  double elapsed = time2 - time1;
  double overallTime;
  MPI_Reduce(&elapsed, &overallTime, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
  printf("Time taken by MPI_Sort_odd_even by processor %d is %f\n", rank,
         elapsed);
  // print array to check
  // if (rank == 0) {
  //   for (int i = 0; i < n; i++) {
  //     printf("%f \n", array[i]);
  //   }
  // }

  if (rank == 0) {
    printf("Overall time taken by MPI_Sort_odd_even is %f\n", overallTime);
  }

  // double time1b = MPI_Wtime();
  //
  // double time2b = MPI_Wtime();
  // double elapsedb = time2 - time1;
  // double overallTimeb;
  // MPI_Reduce(&elapsedb, &overallTimeb, 1, MPI_DOUBLE, MPI_MAX, 0,
  //            MPI_COMM_WORLD);
  // printf("Time taken by MPI_Sort_bucket by processor %d is %f\n", rank,
  //        elapsed);
  //
  // if (rank == 0) {
  //   printf("Overall time taken by MPI_Sort_bucket is %f\n", overallTime);
  // }
  //
  MPI_Finalize();
}

int MPI_Sort_bucket(int n, double *a, double max, int root, MPI_Comm comm) {
  // get rank and size
  int rank, size;
  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);
  // bcast a
  MPI_Bcast(a, n, MPI_DOUBLE, root, comm);
  // collect a's elements to processor ranks bucket
  double *bucket = (double *)calloc(n, sizeof(double));
  int count = 0;
  for (int i = 0; i < n; i++) {
    // check if a[i] is in the bucket [max/size*rank:max/size*rank+1]
    if (a[i] > (max / (double)size) * rank &&
        a[i] < (max / (double)size) * rank + 1) {
      bucket[count++] = a[i];
      //
      //
    }
  }
  // sort bucket
  merge_sort(count, bucket);
  // gatherv  bucket
  int *recvcounts = (int *)calloc(size, sizeof(int));
  int *displs = (int *)calloc(size, sizeof(int));
  MPI_Gather(&count, 1, MPI_INT, recvcounts, 1, MPI_INT, root, comm);

  if (rank == root) {
    displs[0] = 0;
    for (int i = 1; i < size; i++)
      displs[i] = displs[i - 1] + recvcounts[i - 1];
  }
  MPI_Gatherv(bucket, count, MPI_DOUBLE, a, recvcounts, displs, MPI_DOUBLE,
              root, comm);
  return MPI_SUCCESS;
}

// MPI Functions
int MPI_Sort_direct(int n, double *array, int root, MPI_Comm comm) {
  int rank, size;
  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);
  double *localArray = (double *)calloc(n / size, sizeof(double));
  // scatter array to localArray with double elements
  int rc = MPI_Scatter(array, n / size, MPI_DOUBLE, localArray, n / size,
                       MPI_DOUBLE, root, comm);
  if (rc != 0) {
    return rc;
  }
  // bubble_sort(n / size, localArray);
  merge_sort(n / size, localArray);
  rc = MPI_Gather(localArray, n / size, MPI_DOUBLE, array, n / size, MPI_DOUBLE,
                  root, comm);

  // sort localArray
  // gather localArray to array with double elements
  if (rank == 0) {
    // merge the size chunks of array
    // print array
    int chunk = n / size;
    for (int i = 1; i < size; i++) {
      // merge the sorted prefix (i chunks) with the next sorted chunk
      double *tmp = merge_array(i * chunk, array, chunk, array + i * chunk);
      memcpy(array, tmp, (i + 1) * chunk * sizeof(double));
      free(tmp);
    }

    // for (int i = 0; i < n; i++) {
    //   printf("%f ", array[i]);
    // }
    // printf("\n");
  }
  return MPI_SUCCESS;
}
// function to merge the array a with n elements with the array b with m
// elements function returns the nerged array
double *merge_array(int n, double *a, int m, double *b) {
  int i, j, k;
  double *c = (double *)calloc(n + m, sizeof(double));
  for (i = j = k = 0; (i < n) && (j < m);)
    if (a[i] <= b[j])
      c[k++] = a[i++];
    else
      c[k++] = b[j++];
  if (i == n)
    for (; j < m;)
      c[k++] = b[j++];
  else
    for (; i < n;)
      c[k++] = a[i++];
  return c;
}
// function to merge sort the array a with n elements
void merge_sort(int n, double *a) {
  double *c;
  int i;
  if (n <= 1)
    return;
  if (n == 2) {
    if (a[0] > a[1])
      swap(&a[0], &a[1]);
    return;
  }
  merge_sort(n / 2, a);
  merge_sort(n - n / 2, a + n / 2);
  c = merge_array(n / 2, a, n - n / 2, a + n / 2);
  for (i = 0; i < n; i++)
    a[i] = c[i];
  return;
}
// function to bubble sort
void bubble_sort(int n, double *a) {
  for (int i = 0; i < n - 1; i++)
    for (int j = 0; j < n - 1; j++)
      if (a[j] > a[j + 1])
        swap(a + j, a + j + 1);
}
// swap two doubles
void swap(double *a, double *b) {
  double temp;
  temp = *a;
  *a = *b;
  *b = temp;
}

int MPI_Sort_odd_even(int n, double *a, int root, MPI_Comm comm) {
  int rank, size;
  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);
  // allocate space
  double *locala = (double *)calloc(n / size, sizeof(double));
  // scater
  MPI_Scatter(a, n / size, MPI_DOUBLE, locala, n / size, MPI_DOUBLE, root,
              comm);
  // sort
  merge_sort(n / size, locala);
  for (int i = 0; i < size; i++) {
    if ((i + rank) % 2 == 0) {
      if (rank + 1 < size)
        MPI_Exchange(n / size, locala, rank, rank + 1, comm);
    } else {
      if (rank > 0)
        MPI_Exchange(n / size, locala, rank - 1, rank, comm);
    }
    // MPI_Barrier(comm);
    // check sort
    MPI_Issorted(n / size, locala, 0, comm);
  }
  MPI_Gather(locala, n / size, MPI_DOUBLE, a, n / size, MPI_DOUBLE, root, comm);

  return MPI_SUCCESS;
}

int MPI_Sort_shell(int n, double *a, int root, MPI_Comm comm) {
  int rank, size;
  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);
  // allocate space
  double *locala = (double *)calloc(n / size, sizeof(double));
  // scater
  MPI_Scatter(a, n / size, MPI_DOUBLE, locala, n / size, MPI_DOUBLE, root,
              comm);
  // sort
  merge_sort(n / size, locala);

  int left = 0, right = n / size - 1;

  return MPI_SUCCESS;
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

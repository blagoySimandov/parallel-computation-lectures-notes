#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int MPI_Sort_direct(int n, double *array, int root, MPI_Comm comm);
int MPI_Sort_bucket(int n, double *array, int root, MPI_Comm comm);
void print_array(int n, double *array);

double *generate_array(int n) {
  int i, j, k, x, q, l, shell, pair, *nr;
  double m = 10.0;
  double *array = (double *)calloc(n, sizeof(double));
  for (i = 0; i < n; i++) {
    array[i] = rand() / (double)RAND_MAX * m;
  }
  return array;
}

int main(int argc, char **argv) {
  int rank, size;
  int root = 0;
  int n = 1000;
  double *array = generate_array(n);
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Sort_direct(n, array, root, MPI_COMM_WORLD);
  if (rank == root)
    print_array(n, array);

  MPI_Finalize();
  return 0;
}

void print_array(int n, double *array) {
  for (int i = 0; i < n; i++)
    printf("%f ", array[i]);
  printf("\n");
}

// swap two doubles
void swap(double *a, double *b) {
  double temp;
  temp = *a;
  *a = *b;
  *b = temp;
}

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

// MPI Functions
int MPI_Sort_direct(int n, double *array, int root, MPI_Comm comm) {
  int rank, size;
  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);
  double *localArray = (double *)calloc(n / size, sizeof(double));
  // scatter array to localArray with double elements
  int remaining = n % size;
  int *sendcount = calloc(size, sizeof(int));
  int *displs = calloc(size, sizeof(int));
  // each rank needs at most one extra element (if they all need one extra then
  // %size would be zero)
  for (int i = 0; i < size; i++) {
    sendcount[i] = n / size;
    if (i < remaining)
      sendcount[i]++;
    if (i == 0)
      displs[i] = 0;
    else
      displs[i] = displs[i - 1] + sendcount[i - 1];
  }

  int rc = MPI_Scatterv(array, sendcount, displs, MPI_DOUBLE, localArray,
                        n / size, MPI_DOUBLE, root, comm);
  if (rc != 0) {
    return rc;
  }
  // bubble_sort(n / size, localArray);
  merge_sort(n / size, localArray);

  // i send sendcount[rank] while root receives sendcount(array) elements with
  // different displs
  rc = MPI_Gatherv(localArray, sendcount[rank], MPI_DOUBLE, array, sendcount,
                   displs, MPI_DOUBLE, root, comm);

  // sort localArray
  // gather localArray to array with double elements
  if (rank == root) {
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

int MPI_Sort_bucket(int n, double *array, int root, MPI_Comm comm) {
  int rank, size;
  MPI_Comm_rank(comm, &rank);
}

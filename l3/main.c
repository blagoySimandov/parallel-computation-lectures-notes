#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>

// utility functions
int **alloc_matrix(int n, int m);
void init_matrix(int n, int m, int **a);
void print_matrix(int n, int m, int **a);
// matrix operations
int **prod_matrix(int n, int l, int m, int **a, int **b);
int **trans_matrix(int n, int m, int **a);

int **pseudo_prod_matrix(int n, int l, int m, int **a, int **b) {
  int i, j, k, **c;
  c = alloc_matrix(n, m);
  for (i = 0; i < n; i++)
    for (j = 0; j < m; j++) {
      c[i][j] = 0;
      for (k = 0; k < l; k++) {
        c[i][j] = c[i][j] + a[i][k] * b[j][k];
      }
    }
  return c;
}
// MPI functions
int MPI_Prod_matrix(int n, int **a, int **b, int **c, int root, MPI_Comm comm);
int main(int argc, char **argv) {
  int size, rank, tag = 1, i, j, n = 2000, **a, **b, **c;
  double time;
  MPI_Status stat;
  MPI_Datatype rowtype;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Type_contiguous(n, MPI_INT, &rowtype);
  MPI_Type_commit(&rowtype);
  a = alloc_matrix(n, n);
  b = alloc_matrix(n, n);
  c = alloc_matrix(n, n);
  if (rank == 0) {
    // initialise the matrices
    init_matrix(n, n, a);
    init_matrix(n, n, b);
  }
  time = MPI_Wtime();
  MPI_Prod_matrix(n, a, b, c, 0, MPI_COMM_WORLD);
  time = MPI_Wtime() - time;
  printf("Processor %d worked for %lf\n\n", rank, time);
  if (rank == 0) {
    // initialise the matrices
    // printf("Matrix a:\n\n");
    // print_matrix(n, n, a);
    // printf("Matrix b:\n\n");
    // print_matrix(n, n, b);
    // printf("Matrix c:\n\n");
    // print_matrix(n, n, c);
  }
  MPI_Type_free(&rowtype);
  MPI_Finalize();
}

int MPI_Prod_matrix(int n, int **a, int **b, int **c, int root, MPI_Comm comm) {
  // get rank and size of comm
  int rank, size;
  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);
  double commT, compT, time;
  time = MPI_Wtime();

  // alocate space for local_a and local_c
  int **local_a = alloc_matrix(n / size, n);
  int **local_c = alloc_matrix(n / size, n);
  // scatter a to local_a and bcast b
  commT = MPI_Wtime() - time;
  MPI_Scatter(a[0], n * n / size, MPI_INT, local_a[0], n * n / size, MPI_INT,
              root, comm);
  MPI_Bcast(b[0], n * n, MPI_INT, root, comm);

  // calculate local_c = local_a * b
  time = MPI_Wtime();
  local_c = prod_matrix(n / size, n, n, local_a, b);
  compT = MPI_Wtime() - time;
  // gather local_c
  MPI_Gather(local_c[0], n * n / size, MPI_INT, c[0], n * n / size, MPI_INT,
             root, comm);
  MPI_Reduce(rank == root ? MPI_IN_PLACE : &commT, &commT, 1, MPI_DOUBLE,
             MPI_MAX, root, comm);
  MPI_Reduce(rank == root ? MPI_IN_PLACE : &compT, &compT, 1, MPI_DOUBLE,
             MPI_MAX, root, comm);
  printf("Processor %d worked for communication %lf and computation %lf\n\n",
         rank, commT, compT);
  return MPI_SUCCESS;
}

int MPI_Prod_matrix2(int n, int **a, int **b, int **c, int root,
                     MPI_Comm comm) {
  // get rank and size of comm
  int rank, size;
  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);
  double commT, compT, time;
  time = MPI_Wtime();

  // alocate space for local_a and local_c
  int **local_a = alloc_matrix(n / size, n);
  int **local_c = alloc_matrix(n / size, n);
  // scatter a to local_a and bcast b
  commT = MPI_Wtime() - time;
  MPI_Scatter(a[0], n * n / size, MPI_INT, local_a[0], n * n / size, MPI_INT,
              root, comm);
  MPI_Bcast(b[0], n * n, MPI_INT, root, comm);

  // calculate local_c = local_a * b
  time = MPI_Wtime();
  local_c = pseudo_prod_matrix(n / size, n, n, local_a, b);
  compT = MPI_Wtime() - time;
  // gather local_c
  MPI_Gather(local_c[0], n * n / size, MPI_INT, c[0], n * n / size, MPI_INT,
             root, comm);
  MPI_Reduce(rank == root ? MPI_IN_PLACE : &commT, &commT, 1, MPI_DOUBLE,
             MPI_MAX, root, comm);
  MPI_Reduce(rank == root ? MPI_IN_PLACE : &compT, &compT, 1, MPI_DOUBLE,
             MPI_MAX, root, comm);
  printf("Processor %d worked for communication %lf and computation %lf\n\n",
         rank, commT, compT);
  return MPI_SUCCESS;
}

int MPI_Prod_matrix1(int n, int **a, int **b, int **c, int root,
                     MPI_Comm comm) {
  // get rank and size of comm
  int rank, size;
  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);
  // alocate space for local_a and local_c
  // timings
  double commT, compT, time;
  int **local_a = alloc_matrix(n / size, n);
  int **local_c = alloc_matrix(n / size, n);
  MPI_Datatype rowtype;

  time = MPI_Wtime();
  MPI_Type_contiguous(n / size, MPI_INT, &rowtype);
  MPI_Type_commit(&rowtype);
  // scatter a to local_a and bcast b
  MPI_Scatter(a[0], n / size, rowtype, local_a[0], n / size, rowtype, root,
              comm);
  MPI_Bcast(b[0], n, rowtype, root, comm);

  commT = MPI_Wtime() - time;

  // calculate local_c = local_a * b
  time = MPI_Wtime();
  local_c = prod_matrix(n / size, n, n, local_a, b);
  compT = MPI_Wtime() - time;

  // gather local_c
  MPI_Gather(local_c[0], n / size, rowtype, c[0], n / size, rowtype, root,
             comm);
  MPI_Reduce(rank == root ? MPI_IN_PLACE : &commT, &commT, 1, MPI_DOUBLE,
             MPI_MAX, root, comm);
  MPI_Reduce(rank == root ? MPI_IN_PLACE : &compT, &compT, 1, MPI_DOUBLE,
             MPI_MAX, root, comm);
  printf("Processor %d worked for communication %lf and computation %lf\n\n",
         rank, commT, compT);

  return MPI_SUCCESS;
}
/*
The function alloc_matrix is to allocate dynamically a matrix of integers with
n rows and m columns.
The function arguments are;
n - number of rows
m - number of columns
The function returns the double pointer representing the matrix.
*/
int **alloc_matrix(int n, int m) {
  int i, j, **a, *aa;
  aa = (int *)calloc(n * m, sizeof(int));
  a = (int **)calloc(n, sizeof(int *));
  for (i = 0; i < n; i++)
    a[i] = aa + i * m;
  for (i = 0; i < n; i++)
    for (j = 0; j < m; j++)
      a[i][j] = 0;
  return a;
}
/*
The function init_matrix is to initialize the matrix a with random integers.
The function arguments are:
n - number of rows
m - number of columns
a - the matrix
*/
void init_matrix(int n, int m, int **a) {
  int i, j;
  for (i = 0; i < n; i++)
    for (j = 0; j < m; j++)
      a[i][j] = rand() % 10;
}
/*
The function print_matrix is to display the matrix a.
The function arguments are:
n - number of rows
m - number of columns
a - the matrix
*/
void print_matrix(int n, int m, int **a) {
  int i, j;
  printf("\n");
  for (i = 0; i < n; i++) {
    for (j = 0; j < m; j++)
      printf("%d ", a[i][j]);
    printf("\n");
  }
}
/*
The function prod_matrix is to multiply the matrices a and b.
The function arguments are;
n - number of rows of a
l - number of columns of a // the matrix b must have l rows
m - number of columns of b
a - the first matrix
b - the second matrix
The function returns the double pointer representing the product matrix.
*/
int **prod_matrix(int n, int l, int m, int **a, int **b) {
  int i, j, k, **c;
  c = alloc_matrix(n, m);
  for (i = 0; i < n; i++)
    for (j = 0; j < m; j++) {
      c[i][j] = 0;
      for (k = 0; k < l; k++) {
        c[i][j] = c[i][j] + a[i][k] * b[k][j];
      }
    }
  return c;
}

int **trans_matrix(int n, int m, int **a) {
  int i, j;
  int **b;
  b = alloc_matrix(m, n);
  for (j = 0; j < m; j++)
    for (i = 0; i < n; i++) {
      b[j][i] = a[i][j];
    }
  return b;
}

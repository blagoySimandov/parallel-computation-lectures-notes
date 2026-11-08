#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int MPI_Sort_direct(int n, double *array, int root, MPI_Comm comm);
int MPI_Sort_bucket(int n, double *a, double max, int root, MPI_Comm comm);
int MPI_Sort_odd_even(int n, double *a, int root, MPI_Comm comm);
void print_array(int n, double *array);
int MPI_Exchange(int n, double *a, int rank1, int rank2, MPI_Comm comm);
int MPI_Issorted(int n, double *array, int root, MPI_Comm comm);
int MPI_Sort_all_to_all_bucket(int n, double *a, double max, int root,
                               MPI_Comm comm);

double *generate_array(int n, int max) {
  double *array = (double *)calloc(n, sizeof(double));
  for (int i = 0; i < n; i++) {
    array[i] = rand() / (double)RAND_MAX * max;
  }
  return array;
}

int main(int argc, char **argv) {
  int rank, size;
  int root = 0;
  int n = 1000;
  double max = 10.0; // max possible value to be present in the generated array
  double *array = generate_array(n, max);
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  // MPI_Sort_direct(n, array, root, MPI_COMM_WORLD);
  // MPI_Sort_odd_even(n, array, root, MPI_COMM_WORLD);
  MPI_Sort_all_to_all_bucket(n, array, max, root, MPI_COMM_WORLD);
  if (rank == root)
    print_array(n, array);

  free(array);
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
  free(c);
  return;
}

// MPI Functions
int MPI_Sort_direct(int n, double *array, int root, MPI_Comm comm) {
  int rank, size;
  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);
  // scatter array to localArray with double elements
  int remaining = n % size;
  int *sendcount = calloc(size, sizeof(int));
  int *displs = calloc(size, sizeof(int));
  // each rank needs at most one extra element (if they all need one extra
  // then %size would be zero)
  for (int i = 0; i < size; i++) {
    sendcount[i] = n / size;
    if (i < remaining)
      sendcount[i]++;
    if (i == 0)
      displs[i] = 0;
    else
      displs[i] = displs[i - 1] + sendcount[i - 1];
  }

  // allocate space as much as we are sending
  double *localArray = (double *)calloc(sendcount[rank], sizeof(double));

  int rc = MPI_Scatterv(array, sendcount, displs, MPI_DOUBLE, localArray,
                        sendcount[rank], MPI_DOUBLE, root, comm);
  if (rc != 0) {
    free(localArray);
    free(sendcount);
    free(displs);
    return rc;
  }
  // bubble_sort(n / size, localArray);
  merge_sort(sendcount[rank], localArray);

  // i send sendcount[rank] while root receives sendcount(array) elements with
  // different displs
  rc = MPI_Gatherv(localArray, sendcount[rank], MPI_DOUBLE, array, sendcount,
                   displs, MPI_DOUBLE, root, comm);

  // sort localArray
  // gather localArray to array with double elements
  if (rank == root) {
    // merge the size chunks of array
    // print array
    for (int i = 1; i < size; i++) {
      int prefix = displs[i];
      double *tmp = merge_array(prefix, array, sendcount[i], array + prefix);
      memcpy(array, tmp, (prefix + sendcount[i]) * sizeof(double));
      free(tmp);
    }

    // for (int i = 0; i < n; i++) {
    //   printf("%f ", array[i]);
    // }
    // printf("\n");
  }
  free(localArray);
  free(sendcount);
  free(displs);
  return MPI_SUCCESS;
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
    int bindex = (int)(a[i] * size / max);
    if (bindex >= size) // happens when val is equal to max
      bindex = size - 1;
    if (bindex == rank)
      bucket[count++] = a[i];
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
  free(bucket);
  free(recvcounts);
  free(displs);
  return MPI_SUCCESS;
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
    // we don't need the barrier for the correctness
    // Sendrecv will block correctly even if ranks drift apart.
    // MPI_Barrier(comm);
    // check sort and stop early
    if (MPI_Issorted(n / size, locala, 0, comm)) {
      break;
    }
  }
  MPI_Gather(locala, n / size, MPI_DOUBLE, a, n / size, MPI_DOUBLE, root, comm);
  free(locala);
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

int MPI_Sort_all_to_all_bucket(int n, double *a, double max, int root,
                               MPI_Comm comm) {
  // get rank and size
  int rank, size;
  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);
  // scatterv elements
  int *sendcount = (int *)calloc(size, sizeof(int));
  int *displs = (int *)calloc(size, sizeof(int));
  int remain = n % size;
  for (int i = 0; i < size; i++) {
    sendcount[i] = n / size + (i < remain ? 1 : 0); // evenly distribute +1s
    if (i == 0)
      displs[i] = 0;
    else
      displs[i] = displs[i - 1] + sendcount[i - 1];
  }

  int localN = sendcount[rank];
  double *localArray = (double *)calloc(localN, sizeof(double));

  MPI_Scatterv(a, sendcount, displs, MPI_DOUBLE, localArray, localN, MPI_DOUBLE,
               root, comm);

  merge_sort(localN, localArray);

  // Put each block into  at most size smaller buckets and do all to all
  // communication
  int *bucketCounts = (int *)calloc(size, sizeof(int));
  for (int i = 0; i < localN; i++) {
    int bindex = (int)(localArray[i] * size / max);
    if (bindex >= size) // happens if val == max
      bindex = size - 1;
    bucketCounts[bindex]++;
  }
  int *bucketSendDispls = (int *)calloc(size, sizeof(int));

  for (int i = 0; i < size; i++) {
    if (i == 0)
      bucketSendDispls[i] = 0;
    else
      bucketSendDispls[i] = bucketSendDispls[i - 1] + bucketCounts[i - 1];
  }
  int *bucketRecvs = (int *)calloc(size, sizeof(int));
  MPI_Alltoall(bucketCounts, 1, MPI_INT, bucketRecvs, 1, MPI_INT,
               comm); // send bucketCounts to everybody so they know how many
                      // items to recv
  int *recvDispls = (int *)calloc(size, sizeof(int));
  for (int i = 0; i < size; i++) {
    if (i == 0) {
      recvDispls[i] = 0;
    } else {
      recvDispls[i] = recvDispls[i - 1] + bucketRecvs[i - 1];
    }
  }

  // SEND each bucket's elements to the corresponding rank
  int bucketN = recvDispls[size - 1] + bucketRecvs[size - 1];
  double *bucket = (double *)calloc(bucketN, sizeof(double));
  MPI_Alltoallv(localArray, bucketCounts, bucketSendDispls, MPI_DOUBLE, bucket,
                bucketRecvs, recvDispls, MPI_DOUBLE, comm);

  // Sort local bucket
  merge_sort(bucketN, bucket);

  // Gather all to root.
  // First see how big is each bucket
  int *count = (int *)calloc(size, sizeof(int));
  MPI_Gather(&bucketN, 1, MPI_INT, count, 1, MPI_INT, root, comm);

  if (rank == root) {
    displs[0] = 0;
    for (int i = 1; i < size; i++) {
      displs[i] = displs[i - 1] + count[i - 1];
    }
  }

  MPI_Gatherv(bucket, bucketN, MPI_DOUBLE, a, count, displs, MPI_DOUBLE, root,
              comm);
  free(sendcount);
  free(displs);
  free(localArray);
  free(bucketCounts);
  free(bucketSendDispls);
  free(bucketRecvs);
  free(recvDispls);
  free(bucket);
  free(count);
  return MPI_SUCCESS;
}

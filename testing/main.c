#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[]) {
  int rank, size;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  srand(time(NULL) + rank);
  int moveMe = 0;
  int moveOp;
  MPI_Status status;
  if (rank == 0) {
    MPI_Send(&moveMe, 1, MPI_INT, 1, 1, MPI_COMM_WORLD);
    MPI_Recv(&moveOp, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);
  }
  if (rank == 1) {
    MPI_Recv(&moveOp, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);
    MPI_Send(&moveMe, 1, MPI_INT, 1, 1, MPI_COMM_WORLD);
  }
}

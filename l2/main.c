#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int test(int move1, int move2) {
  char *rps = "RPS";
  char cMove1 = rps[move1], cMove2 = rps[move2];

  if (cMove1 == cMove2) {
    return 2;
  } else if ((cMove1 == 'R' && cMove2 == 'S') ||
             (cMove1 == 'S' && cMove2 == 'P') ||
             (cMove1 == 'P' && cMove2 == 'R')) {
    return 0;
  }
  return 1;
}

int main(int argc, char *argv[]) {
  // Initialize the MPI environment
  MPI_Init(&argc, &argv);
  int world_size;
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  // local data  for rank
  srand(rank + (unsigned int)time(NULL));
  int tag1 = 1;
  int move1 = rand() % 3;
  int move2;
  int winner = 1;
  MPI_Status status;
  if (rank == 0) {
    MPI_Send(&move1, 1, MPI_INT, 1, tag1, MPI_COMM_WORLD);
    MPI_Recv(&move1, 1, MPI_INT, 1, tag1, MPI_COMM_WORLD, &status);
    winner = test(move1, move2);
  } else if (rank == 1) {
    MPI_Recv(&move1, 1, MPI_INT, 0, tag1, MPI_COMM_WORLD, &status);
    MPI_Send(&move1, 1, MPI_INT, 0, tag1, MPI_COMM_WORLD);
    winner = test(move2, move1);
  }
  printf("RPS Game on %d processor %d is winner \n", rank, winner);

  MPI_Finalize();
  return 0;
}

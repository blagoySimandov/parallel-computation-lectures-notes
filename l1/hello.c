#include "mpi.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
  int rank, size;
  int i, namelen;
  char processor_name[MPI_MAX_PROCESSOR_NAME];

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  // MPI_Get_processor_name(processor_name,&namelen);

  // printf("called on %s\n",processor_name);

  printf("Hello world from process %d of %d\n", rank, size);

  MPI_Finalize();
  return 0;
}

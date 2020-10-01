#include <mpi.h>
#include <stdio.h>
int main(int argc, char **argv) {
  int size;
  int rang;
  MPI_Status status;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rang);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  printf("Processus %d sur %d : Hello MPI\n", rang, size);
  MPI_Finalize();
  return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

//************   LES TAGS MESSAGES
#define REQUEST   0
#define REPLY     1
#define DONE      2

//************   LES ETATS D'UN NOEUD
#define REQUESTING        0
#define NOT_REQUESTING    1
#define CRITICAL_SECTION  2

#define MAX_CS    2

//************   LES VARIABLES MPI
int rank;
int NB;

//************   LES VARIBLES LOCALES
int state = 1;
int nbCS = 0;
int nbDONE = 0;
int local_clock = 1;
int msgRecu[3];
int ** file;
int tailleFile = 0;

//************   LA TEMPORISATION
int max(int a, int b) {
   return (a>b?a:b);
}

void updateclock() {
  local_clock++;
  local_clock = max(msgRecu[2], local_clock) + 1 ;
}

//************   LES MESSAGES
void receive_message(MPI_Status *status) {
	// A completer
  MPI_Recv(msgRecu, 3, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, status);
}

void send_message(int dest, int tag) {
	 // A completer
   int msg[3];
   msg[0] = tag;
   msg[1] = local_clock;
   msg[2] = rank;
   MPI_Send(msg, 3, MPI_INT, dest, 99, MPI_COMM_WORLD);
}

void broadcast_message(int tag) {
  int msg[3];
  msg[0] = tag;
  msg[1] = local_clock;
  msg[2] = rank;
  MPI_Bcast(msg, 3, MPI_INT, rank, MPI_COMM_WORLD);
}

//************   GESTION FILE

void initFile() {
    file = (int **) malloc(NB * sizeof(int *));
    int i;
    for (i = 0; i < NB; i++) {
      file[i] = (int *) malloc (2 * sizeof(int));
    }
}

void freeFile() {
    int i;
    for(i = 0; i < NB; i++) free(file[i]);
    free(file);
}

//************   LE PROGRAMME   ***************************
int main(int argc, char* argv[]) {

   MPI_Status status;

   MPI_Init(&argc, &argv);
   MPI_Comm_size(MPI_COMM_WORLD, &NB);
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);

   initFile();

   while (nbDONE < NB) {
     if (nbCS < MAX_CS) {
       broadcast_message(REQUEST);
       printf("P%d> Asking for CS\t at %d\n", rank, local_clock);
       state = REQUESTING;
     }
     receive_message(&status);
     updateclock();

     if (msgRecu[0] == REQUEST) {
       printf("P%d> %d Asking for chopsticks\t at %d\n", status.MPI_SOURCE, rank, local_clock);
     }


   }

   freeFile();

   MPI_Finalize();
   return 0;
}

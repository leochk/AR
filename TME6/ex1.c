#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <mpi.h>

//************   LES TAGS
#define OUT         0
#define IN          1
#define NOTIFY      3

//************   LES ETATS D'UN NOEUD
#define INITIATEUR  0
#define LEADER      1
#define NONE        2

//************   LES INDEX DES MESSAGES
#define EMETTEUR    0
#define INIT        1
#define TAGMSG      2
#define DISTANCE    3

//************   LES VARIABLES MPI
int NB;               // nb total de processus
int rank;             // mon identifiant
int *graph;           // l'anneau
int msg[4];           // message envoyé
int msgrec[4];        // message recu
int k = 0;
//************   LES ETATS LOCAUX
int etat = NONE;
int leader = -1;

//************   LES FONCTIONS   ***************************

/**
*   Generate a random ring graph, so that for each proc i, proc array[i] is his
*   next node in the graph.
*/
void createAnneau(int *array, size_t n) {
    if (n > 1) {
        size_t i;
        for (i = 0; i < n - 1; i++) {
          size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
          if (i == j) {
            i--;
            continue;
          }
          int t = array[j];
          array[j] = array[i];
          array[i] = t;
        }
    }
}

/**
*  Compute x^y
*/
int mypow (int x, int y)
{
    if (y == 0)
        return 1;
    else if ((y % 2) == 0)
        return mypow (x, y / 2) * mypow (x, y / 2);
    else
        return x * mypow (x, y / 2) * mypow (x, y / 2);
}

/**
*   Initialize random candidates for election.
*   Then, proc 0 is charged of generating and broadcasting the ring graph to
*   all other proc.
*/
void init() {
  int i;

  srand(getpid());
  if (rand()%2 == 0) {
    etat = INITIATEUR;
    printf("P%d initiateur\n", rank);
  }
  graph = malloc(NB*sizeof(int));

  if (rank == 0) {
    for (i = 0; i < NB; i++) {
      graph[i] = i;
    }
    createAnneau(graph, NB);

    for (i = 0; i < NB; i++) {
      printf("P%d a pour successeur P%d\n", i, graph[i] );
    }
  }
  MPI_Bcast(graph, NB, MPI_INT, 0, MPI_COMM_WORLD);
}

/**
*   Function to make easier communication between proc
*/
void send_message(int emetteur, int initiateur, int tagmsg, int distance) {
  if (tagmsg == OUT)
    printf("P%d, k = %d : send < %d, %d, OUT, %d > \n", rank, k, emetteur, initiateur, distance);
  else if (tagmsg == IN)
    printf("P%d, k = %d : send < %d, %d, IN,  %d > \n", rank, k, emetteur, initiateur, distance);
  else
    printf("P%d, k = %d : send < %d, %d, NOTIFY,  %d > \n", rank, k, emetteur, initiateur, distance);

  msg[EMETTEUR]   = emetteur;
  msg[INIT] = initiateur;
  msg[TAGMSG]     = tagmsg;
  msg[DISTANCE]   = distance;
  MPI_Send(msg, 4, MPI_INT, graph[rank], 0, MPI_COMM_WORLD);

}

/**
*   Function to make easier communication between proc
*/
void receive_message(MPI_Status *status) {
    MPI_Recv(msgrec, 4, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, status);
}

/**
*   k gestion
*/
void initier_etape(int *k) {
    if (*k == 0) {
        etat = INITIATEUR;
    }
    leader = -1;
    send_message(rank, rank, OUT, mypow(2,*k));
    (*k)++;
}

/**
*   Every time proc receive an OUT message, he calls this function
*/
void out_treat() {
    printf("P%d, k = %d : rece < %d, %d, OUT, %d > \n", rank, k, msgrec[EMETTEUR], msgrec[INIT], msgrec[DISTANCE]);
    if (etat != INITIATEUR || msgrec[INIT] > rank) {
        etat = NONE;
        if (msgrec[DISTANCE] > 1) {
            send_message(rank, msgrec[INIT], OUT, msgrec[DISTANCE] - 1);
        } else {
            send_message(rank, msgrec[INIT], IN, -1);
        }
     } else {
      if (msgrec[INIT] == rank) {
          etat = LEADER;
          leader = rank;
          printf("P%d : JE SUIS CHEF\n", rank );
          send_message(rank, rank, NOTIFY, NB);
      } else {
        printf("P%d, k = %d : P%d NE PEUT PLUS ÊTRE ÉLU\n", rank, k, msgrec[INIT]);

      }
    }
}

/**
*   Every time proc receive an IN message, he calls this function
*/
void in_treat() {
    printf("P%d, k = %d : rece < %d, %d, IN,  %d > \n", rank, k, msgrec[EMETTEUR], msgrec[INIT], msgrec[DISTANCE]);
    if (msgrec[INIT] != rank) {
        send_message(rank, msgrec[INIT], IN, msgrec[DISTANCE]);
    } else {
        initier_etape(&k);
    }
}

/**
*   Every time proc receive an NOTIFY message, he calls this function
*/
void notify_treat() {
    printf("P%d, k = %d : rece < %d, %d, NOTIFY,  %d > : %d EST CHEF\n", rank, k, msgrec[EMETTEUR], msgrec[INIT], msgrec[DISTANCE], msgrec[INIT]);
    leader = msg[INIT];
    send_message(rank, msgrec[INIT], NOTIFY, msgrec[INIT] - 1);
    printf("P%d : ON A NOTRE CHEF!! VIVE P%d !!\n", rank, leader);
}

//************   LE PROGRAMME   ***************************
int main(int argc, char* argv[]) {

   MPI_Status status;

   MPI_Init(&argc, &argv);
   MPI_Comm_size(MPI_COMM_WORLD, &NB);
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);

   init();

   if (etat == INITIATEUR)
      initier_etape(&k);

   while (1) {
      receive_message(&status);

      if (msgrec[TAGMSG] == IN) {
          in_treat();
      } else if (msgrec[TAGMSG] == OUT) {
          out_treat();
      } else {
        notify_treat();
        break;
      }

   }
   MPI_Finalize();
   return 0;
}

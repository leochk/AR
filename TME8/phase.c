#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define TAGINIT     0
#define TAGPHASE    1

#define NB_SITE     6

#define DIAMETRE    5		/* !!!!! valeur a initialiser !!!!! */

int rang;


/**
*   Check if proc has finished protocol
*/
int notFinished(int * RCpt, int nb_voisins_in) {
    int i;

    for (i = 0; i < nb_voisins_in; i++) {
        if (RCpt[i] < DIAMETRE) {
            return 1;
        }
    }
    return 0;
}

/**
*   Check if proc can send message to its neighbors
*/
int canSend(int * RCpt, int nb_voisins_in, int SCpt) {
    int i;

    if (SCpt >= DIAMETRE) return 0;
    for (i = 0; i < nb_voisins_in; i++) {
        if (RCpt[i] < SCpt) return 0;
    }
    return 1;
}

/**
*   Send message to neighbor
*/
int sendMessage(int * voisins_out, int nb_voisins_out, int min_local, int rang) {
    int i;
    int msg[2];
    msg[0] = min_local;
    msg[1] = rang;
    for (i = 0; i < nb_voisins_out; i++) {
        MPI_Send(msg, 2, MPI_INT, voisins_out[i], TAGPHASE, MPI_COMM_WORLD);
    }
}

void simulateur(void) {
   int i;

   /* nb_voisins_in[i] est le nombre de voisins entrants du site i */
   /* nb_voisins_out[i] est le nombre de voisins sortants du site i */
   int nb_voisins_in[NB_SITE+1] = {-1, 2, 1, 1, 2, 1, 1};
   int nb_voisins_out[NB_SITE+1] = {-1, 2, 1, 1, 1, 2, 1};

   int min_local[NB_SITE+1] = {-1, 4, 7, 1, 6, 2, 9};

   /* liste des voisins entrants */
   int voisins_in[NB_SITE+1][2] = {{-1, -1},
				{4, 5}, {1, -1}, {1, -1},
				{3, 5}, {6, -1}, {2, -1}};

   /* liste des voisins sortants */
   int voisins_out[NB_SITE+1][2] = {{-1, -1},
				{2, 3}, {6, -1}, {4, -1},
				{1, -1}, {1, 4}, {5,-1}};

   for(i=1; i<=NB_SITE; i++){
      MPI_Send(&nb_voisins_in[i], 1, MPI_INT, i, TAGINIT, MPI_COMM_WORLD);
      MPI_Send(&nb_voisins_out[i], 1, MPI_INT, i, TAGINIT, MPI_COMM_WORLD);
      MPI_Send(voisins_in[i], nb_voisins_in[i], MPI_INT, i, TAGINIT, MPI_COMM_WORLD);
      MPI_Send(voisins_out[i], nb_voisins_out[i], MPI_INT, i, TAGINIT, MPI_COMM_WORLD);
      MPI_Send(&min_local[i], 1, MPI_INT, i, TAGINIT, MPI_COMM_WORLD);
   }
}

void calcul_min(int rang) {
    MPI_Status status;
    int     i;
    int     msg[2];
    int     nb_voisins_in, nb_voisins_out, min_local;
    // min est le minimum du système
    int minSysteme;
    int *   voisins_in;
    int *   voisins_out;
    int *   RCpt;
    int     SCpt;

    /*INITIALISATION PAR LE SIMULATEUR =======================================*/

    MPI_Recv(msg, 1, MPI_INT, MPI_ANY_SOURCE, TAGINIT, MPI_COMM_WORLD, &status);
    nb_voisins_in = msg[0];

    MPI_Recv(msg, 1, MPI_INT, MPI_ANY_SOURCE, TAGINIT, MPI_COMM_WORLD, &status);
    nb_voisins_out = msg[0];

    voisins_in =    (int *) malloc(sizeof(int)*(nb_voisins_in));
    RCpt =          (int *) malloc(sizeof(int)*(nb_voisins_in));
    for (i = 0; i < nb_voisins_in; i++) RCpt[i] = 0;

    MPI_Recv(voisins_in, nb_voisins_in, MPI_INT, MPI_ANY_SOURCE, TAGINIT, MPI_COMM_WORLD, &status);

    voisins_out =   (int *) malloc(sizeof(int)*(nb_voisins_out));
    MPI_Recv(voisins_out, nb_voisins_out, MPI_INT, MPI_ANY_SOURCE, TAGINIT, MPI_COMM_WORLD, &status);

    MPI_Recv(msg, 1, MPI_INT, MPI_ANY_SOURCE, TAGINIT, MPI_COMM_WORLD, &status);
    min_local = msg[0];
    minSysteme = min_local;
    printf("P%d : nb_voisins_in = %d, nb_voisins_out = %d, min_local = %d\n",
        rang, nb_voisins_in, nb_voisins_out, min_local);

    /* ALGORITHME DE PHASE ===================================================*/

    // tant que le proc peut participer au protocol phase
    while(notFinished(RCpt, nb_voisins_in)) {
        // il vérifie s'il peut envoyer des messages, c'est à dire s'il n'est
        // pas en avance par rapport à ses voisins in
        if (canSend(RCpt, nb_voisins_in, SCpt)) {
            // envoie du messages aux voisins out, et incrémente son compteur de
            // message envoyé
            sendMessage(voisins_out, nb_voisins_out, minSysteme, rang);
            SCpt++;
        }
        // attente de réception d'un message d'un voisin in
        MPI_Recv(msg, 2, MPI_INT, MPI_ANY_SOURCE, TAGPHASE, MPI_COMM_WORLD, &status);
        int minRecu = msg[0];
        int rangEmetteur = msg[1];
        printf("P%d : <%d, %d>\n", rang, minRecu, rangEmetteur);
        // trouve l'index de l'émetteur du voisin
        for (i = 0; i < nb_voisins_in; i++) {
            // pour incrémenter le nombre de message recu par celui ci
            if (voisins_in[i] == rangEmetteur) {
                RCpt[i]++;
            }
        }
        // si le message contient un minimum plus petit que mon min local, MAJ
        if (minRecu < minSysteme) //min = minRecu;
            minSysteme = minRecu;
    }

    // Fin : le proc affiche le min du systeme
    printf("P%d : minimum est %d\n", rang, minSysteme);

}

/******************************************************************************/

int main (int argc, char* argv[]) {
   int nb_proc;
   MPI_Init(&argc, &argv);
   MPI_Comm_size(MPI_COMM_WORLD, &nb_proc);

   if (nb_proc != NB_SITE+1) {
      printf("Nombre de processus incorrect !\n");
      MPI_Finalize();
      exit(2);
   }

   MPI_Comm_rank(MPI_COMM_WORLD, &rang);

   if (rang == 0) {
      simulateur();
   } else {
      calcul_min(rang);
   }

   MPI_Finalize();
   return 0;
}

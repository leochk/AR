#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <mpi.h>

#define TAGINIT    0
#define NB_SITE 6

/* nb_voisins[i] est le nombre de voisins du site i */
int nb_voisins[NB_SITE+1] = {-1, 2, 3, 2, 1, 1, 1};
int min_local[NB_SITE+1] = {-1, 3, 11, 8, 14, 5, 17};

/* liste des voisins */
int voisins[NB_SITE+1][3] = {
    {-1, -1, -1},
    {2, 3, -1},
    {1, 4, 5},
    {1, 6, -1},
    {2, -1, -1},
    {2, -1, -1},
    {3, -1, -1}
};

//************   LES TAGS
#define REVEIL      0
#define IDENT       1

//************   LES ETATS D'UN NOEUD
#define INITIATEUR  0
#define LEADER      1
#define NONE        2

//************   LES INDEX DES MESSAGES
#define EMETTEUR    0
#define INIT        1
#define TAGMSG      2
#define MIN         3

//************   LES VARIABLES MPI
int rank;             // mon identifiant
int msg[4];           // message envoyé
int msgrec[4];        // message recu

//************   LES ETATS LOCAUX
int reveil_first = 0;
int etat = NONE;
int leader = -1;
int nbIdent = 0;
int identFromVoisin[3] = {-1, -1, -1};      // valeurs des min des voisins lors de la reception d'un IDENT
int identRank[3]       = {-1, -1, -1};      // rank des proc correspondant au min de identFromVoisin
int chef = 0;                               // chef élu
//************   LES FONCTIONS   ***************************

/**
*   Initialize random candidates for election.
*
*/
void init() {
  int i;
  MPI_Status * status;
  srand(getpid());
  if (rand()%2 == 0 && rank != 0) {
    etat = INITIATEUR;
    printf("P%d initiateur : min = %d\n", rank, min_local[rank]);
  }
}

/**
*   Function to make easier communication between proc
*/
void send_message(int emetteur, int initiateur, int tagmsg, int min, int destinataire) {

    msg[EMETTEUR] = emetteur;
    msg[INIT]     = initiateur;
    msg[TAGMSG]   = tagmsg;
    msg[MIN]      = min;

    if (tagmsg == IDENT)
         printf("P%d : send < %d, %d, I, %d > to %d \n", rank, emetteur, initiateur, min, destinataire);
    else if (tagmsg == REVEIL)
        printf("P%d : send < %d, %d, R, %d > to %d \n", rank, emetteur, initiateur, min, destinataire);

    MPI_Send(msg, 4, MPI_INT, destinataire, 0, MPI_COMM_WORLD);
}

/**
*   Function to make easier communication between proc
*/
void receive_message(MPI_Status *status) {
    MPI_Recv(msgrec, 4, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, status);
}

/**
*   Return the index of the minimal positive value in array
*/
int indexOfMinInArray(int * array, int size) {
    int i;
    int res = 0;
    int min = array[0];

    for (i = 0; i < size; i++) {
        if (array[i] < min && array[i] > 0) {
            min = array[i];
            res = i ;
        }
    }

    return res;
}

/**
*   Return the index corresponding to the inserted integer in array
*/
int indexOfIntInArray(int * array, int size, int toFound) {
    int i;
    for (i = 0; i < size; i++)
        if (array[i] == toFound)
            return i;
    return -1;
}

void reveil_treat() {
    //printf("P%d : rece < %d, %d, R, %d > \n", rank, msgrec[EMETTEUR], msgrec[INIT], msgrec[MIN]);
    int i;

    //Si on a pas été réveillé
    if (reveil_first == 0) {
        reveil_first++;

        //Si je suis une feuille, j'envoie directement un message IDENT
        if (nb_voisins[rank] == 1) {
            int minimum = msgrec[MIN];
            int init    = msgrec[INIT];

            //Si je suis INITIATEUR, je teste si mon min est plus petit
            if (etat == INITIATEUR && min_local[rank] < minimum) {
                minimum = min_local[rank];
                init    = rank;
            }
            send_message(rank, init, IDENT, minimum, voisins[rank][0]);
            return;
        }

        //Sinon, je réveille tous mes voisins sauf celui qui m'a reveillé
        for (i = 0; i < nb_voisins[rank]; i++)
            if (msgrec[EMETTEUR] != voisins[rank][i])
                send_message(rank, msgrec[INIT], msgrec[TAGMSG], msgrec[MIN], voisins[rank][i]);
    }
}

void ident_treat() {
    //printf("P%d : rece < %d, %d, I, %d > \n", rank, msgrec[EMETTEUR], msgrec[INIT], msgrec[MIN]);
    int i;
    //Récup l'index de l'emetteur dans le tableau des voisins
    int indexEmetteur = indexOfIntInArray(voisins[rank], nb_voisins[rank], msgrec[EMETTEUR]);
    //Si j'ai déjà recu IDENT de cet emetteur, je ne compte pas
    if (identRank[indexEmetteur] == -1)
        nbIdent++;

    //Affectation des valeurs dans les tableaux au bon index
    identFromVoisin[indexEmetteur] = msgrec[MIN];
    identRank[indexEmetteur]       = msgrec[INIT];

    //Si je suis INITIATEUR, je teste si mon min est plus petit
    int minimum =   (etat == INITIATEUR) ?
                    min_local[rank]: identFromVoisin[indexEmetteur];
    int init =      (etat == INITIATEUR) ?
                    rank           : identRank[indexEmetteur];

    for (i = 0; i < 3; i++) {
        int tmp = identFromVoisin[i];
        if (tmp < minimum && tmp > 0) {
            minimum = tmp;
            init = identRank[i];
        }
    }

    //Si on a recu IDENT de tous les voisins sauf 1
    if (nbIdent == nb_voisins[rank] - 1) {
        //Envoie du minimum au voisin qui n'a pas envoyé IDENT
        int index = indexOfIntInArray(identRank, 3, -1);
        send_message(rank, init, IDENT, minimum, voisins[rank][index]);

    } // Sinon, si on a recu IDENT de tous les voisins
    else if (nbIdent == nb_voisins[rank]) {
        //Envoie du minimum à tous les voisins sauf celui qui m'a envoyé IDENT
        for (i = 0; i < nb_voisins[rank]; i++)
            if (voisins[rank][i] != msgrec[EMETTEUR])
                send_message(rank, init, IDENT, minimum, voisins[rank][i]);
        //Je connais le chef
        chef = init;
    }

}

/******************************************************************************/

int main (int argc, char* argv[]) {
    int nb_proc,rang;
    MPI_Status * status;
    int i;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nb_proc);

    if (nb_proc != NB_SITE+1) {
       printf("Nombre de processus incorrect !\n");
       MPI_Finalize();
       exit(2);
    }

    if (rank == 0) {
        MPI_Finalize();
        return 0;
    }

    init();

    if (etat == INITIATEUR) {
        //Si je suis une feuille et initiateur, j'envoie IDENT
        if (nb_voisins[rank] == 1) {
            send_message(rank, rank, IDENT, min_local[rank], voisins[rank][0]);
        } else { //Sinon, je réveille tous mes voisins en envoyant REVEIL
            for (i = 0; i < nb_voisins[rank]; i++)
                send_message(rank, rank, REVEIL, min_local[rank], voisins[rank][i]);
        }
    }

    while(chef == 0) { // Tant qu'on a pas élu un chef
        receive_message(status);

        if (msgrec[TAGMSG] == IDENT)
            ident_treat();
        else if (msgrec[TAGMSG] == REVEIL)
            reveil_treat();

        if (chef != 0) {
            if (chef == rank) {
                etat = LEADER;
                printf("P%d : JE SUIS CHEF !! \n", rank);
            } else {
                printf("P%d : %d EST NOTRE CHEF !! \n", rank, chef);
            }
        }
    }
    
    MPI_Finalize();
    return 0;
}

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

#define M 6
#define NB_SITE 10

//************   TAGS
#define TAGINIT     0
#define TAGQUERYID  1
#define TAGFINGER   2

//************   NODE STATE
#define INITIATEUR  1
#define NONE        0

int rank;
int showMessage = 0;
//************   FUNCTIONS

/*
*   power integer
*/
int ipow(int base, int exp) {
    int result = 1;
    for (;;)
    {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        if (!exp)
            break;
        base *= base;
    }

    return result;
}

/*
*   Compute if k in [a, b[
*/
int app(int k, int a, int b) {
    int range = pow(2, M);
    if (a < b) return ((k >= a) && (k < b));
    return (((k >= 0) && (k < b)) || ((k >= a) && (k < range)));
}


/*
*   Cyclic order
*/
int lowerThan(int a, int b) {
    int K = ipow(2, M);
    return app((b - a)%(K-1), 0, K/2 + 1);
}

/**
*   Generate a ring graph, so that for each proc i, proc array[(i+1)%NB_SITE] is
*   its previous neighbor and array[i][(i-1)%NB_SITE] its next neighbor in the
*   graph.
*/
void createAnneau(int * array) {
    int i;
    for (i = 0; i < NB_SITE; i++) array[i] = i;
}

/*
*   Compare integer
*/
int compare( const void* a, const void* b) {
     int int_a = * ( (int*) a );
     int int_b = * ( (int*) b );

     if ( int_a == int_b ) return 0;
     else if ( int_a < int_b ) return -1;
     else return 1;
}

void sort(int * array, int size) {
    int i;
    qsort( array, NB_SITE, sizeof(int), compare);

}
/*
*   Compute unique random CHORD ID for each site in inputted array
*/
void randomSelectIdChord(int * array) {
    int range = ipow(2, M) - 1;
    unsigned char is_used[range];
    int i;

    for (i = 0; i < range; i++) is_used[i] = 0;
    for (i = 0; i < NB_SITE; i++) {
        int r = rand() % range;
        while (is_used[r]) {
            r = rand() % range;
        }
        array[i] = r;
        is_used[r] = 1;
    }
/*
    array[0] = 2;
    array[1] = 7;
    array[2] = 13;
    array[3] = 14;
    array[4] = 21;
    array[5] = 38;
    array[6] = 42;
    array[7] = 48;
    array[8] = 51;
    array[9] = 59;

    array[9] = 2;
    array[8] = 7;
    array[7] = 13;
    array[6] = 14;
    array[5] = 21;
    array[4] = 38;
    array[3] = 42;
    array[2] = 48;
    array[1] = 51;
    array[0] = 59;
*/
}

/*
*   Election of n > 1 initiators
*   Return the number of initiators
*/
int election(int * is_elected) {
    int i;
    int n = 1 + rand()%(NB_SITE - 1);

    for (i = 0; i < NB_SITE; i++) is_elected[i] = 0;

    for (i = 0; i < n; i++) {
        int r = rand() % NB_SITE;
        while (is_elected[r]) {
            r = rand() % NB_SITE;
        }
        is_elected[r] = 1;
    }
    return n;
}

/*
*   Compute finger tables
*/
void computeFingerTable(int fingerTable[][M], int * idChord, int rank, int id) {
    int i, j;
    int range = ipow(2, M) - 1;
    // we need sorted id chord list to compute fingers
    int idChordSorted[NB_SITE];
    for (i = 0; i < NB_SITE; i++) idChordSorted[i] = idChord[i];
    sort(idChordSorted, NB_SITE);

    for (i = 0; i < M; i++) {
        // we compute tmp2 = <chord_id> + 2^<i> % <M>
        int tmp = id + ipow(2, i);
        int tmp2 = tmp % range;

        // and we look for the minimal chord id superior than tmp and its rank
        fingerTable[rank][i] = idChordSorted[0];
        for (j = 0; j < NB_SITE; j++) {
            if (lowerThan(tmp2, idChordSorted[j])) {
                fingerTable[rank][i] = idChordSorted[j];
                break;
            }
        }

    }
}

void simulateur() {
    int i;
    int ringGraph[NB_SITE];
    int idChord[NB_SITE];
    int elected[NB_SITE];
    int msg[1];

    /* Generating the system (graph, id chord, election initiators)*/
    createAnneau(ringGraph);
    randomSelectIdChord(idChord);
    election(elected);

    /* Sending system configuration to proc*/
    for (i = 0; i < NB_SITE; i++) {
        MPI_Send(idChord, NB_SITE, MPI_INT, ringGraph[i], TAGINIT, MPI_COMM_WORLD);
        MPI_Send(ringGraph+((NB_SITE + i-1)%NB_SITE), 1, MPI_INT, ringGraph[i],
            TAGINIT, MPI_COMM_WORLD);
        MPI_Send(ringGraph+((NB_SITE + i+1)%NB_SITE), 1, MPI_INT, ringGraph[i],
            TAGINIT, MPI_COMM_WORLD);
        msg[0] = elected[i];
        MPI_Send(msg, 1, MPI_INT, ringGraph[i], TAGINIT, MPI_COMM_WORLD);
    }
}

void chord(MPI_Status status) {
    int i, j;
    int id, prev, next, state;
    int idChord[NB_SITE];
    int myfinger[M];
    int msg[1];

    MPI_Recv(idChord, NB_SITE, MPI_INT, MPI_ANY_SOURCE, TAGINIT, MPI_COMM_WORLD, &status);
    id = idChord[rank];
    MPI_Recv(msg, 1, MPI_INT, MPI_ANY_SOURCE, TAGINIT, MPI_COMM_WORLD, &status);
    prev = msg[0];
    MPI_Recv(msg, 1, MPI_INT, MPI_ANY_SOURCE, TAGINIT, MPI_COMM_WORLD, &status);
    next = msg[0];
    MPI_Recv(msg, 1, MPI_INT, MPI_ANY_SOURCE, TAGINIT, MPI_COMM_WORLD, &status);
    if (msg[0] == 1)
        state = INITIATEUR;
    else
        state = NONE;

    //printf("P%d : %d\n", rank, id);
    printf("P%d : prev = %d | next = %d | init = %d\n", rank, prev, next, state);
    //printf("P%d : state = %d\n", rank, state);

    int msgResponsibleOf[NB_SITE];
    int msgFingerTables[NB_SITE][M];

    for (i = 0; i < NB_SITE; i++)
        msgResponsibleOf[i] = 0;
    for (i = 0; i < NB_SITE; i++) {
        for (j = 0; j < M; j++) {
            msgFingerTables[i][j] = -1;
        }
    }


    if (state == INITIATEUR) {
        /*
        *   Send a message TAGQUERYID to next proc
        *   If the next proc is non initiator, it will set msgResponsibleOf[next]
        *   at 1 and spread the message to its next proc.
        *   If the next proc is initiator, it will compute finger tables of
        *   itself and of all proc i if msgResponsibleOf[i] == 1. The message
        *   is finally destroyed.
        */
        MPI_Send(msgResponsibleOf, NB_SITE, MPI_INT, next, TAGQUERYID, MPI_COMM_WORLD);
        if (showMessage) printf("P%d : send TAGQUERYID to %d\n", rank, next);

        // to make sure each initiator treat a TAGQUERYID and a TAGFINGER.
        // otherwise, it could lead to a deadlock
        int tmp = 0;
        int tmp2 = 0;
        while(1) {
            /* get tag of message */
		    MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

            /* TAGQUERYID treatment */
            if(status.MPI_TAG == TAGQUERYID) {
                if (showMessage) printf("P%d : TAGQUERYID from %d\n", rank, status.MPI_SOURCE);

			    MPI_Recv(msgResponsibleOf, NB_SITE, MPI_INT, MPI_ANY_SOURCE, TAGQUERYID, MPI_COMM_WORLD, &status);
                /* compute fingers of itself and of which it is responsible of*/
                computeFingerTable(msgFingerTables, idChord, rank, idChord[rank]);
                for (i = 0; i < NB_SITE; i++) {
                    if (msgResponsibleOf[i] == 1) {
                        computeFingerTable(msgFingerTables, idChord, i, idChord[i]);
                    }
                }
                printf("P%d : id = %d\n", rank, idChord[rank]);
                for (i = 0; i < M; i++) {
                    myfinger[i] = msgFingerTables[rank][i];
                    printf("P%d : fingers[%d] = %d\n", rank, i, myfinger[i]);
                }
                /*
                *   Send a message TAGFINGER to prev proc
                *   If the prev proc is non initiator, he will store its finger
                *   table contained in msgFingerTables[prev]. Then, it will
                *   finish its execution after spreading the message to its prev
                *   proc.
                *   If the prev proc is initiator, the message will be destroyed
                *   and proc will finish its execution.
                */
                MPI_Send(msgFingerTables, NB_SITE*M, MPI_INT, prev, TAGFINGER, MPI_COMM_WORLD);
                if (showMessage) printf("P%d : send TAGFINGER to %d\n", rank, next);

                tmp++;
            } else if (status.MPI_TAG == TAGFINGER) {
                MPI_Recv(msgFingerTables, NB_SITE*M, MPI_INT, MPI_ANY_SOURCE, TAGFINGER, MPI_COMM_WORLD, &status);
                if (showMessage) printf("P%d : TAGFINGER from %d\n", rank, status.MPI_SOURCE);

                tmp2++;
            }
            if (tmp && tmp2) break;
        }

    } else {
        while(1) {
            /* get tag of message */
		    MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

            /* TAGQUERYID treatment */
            if(status.MPI_TAG == TAGQUERYID) {
                if (showMessage) printf("P%d : TAGQUERYID from %d\n", rank, status.MPI_SOURCE);

                /* Reception, mark msgResponsibleOf[rank] at 1, and spread msg*/
			    MPI_Recv(msgResponsibleOf, NB_SITE, MPI_INT, MPI_ANY_SOURCE, TAGQUERYID, MPI_COMM_WORLD, &status);
                msgResponsibleOf[rank] = 1;
                MPI_Send(msgResponsibleOf, NB_SITE, MPI_INT, next, TAGQUERYID, MPI_COMM_WORLD);
                if (showMessage) printf("P%d : send TAGQUERYID to %d\n", rank, next);

		    }

            /* TAGFINGER treatment */
            if(status.MPI_TAG == TAGFINGER){
                if (showMessage) printf("P%d : TAGFINGER from %d\n", rank, status.MPI_SOURCE);

                /*
                *   Reception, store its finger table contained in
                *   msgFingerTables[rank], spread the message to prev proc.
                */
                MPI_Recv(msgFingerTables, NB_SITE*M, MPI_INT, MPI_ANY_SOURCE, TAGFINGER, MPI_COMM_WORLD, &status);

                printf("P%d : id = %d\n", rank, idChord[rank]);
                for (i = 0; i < M; i++) {
                    myfinger[i] = msgFingerTables[rank][i];
                    printf("P%d : fingers[%d] = %d\n", rank, i, myfinger[i]);
                }

                MPI_Send(msgFingerTables, NB_SITE*M, MPI_INT, prev, TAGFINGER, MPI_COMM_WORLD);
                if (showMessage) printf("P%d : send TAGFINGER to %d\n", rank, next);

                break;
		    }
        }
    }

}

int main (int argc, char* argv[]) {
    int nb_proc, i;
    srand (time (NULL));
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nb_proc);
    MPI_Status status;

    if (nb_proc != NB_SITE+1) {
        printf("Nombre de processus incorrect !\n");
        MPI_Finalize();
        exit(2);
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == nb_proc - 1) {
        simulateur();
    } else {
        chord(status);
   }
   MPI_Finalize();
   return 0;
}

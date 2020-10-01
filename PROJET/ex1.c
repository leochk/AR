#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

#define M 6
#define NB_SITE 10

//************   TAGS
#define ID_MSG          0
#define FINGER_MSG      1
#define FINGERRANK_MSG  2
#define LOOKUP_MSG      3

#define STEP_LOOKING    0
#define STEP_FOUND      1
#define STEP_TERM       2
//************   MESSAGE
int msgID[1];
int msgLookup[5];

//************   LES INDEX DES MESSAGES
#define EMETTEUR    0
#define INITIATEUR  1
#define DATA        2
#define RESP        3
#define STEP        4

//************   LOCAL VARIABLES
int rank;
int succ;
int id_chord;
int myfinger[M];
int myfingerRank[M];
int showFingerTable = 0;

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


/*
*   Compute unique random CHORD ID for each site in inputted array
*/
void randomSelectIdChord(int* array) {
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
    qsort( array, NB_SITE, sizeof(int), compare);
}

void simulateur(MPI_Status status) {
    // loop variables
    int curr, i, j;
    // range data value
    int range = ipow(2, M);
    // chord id's of the system
    int idchord[NB_SITE];
    randomSelectIdChord(idchord);

	for(i = 0; i < NB_SITE; i++)
        printf("idchord of P%d = %d\n", i+1, idchord[i]);

    // for each site,
    for(curr = 1; curr <= NB_SITE; curr++) {
        // we compute their chord id
        int id = idchord[curr-1];

        // and the finger table
        int finger[M];
        int fingerRank[M];
        for (i = 0; i < M; i++) finger[i] = -1;
        for (i = 0; i < M; i++) fingerRank[i] = -1;

        for (i = 0; i < M; i++) {
            // we compute tmp2 = <chord_id> + 2^<i> % <M>
            int tmp = id + ipow(2, i);
            int tmp2 = tmp % range;

            // and we look for the minimal chord id superior than tmp and its rank
            finger[i] = idchord[0];
            fingerRank[i] = 1;
            for (j = 0; j < NB_SITE; j++) {
                if (lowerThan(tmp2, idchord[j])) {
                    finger[i] = idchord[j];
                    fingerRank[i] = j+1;
                    break;
                }
            }

        }
        // finally, send to the site its chord id, its finger table and ranks
        // of each finger
        msgID[0] = id;
        MPI_Send(msgID, 1, MPI_INT, curr, ID_MSG, MPI_COMM_WORLD);
        MPI_Send(fingerRank, M, MPI_INT, curr, FINGERRANK_MSG, MPI_COMM_WORLD);
        MPI_Send(finger, M, MPI_INT, curr, FINGER_MSG, MPI_COMM_WORLD);
   }

   // at this moment, DHT CHORD is initialized
   // creating a random data request to one of the system site
   int p = rand()%NB_SITE;
   int pair = idchord[p];
   int data = rand()%range;

   printf("Simulateur asking %d for data %d\n", pair, data);
   msgLookup[EMETTEUR] = 0;
   msgLookup[INITIATEUR] = p;
   msgLookup[DATA] = data;
   msgLookup[RESP] = -1;
   msgLookup[STEP] = STEP_LOOKING;

   // sending the request
   MPI_Send(msgLookup, 5, MPI_INT, p, LOOKUP_MSG, MPI_COMM_WORLD);
   // waiting for response
   MPI_Recv(msgLookup, 5, MPI_INT, MPI_ANY_SOURCE, LOOKUP_MSG, MPI_COMM_WORLD, &status);
   printf("%d is responsible of data %d\n", msgLookup[RESP], data);

   // test response
   int test = idchord[0];
   for (i = 0; i < NB_SITE; i++) {
       if (idchord[i] > data) {
           test = idchord[i];
           break;
       }
   }
   if (test == msgLookup[RESP]) {
       printf("CORRECT\n" );
   }
   else {
       printf("ERROR : should be %d\n", test );
   }

   // stop the system
   for (i = 1; i <= NB_SITE; i++) {
       msgLookup[STEP] = STEP_TERM;
       MPI_Send(msgLookup, 5, MPI_INT, i, LOOKUP_MSG, MPI_COMM_WORLD);
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
    if (rank == 0) {
        simulateur(status);
    } else {
        // receive my chord id
        MPI_Recv(msgID, 1, MPI_INT, MPI_ANY_SOURCE, ID_MSG, MPI_COMM_WORLD, &status);
        // receive my finger table
        MPI_Recv(myfingerRank, M, MPI_INT, MPI_ANY_SOURCE, FINGERRANK_MSG, MPI_COMM_WORLD, &status);
        // receive rank corresponding to each finger
        MPI_Recv(myfinger, M, MPI_INT, MPI_ANY_SOURCE, FINGER_MSG, MPI_COMM_WORLD, &status);
        id_chord = msgID[0];
        succ = myfinger[0];

        if (showFingerTable) {
            printf("P%d: my chord id is %d\n", rank, id_chord );
            printf("P%d: my succ id is %d\n", rank, succ );
            for (i = 0; i < M; i++) {
                printf("P%d: finger %d is %d (rank : %d)\n", rank, i, myfinger[i], myfingerRank[i]);
            }
            printf("\n" );
        }

        while (1) {
            // wait for message from simulateur or site
            MPI_Recv(msgLookup, 5, MPI_INT, MPI_ANY_SOURCE, LOOKUP_MSG, MPI_COMM_WORLD, &status);

            // simulateur found the responsible of its data
            if (msgLookup[STEP] == STEP_TERM) {
                break;
            }
            // simulateur looks for a data
            else if (msgLookup[STEP] == STEP_LOOKING) {
                printf("P%d: reception <%d, %d, %d, %d, %d>\n", rank,
                    msgLookup[EMETTEUR],
                    msgLookup[INITIATEUR],
                    msgLookup[DATA],
                    msgLookup[RESP],
                    msgLookup[STEP]
                );
                
                int found = 0;
                int nextFinger = -1;
                int nextRank = -1;
                // looking for the the greater key j of the finger table as msgLookUp[DATA] in [j, id_chord]
                for (i = M-1; i >= 0; i--) {
                    if (lowerThan(myfinger[i] , msgLookup[DATA]) && app(msgLookup[DATA], myfinger[i], id_chord)) {
                        msgLookup[EMETTEUR] = rank;
                        found = 1;
                        nextFinger = myfinger[i];
                        nextRank = myfingerRank[i];
                        //break;
                    }
                }
                // if we found it, recursive call request to the finger
                if (found) {
                    printf("P%d: ask to %d (P%d) data %d\n", rank, nextFinger, nextRank, msgLookup[DATA]);
                    MPI_Send(msgLookup, 5, MPI_INT, nextRank, LOOKUP_MSG, MPI_COMM_WORLD);

                }
                // else, the responsible of the data is my successor
                else {
                    printf("P%d: my successor %d (P%d) is responsible of data %d\n", rank, myfinger[0], myfingerRank[0], msgLookup[DATA] );
                    msgLookup[STEP] = STEP_FOUND;
                    msgLookup[RESP] = succ;
                    // sending the responsible to simulateur
                    MPI_Send(msgLookup, 5, MPI_INT, 0, LOOKUP_MSG, MPI_COMM_WORLD);
                }
            }
        }
   }

   MPI_Finalize();
   return 0;
}

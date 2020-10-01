#include <stdio.h>
#include <string.h>
#include <mpi.h>

//************   LES TAGS
#define WANNA_CHOPSTICK 0		// Demande de baguette
#define CHOPSTICK_YOURS 1		// Cession de baguette
#define DONE_EATING     2		// Annonce de terminaison

//************   LES ETATS D'UN NOEUD
#define THINKING 0   // N'a pas consomme tous ses repas & n'attend pas de baguette
#define HUNGRY   1   // En attente de baguette
#define DONE     2   // A consomme tous ses repas

//************   LES REPAS
#define NB_MEALS 3	// nb total de repas a consommer par noeud

//************   LES VARIABLES MPI
int NB;               // nb total de processus
int rank;             // mon identifiant
int left, right;      // les identifiants de mes voisins gauche et droit

//************   LA TEMPORISATION
int local_clock = 0;                    // horloge locale
int clock_val;                          // valeur d'horloge recue
int meal_times[NB_MEALS];        // dates de mes repas

//************   LES ETATS LOCAUX
int local_state = THINKING;		// moi
int left_state  = THINKING;		// mon voisin de gauche
int right_state = THINKING;		// mon voisin de droite

//************   LES BAGUETTES
int left_chopstick = 0;		// je n'ai pas celle de gauche
int right_chopstick = 0;	// je n'ai pas celle de droite

//************   LES REPAS
int meals_eaten = 0;		// nb de repas consommes localement


//************   LES FONCTIONS   ***************************
int max(int a, int b) {
   return (a>b?a:b);
}

void receive_message(MPI_Status *status) {
	// A completer
  MPI_Recv(&clock_val, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, status);
}

void send_message(int dest, int tag) {
	 // A completer
   MPI_Send(&local_clock, 1, MPI_INT, dest, tag, MPI_COMM_WORLD);
}

/* renvoie 0 si le noeud local et ses 2 voisins sont termines */
int check_termination() {
   // A completer
   if (local_state == DONE && left_state == DONE && right_state == DONE) return 0;
   return 1;
}

//************   LE PROGRAMME   ***************************
int main(int argc, char* argv[]) {

   MPI_Status status;

   MPI_Init(&argc, &argv);
   MPI_Comm_size(MPI_COMM_WORLD, &NB);
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);

   left = (rank + NB - 1) % NB;
   right = (rank + 1) % NB;

   int *source;
   int *source_state;
   int *source_chopsticks;

   int displayAsk = 1;
   int displayDoneEating = 1;
   int displayChopYours = 1;
   int displayWannaChop = 1;
   int displayIsEating = 1;

   while(check_termination()) {
     local_clock++;
      /* Tant qu'on n'a pas fini tous ses repas, redemander les 2 baguettes
         a chaque fin de repas */
      if ((meals_eaten < NB_MEALS) && (local_state == THINKING)) {
         //demande de baguette aux 2 voisins
         if (displayAsk) printf("P%d> Asking for chopsticks\t at %d\n", rank, local_clock);
	       //A Completer
         send_message(left, WANNA_CHOPSTICK);
         send_message(right, WANNA_CHOPSTICK);
         local_state = HUNGRY;
      }

      receive_message(&status);

      local_clock = max(clock_val + 1, local_clock) ;

      if (status.MPI_SOURCE == rank + 1) {
        source = &right;
        source_state = &right_state;
        source_chopsticks = &right_chopstick;
      }
      else {
        source = &left;
        source_state = &left_state;
        source_chopsticks = &left_chopstick;
      }


      if (status.MPI_TAG == DONE_EATING) {
         //Enregistrer qu'un voisin a fini de manger
        *source_chopsticks = 1;
        *source_state = DONE;
        if (displayDoneEating) printf("P%d> %d says DONE_EATING\t\t at %d\n", rank, *source, local_clock);
      } else {
        if (status.MPI_TAG == CHOPSTICK_YOURS) {
           //Recuperation d'une baguette
           if (displayChopYours) printf("P%d> %d says CHOPSTICK_YOURS\t at %d\n", rank, *source, local_clock);
           if (displayChopYours) printf("P%d> %d : %d\n", rank, left_chopstick, right_chopstick);
           *source_chopsticks = 1;
        } else {
		        /* c'est une demande */
            if (displayWannaChop) printf("P%d> %d says WANNA_CHOPSTICK\t at %d\n", rank, *source, local_clock);
            if (local_state == DONE) {
              send_message(status.MPI_SOURCE, CHOPSTICK_YOURS);
              *source_chopsticks = 0;
            } else {
              if (rank > status.MPI_SOURCE && *source_state != DONE) {
                send_message(status.MPI_SOURCE, CHOPSTICK_YOURS);
                *source_chopsticks = 0;
                //local_state = HUNGRY;
              }
            }

         }
      }


      if (left_chopstick == 1 && right_chopstick == 1) {
        if (displayIsEating) printf("P%d> is EATING\t\t\t at %d\n", rank, local_clock);
        left_chopstick = 0;
        right_chopstick = 0;
        meals_eaten++;
        local_state = THINKING;

        if (meals_eaten == NB_MEALS - 1) {
          local_state = DONE;
          printf("P%d> %d et %d\n",rank,left,right );
          send_message(left, DONE_EATING);
          send_message(right, DONE_EATING);
        }

      }

   }

   MPI_Finalize();
   return 0;
}

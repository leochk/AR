#define N 6           /* Number of processes */
#define NB_V_MAX 3    /* maximum degree of a node */

mtype = {calcul};

chan channel_IN[N] = [N] of {mtype, byte, byte};
   /*  All messages to process i are received on channel_IN[i]
       a message contains information <type, sender, value>   */

/**************************************************************/

inline Simulateur() {
   if
      :: (id == 0) ->
	     minloc = 3; nb_voisins = 2;
	     voisins[0] = 1; voisins[1] = 2;
      :: (id == 1) ->
	     minloc = 11; nb_voisins = 3;
	     voisins[0] = 0; voisins[1] = 3; voisins[2] = 4;
      :: (id == 2) ->
	     minloc = 8; nb_voisins = 2;
	     voisins[0] = 0; voisins[1] = 5;
      :: (id == 3) ->
	     minloc = 14; nb_voisins = 1;
	     voisins[0] = 1;
      :: (id == 4) ->
	     minloc = 5; nb_voisins = 1;
	     voisins[0] = 1;
      :: (id == 5) ->
	     minloc = 17; nb_voisins = 1;
	     voisins[0] = 2;
   fi;
   mincal = minloc
}

/**************************************************************/
inline Test_Emission (q) {
   i = 1;
   if
      :: (nb_recus >= nb_voisins - 1) ->
         do
            :: (i <  NB_V_MAX && voisins[i] != -1)  ->
               i++;
            :: (i == NB_V_MAX) -> break;
         od
      :: (nb_voisins - 1 > nb_recus) -> q = -1; break;
   fi
}

/**************************************************************/

proctype node( byte id ) {

    byte nb_voisins;
    byte voisins[NB_V_MAX];

    chan canal_IN = channel_IN[id];
    xr canal_IN;            /* only id reads on this channel */

    mtype type = calcul;
    byte i, nb_recus;
    int sender, receiver, minloc, mincal;

    /* tableau initialise a 0 ; recu[i] = 1 si on a recu de i */
    byte recu[N];
    bool emission, wait = 0, decision, deja_emis = 0;
    Simulateur();

    /* Each process executes a finite loop */
    do
        /* test emission et traitement correspondant
                - on ne doit emettre qu'une fois
                - si on ne peut pas emettre, on attend (wait = 1) que la
                    condition ait pu etre modifiee                          */

      :: (wait == 0 && deja_emis == 0) ->
         Test_Emission(receiver);
         if
            :: (receiver != -1)->
                i = 0;
                do
                    :: i < NB_V_MAX && voisins[i] < N ->
                        channel_IN[voisins[i]] ! calcul(id,minloc);
                    :: i == NB_V_MAX -> break;
                od
               deja_emis = 1; wait = 1;
            :: (sender == -1) -> wait = 1;
         fi


         /* test reception et traitement correspondant */
      :: (wait == 1) ->
            canal_IN ? type, sender, mincal;
            nb_recus++;
            recu[sender] = 1;
            if
               :: (mincal < minloc) -> minloc = mincal;
            fi

      /* test terminaison */
      :: (wait == 0  && deja_emis == 1 && nb_recus == nb_voisins)->
         break;

   od;
   printf("%d : le minimum est %d\n", _pid, mincal);
}

/**************************************************************/
/* Creates a network of 6 nodes
   The structure of the network is given by array voisins */

init{
   byte proc;

   atomic {
      proc=0;
      do
         :: proc < N ->
            run node(proc);
            proc++
         :: proc == N -> break
      od
   }
}

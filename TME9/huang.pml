#define N 3

mtype = {actif, inactif};
mtype = {ack, agr, mes, term};

chan canaux [N]= [N] of {mtype, int, byte};
   /* message = <type, val_horloge, emetteur> */

bool termine = 0;
   /* positionne a vrai quand la terminaison est detectee */


/*************************************************/

inline envoyer(messtype, horloge, destinataire, emetteur) {
    unack++;
    canaux[destinataire] ! messtype, horloge, emetteur;
}

inline broadcast(messtype, horloge, emetteur) {
        byte k;
        k = 1;
        do
            :: k < N -> atomic {
                canaux[emetteur]!messtype,horloge,emetteur;
            }
            k++
            :: k == N -> break
        od;
}

inline desactivation() {
    etat = inactif;
    if
        :: unack == 0 ->
            h++;
            last = i;
            hrec = h;
            nbagr = 0;
            broadcast(term, h, i);
        :: else -> skip
    fi;
}

inline traiterMessage() {
    mtype messtype;
    int horloge, emetteur;
    canaux[id] ? messtype, horloge, emetteur;
    if
        /* ack message =======================================================*/
        :: messtype == ack ->
            unack--;
            if
                :: h < horloge -> h = horloge
                :: else -> skip
            fi;

            if
                :: unack == 0 && etat == inactif ->
                    h++;
                    last = emetteur;
                    hrec = h;
                    nbagr = 0;
                    broadcast(term, h, emetteur);
                :: else -> skip
            fi;
        /* term message ======================================================*/
        :: messtype == term ->
            if
                :: etat == actif || unack > 0 ->
                    if
                        :: h < horloge -> h = horloge
                        :: else -> skip
                    fi;
                :: else ->
                    if
                        :: h > hrec ->
                            h = horloge;
                            hrec = horloge;
                            last = emetteur;
                            envoyer(agr, h, id, emetteur)
                        :: else -> skip
                    fi;
            fi;
        /* agr message =======================================================*/
        :: messtype == agr ->
            if
                :: h == hrec ->
                    nbagr++;
                    if
                        :: nbagr == N-1 ->
                            termine = 1
                        :: else -> skip
                    fi;
                :: else -> skip
            fi;
        /* mes message =======================================================*/
        :: else ->
            if
                :: etat == inactif -> etat = actif
                :: else -> skip
            fi;
            envoyer(ack, h, emetteur, id);
}

/*************************************************/

/* VOUS POUVEZ AJOUTER D'AUTRES INLINE */

/*************************************************/

proctype un_site (byte id) {

   byte i;
   byte last; //dernier processus qui a lancé le terminaison
   byte sdr, dest = (id+1)%N;

   int h, hrec;

   short unack = 0; /* le nombre de messages non acquittes */
   short nbagr; /* nombre d'accords sur la terminaison recus */

   mtype mes_tp, etat = actif;

   h = 0;
   do
      :: ( empty(canaux[id]) && (etat == actif) ) ->
            if
               :: (1) ->   /* on peut arreter les actions locales */
                     desactivation()

               :: (1) ->   /* on peut envoyer un message. A COMPLETER */
            fi;

      :: nempty(canaux[id]) ->   /* on recoit un message. A COMPLETER */
            traiterMessage()
   od
}

/*************************************************/

init {
   byte i;
   atomic {
      i=0;
      do
         :: i <N ->
               run un_site(i);
               i++
         :: i == N -> break
      od
   }
}

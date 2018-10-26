
/******************************************************************************
 *    Massages the constraintlist "net" to create its lattice                  *
 *                                                                             *
 ******************************************************************************/

#include "inc/nova.h"

precode(net) CONSTRAINT **net;

{
  if (POW2CONSTR) {

    /* generates the constraints lattice using only power-of-two constraints */
    pow2pruning(net);

    /*shownet(net,net_name,"after pow2pruning (1st pass)");*/

    red_lattice(net);

    /*shownet(net,net_name,"after red_lattice");*/

    pow2pruning(net);

    /*shownet(net,net_name,"after pow2pruning (2nd pass)");*/

  } else {

    /* generates the constraints lattice using all available constraints */

    lattice(net);

    /*shownet(net,net_name,"after lattice");*/
  }
}

/*******************************************************************************
 * Manipulates the constraints whose cardinality is not a power of two : *
 * either keeps them in the lattice freezing degrees of freedom on the *
 * hypercube ( up to the closest power of two ) or deletes them * ( according to
 *whether free positions on the hypercube are still available   * or not ) *
 *******************************************************************************/

pow2pruning(net) CONSTRAINT **net;

{
  CONSTRAINT *constrptr, *temptr;

  int virtuals;    /* virtual symblemes introduced by forcing the satisfaction
                       of constraints whose cardinality is not a power of 2     */
  int free_places; /* # of vertices of the hypercube not already assigned to
                          symblemes */

  /* empty net : do nothing */
  if ((*net) == (CONSTRAINT *)0)
    return;

  /* prune the constraints whose cardinality is either 1 or larger than 1/2
     of the # of symblemes -
     pruning is done by setting to PRUNED the field "attribute" of
     "constraint "   */
  for (constrptr = (*net); constrptr != (CONSTRAINT *)0;
       constrptr = constrptr->next) {
    if ((constrptr->card == 1) ||
        (constrptr->card > (

                               minpow2(strlen((

                                                  *net)
                                                  ->relation)) /
                               2))) {
      constrptr->attribute = PRUNED;
    }
  }

  /* prune the constraints whose cardinality is not a power of 2
     if the number of symblemes is a power of two                */

  /* virtuals is set to the proper value */
  virtuals = 0;
  for (constrptr = (*net); constrptr != (CONSTRAINT *)0;
       constrptr = constrptr->next) {
    if (constrptr->odd_type == ADMITTED) {
      virtuals = virtuals + minpow2(constrptr->card) - constrptr->card;
    }
  }

  /* free_places is set to the proper value */
  free_places =
      minpow2(strlen((*net)->relation)) - strlen((*net)->relation) - virtuals;

  if (free_places == 0) {

    /* prune the constraints whose cardinality is not a power of 2
       if the number of symblemes is a power of two                */

    for (constrptr = (*net); constrptr != (CONSTRAINT *)0;
         constrptr = constrptr->next) {
      if (constrptr->attribute != PRUNED && constrptr->odd_type != ADMITTED &&
          minpow2(constrptr->card) != constrptr->card) {
        constrptr->attribute = PRUNED;
      }
    }

  } else {

    /* consider as constraints to be satisfied
       ( i.e. set to ADMITTED the field "odd_type" of "constraint" )
       the unpruned constraints whose cardinality is not a
       power-of-two as long as there are free places on the
       hypercube -
       heuristics : keep with higher priority the constraints of
       larger cardinality */

    /* "temptr" points to the next constraint ( whose cardinality
       is not a power of two ) not yet taken into consideration  */
    for (constrptr = (*net); constrptr != (CONSTRAINT *)0;
         constrptr = constrptr->next) {
      if (constrptr->attribute != PRUNED && constrptr->odd_type != ADMITTED &&
          (minpow2(constrptr->card) - constrptr->card != 0)) {
        temptr = constrptr;
        break;
      }
    }

    /* loops as long as there are free places and constraints
       whose cardinality is not a power of two  -
       according to the chosen heuristics ,
       "temptr" points to the next constraint of larger cardinality
       ( whose cardinality is not a power of two ) not yet taken
       into consideration                                         */
    while (free_places > 0 && constrptr != (CONSTRAINT *)0) {
      for (constrptr = (*net); constrptr != (CONSTRAINT *)0;
           constrptr = constrptr->next) {
        if (constrptr->attribute != PRUNED &&
            (constrptr->odd_type != ADMITTED &&
             constrptr->odd_type != EXAMINED) &&
            minpow2(constrptr->card) - constrptr->card != 0) {
          if (constrptr->card > temptr->card) {
            temptr = constrptr;
          }
        }
      }

      /* sets temptr->oddtype to ADMITTED if temptr needs less or
      equal free_places than available ( and freepos gets the
      updated value ) -  otherwise sets temptr->odd_type to
      EXAMINED to avoid reconsidering it as the while goes on */
      if ((minpow2(temptr->card) - temptr->card) <= free_places) {
        free_places = free_places - (minpow2(temptr->card) - temptr->card);
        temptr->odd_type = ADMITTED;
      } else {
        temptr->odd_type = EXAMINED;
      }

      /* finds the next constraint (whose cardinality is not
         a power of two) not yet taken into consideration */
      for (constrptr = (*net); constrptr != (CONSTRAINT *)0;
           constrptr = constrptr->next) {
        if (constrptr->attribute != PRUNED &&
            (constrptr->odd_type != ADMITTED &&
             constrptr->odd_type != EXAMINED) &&
            minpow2(constrptr->card) - constrptr->card != 0) {
          temptr = constrptr;
          break;
        }
      }
    }
  }

  /* prune the remaining constraints (of cardinality not a
     power of two) not taken into consideration because all
     the free places on the hypercube were already gone  */
  for (constrptr = (*net); constrptr != (CONSTRAINT *)0;
       constrptr = constrptr->next) {
    if (constrptr->attribute != PRUNED &&
        (minpow2(constrptr->card) - constrptr->card != 0) &&
        constrptr->odd_type != ADMITTED) {
      constrptr->attribute = PRUNED;
    }
  }
}

/*******************************************************************************
 *    Creates the lattice of the constraintlist "list" , appending to the bare *
 *    constraintlist outputted by the routine "analysis" all the intersections *
 *    of constraints without repetitions ( if an intersection coincides with a *
 *    constraint already present , the action taken is to increase the depth of
 ** the constraint )                                                          *
 *    "lattice" is called when the option POW2CONSTR is not active , i.e. when *
 *    the lattice is not pruned by "pow2pruning" *
 *******************************************************************************/

lattice(net) CONSTRAINT **net;

{
  CONSTRAINT *constrptr1, *constrptr2, *temptr, *new, *newconstraint();
  char *newrelation;
  int i, card, level;
  BOOLEAN cycle_in_progress;
  int countnewconstr;

  countnewconstr = 0;

  /* empty net : do nothing */
  if ((*net) == (CONSTRAINT *)0)
    return;

  if ((newrelation = (char *)calloc((unsigned)(strlen((*net)->relation) + 1),
                                    sizeof(char))) == (char *)0) {
    fprintf(stderr, "Insufficient memory for newrelation in lattice\n");
    exit(-1);
  }
  newrelation[strlen((*net)->relation)] = '\0';

  level = 0;

  do {

    cycle_in_progress = FALSE;

    /* "constrptr1" points to the first operand of the intersection
       operation */
    for (constrptr1 = (*net); constrptr1 != (CONSTRAINT *)0;
         constrptr1 = constrptr1->next) {
      if (constrptr1->attribute == PRUNED || constrptr1->level != level) {
        continue;
      }

      /* "constrptr2" points to the second operand of the intersection
         operation */
      for (constrptr2 = constrptr1->next; constrptr2 != (CONSTRAINT *)0;
           constrptr2 = constrptr2->next) {
        if (constrptr2->attribute == PRUNED || constrptr2->level != level) {
          continue;
        }

        /* compute a new constraint as the intersection of two other ones */
        card = 0;
        for (i = 0; constrptr1->relation[i] != '\0'; i++) {
          if (constrptr1->relation[i] == ONE &&
              constrptr2->relation[i] == ONE) {
            card++;
            newrelation[i] = ONE;
          } else {
            newrelation[i] = ZERO;
          }
        }

        /* discard intersections empty or with cardinality either 1
           or larger than 1/2 of the # of symblemes                */
        if (card == 0 || card == 1 ||
            card > (

                       minpow2(strlen((

                                          *net)
                                          ->relation)) /
                       2)) {
          continue;
        }

        cycle_in_progress = TRUE;

        /* take care of intersections coinciding with already present elements
         */
        for (temptr = (*net); temptr != (CONSTRAINT *)0;
             temptr = temptr->next) {
          if (strcmp(newrelation, temptr->relation) == 0) {
            break;
          }
        }

        /* constraint not already found */
        if (temptr == (CONSTRAINT *)0) {
          new = newconstraint(newrelation, card, level + 1);
          new->next = (*net);
          (*net) = new;
          countnewconstr++;
        } else {
          temptr->weight++;
          temptr->newlevel = level + 1;
        }

        if (countnewconstr > 1000) {
          fprintf(stderr, "\n                  WARNING\n");
          fprintf(stderr,
                  "** After that lattice added the 1001-th new constraint ,\n");
          fprintf(stderr,
                  "Nova stopped executing lattice and went ahead with the\n");
          fprintf(stderr,
                  "constraints that lattice already got ****************\n\n");
          return;
        }
      }
    }

    for (constrptr1 = (*net); constrptr1 != (CONSTRAINT *)0;
         constrptr1 = constrptr1->next) {
      if (constrptr1->newlevel > constrptr1->level) {
        constrptr1->level = constrptr1->newlevel;
      }
    }

    level++;

  } while (cycle_in_progress);
}

/*******************************************************************************
 *    Creates the lattice of the constraintlist "list" , appending to the bare *
 *    constraintlist outputted by the routine "analysis" all the intersections *
 *    of constraints without repetitions ( if an intersection coincides with a *
 *    constraint already present , the action taken is to increase the depth of
 ** the constraint ) -                                                        *
 *    "red_lattice" is called when the option POW2CONSTR is active , i.e. when *
 *    the lattice is pruned by "pow2pruning" *
 *******************************************************************************/

red_lattice(net) CONSTRAINT **net;

{
  CONSTRAINT *constrptr1, *constrptr2, *temptr, *new, *newconstraint();
  char *newrelation;
  int i, card, samecard, level;
  BOOLEAN cycle_in_progress;
  int countnewconstr;

  countnewconstr = 0;

  /* empty net : do nothing */
  if ((*net) == (CONSTRAINT *)0)
    return;

  if ((newrelation = (char *)calloc((unsigned)(strlen((*net)->relation) + 1),
                                    sizeof(char))) == (char *)0) {
    fprintf(stderr, "Insufficient memory for newrelation in lattice\n");
    exit(-1);
  }
  newrelation[strlen((*net)->relation)] = '\0';

  level = 0;

  do {

    cycle_in_progress = FALSE;

    /* "constrptr1" points to the first operand of the intersection
       operation */
    for (constrptr1 = (*net); constrptr1 != (CONSTRAINT *)0;
         constrptr1 = constrptr1->next) {
      if (constrptr1->attribute == PRUNED || constrptr1->level != level) {
        continue;
      }

      /* "constrptr2" points to the second operand of the intersection
         operation */
      for (constrptr2 = constrptr1->next; constrptr2 != (CONSTRAINT *)0;
           constrptr2 = constrptr2->next) {
        if (constrptr2->attribute == PRUNED || constrptr2->level != level) {
          continue;
        }

        /* compute a new constraint as the intersection of two other ones */
        card = 0;
        if (constrptr1->card == constrptr2->card) {
          samecard = 1;
        } else {
          samecard = 0;
        }
        for (i = 0; constrptr1->relation[i] != '\0'; i++) {
          if (constrptr1->relation[i] == ONE &&
              constrptr2->relation[i] == ONE) {
            card++;
            newrelation[i] = ONE;
          } else {
            newrelation[i] = ZERO;
          }
        }

        /* discard empty intersections */
        if (card == 0) {
          continue;
        }

        /* Disregard intersections if
        a) they come from rows of same cardinality
                  &&
        b) their cardinality is not 1\2 of the cardinality of the
           parents .
        Heuristics : we have to prune one of the two new constraints
        (because they are incompatible) , in case one of them is found
        via the "output_forcing" routine and the other is found via the
        "mini" routine we choose to get rid of the one not originated
        via the "output_forcing" routine   */

        if (constrptr1->card == constrptr2->card &&
            card != (constrptr1->card / 2)) {
          if (constrptr1->source_type != INPUT &&
              constrptr2->source_type == INPUT) {
            constrptr2->attribute = PRUNED;
            continue;
          }

          if (constrptr1->source_type == INPUT &&
              constrptr2->source_type != INPUT) {
            constrptr1->attribute = PRUNED;
            break;
          }
        }

        cycle_in_progress = TRUE;

        /* take care of intersections coinciding with already present elements
         */
        for (temptr = (*net); temptr != (CONSTRAINT *)0;
             temptr = temptr->next) {
          if (strcmp(newrelation, temptr->relation) == 0) {
            break;
          }
        }

        /* constraint not already found */
        if (temptr == (CONSTRAINT *)0) {
          new = newconstraint(newrelation, card, level + samecard + 1);
          new->next = (*net);
          (*net) = new;
          countnewconstr++;
        } else {
          temptr->weight++;
          temptr->newlevel = level + samecard + 1;
        }

        if (countnewconstr > 1000) {
          fprintf(stderr, "\n                  WARNING\n");
          fprintf(
              stderr,
              "** After that red_lattice added the 1001-th new constraint ,\n");
          fprintf(
              stderr,
              "Nova stopped executing red_lattice and went ahead with the\n");
          fprintf(
              stderr,
              "constraints that red_lattice already got ****************\n\n");
          return;
        }
      }
    }

    for (constrptr1 = (*net); constrptr1 != (CONSTRAINT *)0;
         constrptr1 = constrptr1->next) {
      if (constrptr1->newlevel > constrptr1->level) {
        constrptr1->level = constrptr1->newlevel;
      }
    }

    level++;

  } while (cycle_in_progress);
}

/*****************************************************************************
 *   Links the constraints to all the data structures involved in the coding  *
 *   algorithm  (e.g. "symblemes", "levelarray", "cardarray")                 *
 *****************************************************************************/

link_constr(net, symblemes, num) CONSTRAINT *net;
SYMBLEME *symblemes;
int num;

{

  CONSTRAINT *temptr;
  CONSTRLINK *newlink;
  int i;

  for (i = 0; i < num; i++) {

    levelarray[i] = (CONSTRAINT *)0;
    cardarray[i] = (CONSTRAINT *)0;
    symblemes[i].constraints = (CONSTRLINK *)0;
    symblemes[i].code_status = NOTCODED;
  }

  for (temptr = net; temptr != (CONSTRAINT *)0; temptr = temptr->next) {

    if (temptr->attribute == PRUNED) {
      continue;
    }

    temptr->attribute = NOTUSED;

    for (i = 0; i < num; i++) {
      if (temptr->relation[i] == ONE) {

        if ((newlink = (CONSTRLINK *)calloc((unsigned)1, sizeof(CONSTRLINK))) ==
            (CONSTRLINK *)0) {
          fprintf(stderr, "Insufficient memory for newlink in link_constr");
          exit(-1);
        }

        newlink->constraint = temptr;
        newlink->next = symblemes[i].constraints;
        symblemes[i].constraints = newlink;
      }
    }

    temptr->levelnext = levelarray[temptr->level];
    levelarray[temptr->level] = temptr;
    temptr->cardnext = cardarray[temptr->card];
    cardarray[temptr->card] = temptr;
  }
}

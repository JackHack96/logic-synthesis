/**CFile****************************************************************

  FileName    [satSort.c]

  PackageName [A C version of SAT solver MINISAT, originally developed 
  in C++ by Niklas Een and Niklas Sorensson, Chalmers University of 
  Technology, Sweden: http://www.cs.chalmers.se/~een/Satzoo.]

  Synopsis    [Sorting clauses.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: satSort.c,v 1.1 2005/07/08 01:01:30 alanmi Exp $]

***********************************************************************/

#include "satInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Sat_SolverSortCompare( Sat_Clause_t ** ppC1, Sat_Clause_t ** ppC2 );

// Returns a random float 0 <= x < 1. Seed must never be 0.
static double drand(double seed) {
    int q;
    seed *= 1389796;
    q = (int)(seed / 2147483647);
    seed -= (double)q * 2147483647;
    return seed / 2147483647; }

// Returns a random integer 0 <= x < size. Seed must never be 0.
static int irand(double seed, int size) {
    return (int)(drand(seed) * size); }

static void Sat_SolverSort( Sat_Clause_t ** array, int size, double seed );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Sat_SolverSort the learned clauses in the increasing order of activity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_SolverSortDB( Sat_Solver_t * p )
{
    Sat_ClauseVec_t * pVecClauses;
    Sat_Clause_t ** pLearned;
    int nLearned;
    // read the parameters
    pVecClauses = Sat_SolverReadLearned( p );
    nLearned    = Sat_ClauseVecReadSize( pVecClauses );
    pLearned    = Sat_ClauseVecReadArray( pVecClauses );
    // Sat_SolverSort the array
//    qSat_SolverSort( (void *)pLearned, nLearned, sizeof(Sat_Clause_t *), 
//            (int (*)(const void *, const void *)) Sat_SolverSortCompare );
//    printf( "Sat_SolverSorting.\n" );
    Sat_SolverSort( pLearned, nLearned, 91648253 );
/*
    if ( nLearned > 2 )
    {
    printf( "Clause 1: %0.20f\n", Sat_ClauseReadActivity(pLearned[0]) );
    printf( "Clause 2: %0.20f\n", Sat_ClauseReadActivity(pLearned[1]) );
    printf( "Clause 3: %0.20f\n", Sat_ClauseReadActivity(pLearned[2]) );
    }
*/
}

/**Function*************************************************************

  Synopsis    [Comparison procedure for two clauses.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sat_SolverSortCompare( Sat_Clause_t ** ppC1, Sat_Clause_t ** ppC2 )
{
    float Value1 = Sat_ClauseReadActivity( *ppC1 );
    float Value2 = Sat_ClauseReadActivity( *ppC2 );
    if ( Value1 < Value2 )
        return -1;
    if ( Value1 > Value2 )
        return 1;
    return 0;
}


/**Function*************************************************************

  Synopsis    [Selection sort for small array size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_SolverSortSelection( Sat_Clause_t ** array, int size )
{
    Sat_Clause_t * tmp;
    int i, j, best_i;
    for ( i = 0; i < size-1; i++ )
    {
        best_i = i;
        for (j = i+1; j < size; j++)
        {
            if ( Sat_ClauseReadActivity(array[j]) < Sat_ClauseReadActivity(array[best_i]) )
                best_i = j;
        }
        tmp = array[i]; array[i] = array[best_i]; array[best_i] = tmp;
    }
}

/**Function*************************************************************

  Synopsis    [The original MiniSat sorting procedure.]

  Description [This procedure is used to preserve trace-equivalence
  with the orignal C++ implemenation of the solver.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_SolverSort( Sat_Clause_t ** array, int size, double seed )
{
    if (size <= 15)
        Sat_SolverSortSelection( array, size );
    else
    {
        Sat_Clause_t *   pivot = array[irand(seed, size)];
        Sat_Clause_t *   tmp;
        int              i = -1;
        int              j = size;

        for(;;)
        {
            do i++; while( Sat_ClauseReadActivity(array[i]) < Sat_ClauseReadActivity(pivot) );
            do j--; while( Sat_ClauseReadActivity(pivot) < Sat_ClauseReadActivity(array[j]) );

            if ( i >= j ) break;

            tmp = array[i]; array[i] = array[j]; array[j] = tmp;
        }
        Sat_SolverSort(array    , i     , seed);
        Sat_SolverSort(&array[i], size-i, seed);
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



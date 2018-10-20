/**CFile****************************************************************

  FileName    [satQueue.c]

  PackageName [A C version of SAT solver MINISAT, originally developed 
  in C++ by Niklas Een and Niklas Sorensson, Chalmers University of 
  Technology, Sweden: http://www.cs.chalmers.se/~een/Satzoo.]

  Synopsis    [The manager of the assignment propagation queue.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: satQueue.c,v 1.1 2005/07/08 01:01:28 alanmi Exp $]

***********************************************************************/

#include "satInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

struct Sat_Queue_t_ 
{
    int         nVars;
    int *       pVars;
    int         iFirst;
    int         iLast;
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates the variable propagation queue.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sat_Queue_t * Sat_QueueAlloc( int nVars )
{
    Sat_Queue_t * p;
    p = ALLOC( Sat_Queue_t, 1 );
    memset( p, 0, sizeof(Sat_Queue_t) );
    p->nVars  = nVars;
    p->pVars  = ALLOC( int, nVars );
    return p;
}

/**Function*************************************************************

  Synopsis    [Deallocate the variable propagation queue.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_QueueFree( Sat_Queue_t * p )
{
    free( p->pVars );
    free( p );
}

/**Function*************************************************************

  Synopsis    [Reads the queue size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sat_QueueReadSize( Sat_Queue_t * p )
{
    return p->iLast - p->iFirst;
}

/**Function*************************************************************

  Synopsis    [Insert an entry into the queue.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_QueueInsert( Sat_Queue_t * p, int Lit )
{
    if ( p->iLast == p->nVars )
    {
        int i;
        assert( 0 );
        for ( i = 0; i < p->iLast; i++ )
            printf( "entry = %2d  lit = %2d  var = %2d \n", i, p->pVars[i], p->pVars[i]/2 );
    }
    assert( p->iLast < p->nVars );
    p->pVars[p->iLast++] = Lit;
}

/**Function*************************************************************

  Synopsis    [Extracts an entry from the queue.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sat_QueueExtract( Sat_Queue_t * p )
{
    if ( p->iFirst == p->iLast )
        return -1;
    return p->pVars[p->iFirst++];
}

/**Function*************************************************************

  Synopsis    [Resets the queue.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_QueueClear( Sat_Queue_t * p )
{
    p->iFirst = 0;
    p->iLast  = 0;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



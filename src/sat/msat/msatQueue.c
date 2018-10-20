/**CFile****************************************************************

  FileName    [msatQueue.c]

  PackageName [A C version of SAT solver MINISAT, originally developed 
  in C++ by Niklas Een and Niklas Sorensson, Chalmers University of 
  Technology, Sweden: http://www.cs.chalmers.se/~een/Satzoo.]

  Synopsis    [The manager of the assignment propagation queue.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: msatQueue.c,v 1.1 2005/07/08 01:01:37 alanmi Exp $]

***********************************************************************/

#include "msatInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

struct Msat_Queue_t_ 
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
Msat_Queue_t * Msat_QueueAlloc( int nVars )
{
    Msat_Queue_t * p;
    p = ALLOC( Msat_Queue_t, 1 );
    memset( p, 0, sizeof(Msat_Queue_t) );
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
void Msat_QueueFree( Msat_Queue_t * p )
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
int Msat_QueueReadSize( Msat_Queue_t * p )
{
    return p->iLast - p->iFirst;
}

/**Function*************************************************************

  Synopsis    [Insert an entry into the queue.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_QueueInsert( Msat_Queue_t * p, int Lit )
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
int Msat_QueueExtract( Msat_Queue_t * p )
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
void Msat_QueueClear( Msat_Queue_t * p )
{
    p->iFirst = 0;
    p->iLast  = 0;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



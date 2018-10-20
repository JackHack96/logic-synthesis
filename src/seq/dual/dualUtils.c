/**CFile****************************************************************

  FileName    [dualUtils.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Manipulation of internal data structures.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 18, 2003.]

  Revision    [$Id: dualUtils.c,v 1.2 2004/02/19 03:06:53 alanmi Exp $]

***********************************************************************/

#include "dualInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dual_Spec_t * Dual_SpecAlloc()
{
    Dual_Spec_t * p;
    p = ALLOC( Dual_Spec_t, 1 );
    memset( p, 0, sizeof(Dual_Spec_t) );
//    p->ppVarNames = ALLOC( char *, nVars );
//    p->pbVars = ALLOC( DdNode *, nVars );
//    p->nVars = nVars;
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dual_SpecFree( Dual_Spec_t * p )
{
    int i;
    for ( i = 0; i < p->nVars; i++ )
        FREE( p->ppVarNames[i] );
    Cudd_RecursiveDeref( p->dd, p->bSpec );
    if ( p->nStates > 0 )
    {
        for ( i = 0; i < p->nStates; i++ )
        {
            if ( p->pbMarksP[i] )
                Cudd_RecursiveDeref( p->dd, p->pbMarksP[i] );
            if ( p->pbMarks[i] )
                Cudd_RecursiveDeref( p->dd, p->pbMarks[i] );
            if ( p->pbTrans[i] )
                Cudd_RecursiveDeref( p->dd, p->pbTrans[i] );
        }
        FREE( p->pbMarksP );
        FREE( p->pbMarks );
        FREE( p->pbTrans );
    }
    if ( p->bCondInit )
        Cudd_RecursiveDeref( p->dd, p->bCondInit );
    FREE( p->pStatesInit );

    Extra_StopManager( p->dd );

    FREE( p->ppVarNames );
    FREE( p->pbVars );
    FREE( p->pName );
    FREE( p );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



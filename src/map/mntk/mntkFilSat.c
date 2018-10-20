/**CFile****************************************************************

  FileName    [mntkFilSat.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic mapped network.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: mntkFilSat.c,v 1.1 2005/02/28 05:34:30 alanmi Exp $]

***********************************************************************/

#include "mntkInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Checks if it is true that the nodes can resynthesis the given node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mntk_Node_t * Mntk_CheckSat( Mntk_Man_t * p, Mntk_Node_t * pPivot, Mntk_Node_t * ppCands[], int nCands )
{
    Mntk_Node_t * pNodeNew;
    int RetValue, i;

    if ( nCands == 1 )
        return NULL;
 
    assert( nCands <= MNTK_MAX_FANINS );
    assert( nCands >= 2 );

    RetValue = Mntk_ResynthesisImage( pPivot, ppCands, nCands );
    if ( RetValue == 0 )
    {
        p->nFails++;
        return NULL;
    }
    for ( i = 0; i < 4; i++ )
        if ( p->uRes0All[i] & p->uRes1All[i] )
        {
            p->nSatFailure++;
            return NULL;
        }
    p->nSatSuccess++;

    // create the cut
    pNodeNew = Mntk_NodeAlloc( p, ppCands, nCands, NULL );
    // set the on-set as the truth table
    pNodeNew->uTruthTemp[0] = p->uRes1All[0];
    pNodeNew->uTruthTemp[1] = p->uRes1All[1];

    pNodeNew->uTruthZero[0] = p->uRes0All[0];
    pNodeNew->uTruthZero[1] = p->uRes0All[1];
/*
    {
        int nMints;
        unsigned uMask;
        nMints = (1 << nCands);
        uMask = ~((~0)<<nMints);
        if ( nMints <= 32 )
        {
            if ( (p->uRes1All[0] & uMask) == (~p->uRes0All[0] & uMask) )
                printf( "1\n" );
            else
                printf( "0\n" );
        }
    }
*/
    return pNodeNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



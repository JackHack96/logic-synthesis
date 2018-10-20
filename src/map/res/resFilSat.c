/**CFile****************************************************************

  FileName    [resFilSat.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic resynthesis engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: resFilSat.c,v 1.1 2005/01/23 06:59:47 alanmi Exp $]

***********************************************************************/

#include "resInt.h"

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
Fpga_Cut_t * Res_CheckSat( Res_Man_t * p, Fpga_Node_t * pPivot, Fpga_Node_t * ppCands[], int nCands )
{
    Fpga_Cut_t * pCut;
    int RetValue, i;

    if ( nCands == 1 )
        return NULL;
 
    assert( nCands <= 7 );
    assert( nCands >= 2 );

    // compute the image using simulation information as a starter
    if ( p->fGlobalCones )
        RetValue = Res_ResynthesisImage( p, pPivot, ppCands, nCands );
    else
        RetValue = Res_ResynthesisImage2( p, pPivot, ppCands, nCands );

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
    pCut = Fpga_CutAlloc( p->pMan );
    pCut->nLeaves = nCands;
    for ( i = 0; i < nCands; i++ )
        pCut->ppLeaves[i] = ppCands[i];
    // set the on-set as the truth table
    pCut->uTruthTemp[0] = p->uRes1All[0];
    pCut->uTruthTemp[1] = p->uRes1All[1];
    pCut->uTruthTemp[2] = p->uRes1All[2];
    pCut->uTruthTemp[3] = p->uRes1All[3];

    pCut->uTruthZero[0] = p->uRes0All[0];
    pCut->uTruthZero[1] = p->uRes0All[1];
    pCut->uTruthZero[2] = p->uRes0All[2];
    pCut->uTruthZero[3] = p->uRes0All[3];
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
    return pCut;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



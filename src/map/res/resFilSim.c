/**CFile****************************************************************

  FileName    [resFilSim.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic resynthesis engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: resFilSim.c,v 1.1 2005/01/23 06:59:47 alanmi Exp $]

***********************************************************************/

#include "resInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Res_TestOneWord( unsigned * puSimRoot, unsigned ** ppuSimNodes, int iWord, int nNodes, int fPhase, unsigned * puTruth );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Checks whether the nodes can implement the root node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Res_TestNodes( Res_Man_t * p, Fpga_Node_t * pRoot, Fpga_Node_t * pNodes[], int nNodes )
{
    unsigned * puSimRoot, * ppuSimNodes[10];
    unsigned uRes0[4], uRes1[4];
    int i;

    // start the return values
    p->uRes0All[0] = p->uRes1All[0] = 0;
    p->uRes0All[1] = p->uRes1All[1] = 0;
    p->uRes0All[2] = p->uRes1All[2] = 0;
    p->uRes0All[3] = p->uRes1All[3] = 0;

    assert( nNodes <= 7 );
    puSimRoot = p->pSimms[pRoot->NumA];
    for ( i = 0; i < nNodes; i++ )
        ppuSimNodes[i] = p->pSimms[pNodes[i]->NumA];

    for ( i = 0; i < p->pMan->nSimRounds; i++ )
    {
        Res_TestOneWord( puSimRoot, ppuSimNodes, i, nNodes, 0, uRes0 );
        Res_TestOneWord( puSimRoot, ppuSimNodes, i, nNodes, 1, uRes1 );

        p->uRes0All[0] |= uRes0[0];
        p->uRes1All[0] |= uRes1[0];
        if ( p->uRes0All[0] & p->uRes1All[0] )
            return 0;

        p->uRes0All[1] |= uRes0[1];
        p->uRes1All[1] |= uRes1[1];
        if ( p->uRes0All[1] & p->uRes1All[1] )
            return 0;

        p->uRes0All[2] |= uRes0[2];
        p->uRes1All[2] |= uRes1[2];
        if ( p->uRes0All[2] & p->uRes1All[2] )
            return 0;

        p->uRes0All[3] |= uRes0[3];
        p->uRes1All[3] |= uRes1[3];
        if ( p->uRes0All[3] & p->uRes1All[3] )
            return 0;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns the signature of non-zero cofactors.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_TestOneWord( unsigned * puSimRoot, unsigned ** ppuSimNodes, int iWord, int nNodes, int fPhase, unsigned * puTruth )
{
    static unsigned uData[128]; // 2^7
    int Power, n, i; 

    assert( nNodes <= 7 );
/*
    for ( i = 0; i < 32; i++ )
    {
        for ( n = 0; n < nNodes; n++ )
            printf( "%d", ((ppuSimNodes[n][iWord] & (1 << i)) > 0) );
        printf( " %d\n", ((puSimRoot[iWord] & (1 << i)) > 0) );
    }
*/

    // start the return values
    puTruth[0] = 0;
    puTruth[1] = 0;
    puTruth[2] = 0;
    puTruth[3] = 0;

    // start the simulation info of the root node
    uData[0] = fPhase? puSimRoot[iWord] : ~puSimRoot[iWord];
    // consider sparse function
    if ( uData[0] == 0 )
        return;

    for ( Power = 1, n = 0; n < nNodes; n++, Power *= 2 )
        for ( i = Power-1; i >= 0; i-- )
        {
            uData[2*i+1] = uData[i] &  ppuSimNodes[nNodes-1-n][iWord];
            uData[2*i]   = uData[i] & ~ppuSimNodes[nNodes-1-n][iWord];
        }

    for ( i = 0; i < Power; i++ )
        if ( uData[i] )
            puTruth[i/32] |=  (1<<(i%32));
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



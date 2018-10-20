/**CFile****************************************************************

  FileName    [mntkFilSim.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic mapped network.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: mntkFilSim.c,v 1.1 2005/02/28 05:34:30 alanmi Exp $]

***********************************************************************/

#include "mntkInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Mntk_TestOneWord( unsigned * puSimRoot, unsigned ** ppuSimNodes, int iWord, int nNodes, int fPhase, unsigned * puTruth );

static void Mntk_SimulateNode( Mntk_Node_t * pNode );
static unsigned Mntk_SimulateWord( Mvc_Cover_t * pMvc, unsigned * pSimInfo, int nInputs );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Checks whether the nodes can implement the root node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mntk_TestNodes( Mntk_Man_t * p, Mntk_Node_t * pRoot, Mntk_Node_t * pNodes[], int nNodes )
{
    unsigned * puSimRoot, * ppuSimNodes[10];
    unsigned uRes0[4], uRes1[4];
    int i;

    // start the return values
    p->uRes0All[0] = p->uRes1All[0] = 0;
    p->uRes0All[1] = p->uRes1All[1] = 0;
//    p->uRes0All[2] = p->uRes1All[2] = 0;
//    p->uRes0All[3] = p->uRes1All[3] = 0;

    assert( nNodes <= 6 );
    puSimRoot = pRoot->pSims;
    for ( i = 0; i < nNodes; i++ )
        ppuSimNodes[i] = pNodes[i]->pSims;

    for ( i = 0; i < p->nSimRounds; i++ )
    {
        Mntk_TestOneWord( puSimRoot, ppuSimNodes, i, nNodes, 0, uRes0 );
        Mntk_TestOneWord( puSimRoot, ppuSimNodes, i, nNodes, 1, uRes1 );

        p->uRes0All[0] |= uRes0[0];
        p->uRes1All[0] |= uRes1[0];
        if ( p->uRes0All[0] & p->uRes1All[0] )
            return 0;

        p->uRes0All[1] |= uRes0[1];
        p->uRes1All[1] |= uRes1[1];
        if ( p->uRes0All[1] & p->uRes1All[1] )
            return 0;
/*
        p->uRes0All[2] |= uRes0[2];
        p->uRes1All[2] |= uRes1[2];
        if ( p->uRes0All[2] & p->uRes1All[2] )
            return 0;

        p->uRes0All[3] |= uRes0[3];
        p->uRes1All[3] |= uRes1[3];
        if ( p->uRes0All[3] & p->uRes1All[3] )
            return 0;
*/
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns the signature of non-zero cofactors.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mntk_TestOneWord( unsigned * puSimRoot, unsigned ** ppuSimNodes, int iWord, int nNodes, int fPhase, unsigned * puTruth )
{
    static unsigned uData[128]; // 2^7
    int Power, n, i; 

    assert( nNodes <= 6 );
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
//    puTruth[2] = 0;
//    puTruth[3] = 0;

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


/**Function*************************************************************

  Synopsis    [Simulate the window.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mntk_SimulateWindow( Mntk_Man_t * p )
{
    Mntk_Node_t * pNode;
    int i, v;
    // mark all the nodes
    for ( i = 0; i < p->vWinNodes->nSize; i++ )
        p->vWinNodes->pArray[i]->fMark0 = 1;
    // assign the random info to the PI nodes
    // vWinTotal comes in the increasing order of arrival times
    for ( i = 0; i < p->vWinTotal->nSize; i++ )
    {
        pNode = p->vWinTotal->pArray[i];
        assert( i == 0 || pNode->tArrival.Worst >= p->vWinTotal->pArray[i-1]->tArrival.Worst );
        if ( pNode->fMark0 == 0 )
        {
            for ( v = 0; v < p->nSimRounds; v++ )
                pNode->pSims[v] = MNTK_RANDOM_UNSIGNED;
        }
        else
        {
            Mntk_SimulateNode( pNode );
            pNode->fMark0 = 0;
        }
    }
    // mark all the nodes
//    for ( i = 0; i < p->vWinNodes->nSize; i++ )
//        p->vWinNodes->pArray[i]->fMark0 = 0;
}

/**Function*************************************************************

  Synopsis    [Simulate the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mntk_SimulateNode( Mntk_Node_t * pNode )
{
    Mntk_Node_t * pFanin;
    unsigned pSimInfo[10];
    unsigned * pSims[10];
    int fCompls[10];
    Mvc_Cover_t * pMvc;
    int i, k;

    // make sure that the inputs are unmarked
    for ( i = 0; i < (int)pNode->nLeaves; i++ )
    {
        pFanin     = Mntk_Regular(pNode->ppLeaves[i]);
        pSims[i]   = pFanin->pSims;
        fCompls[i] = Mntk_IsComplement(pNode->ppLeaves[i]);
        assert( pFanin->fMark0 == 0 );
    }
    // simulate each word
    pMvc = Mio_GateReadMvc(pNode->pGate);
    for ( i = 0; i < pNode->p->nSimRounds; i++ )
    {
        for ( k = 0; k < (int)pNode->nLeaves; k++ )
            pSimInfo[k] = fCompls[k]? ~pSims[k][i] : pSims[k][i];
        pNode->pSims[i] = Mntk_SimulateWord( pMvc, pSimInfo, pNode->nLeaves );
    }
}

/**Function*************************************************************

  Synopsis    [Simulate one word.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Mntk_SimulateWord( Mvc_Cover_t * pMvc, unsigned * pSimInfo, int nInputs )
{
    Mvc_Cube_t * pCube;
    unsigned uResult, uCube;
    int iVar, Value;

    uResult = 0;
    Mvc_CoverForEachCube( pMvc, pCube )
    {
        uCube = ~((unsigned)0);
        Mvc_CubeForEachVarValue( pMvc, pCube, iVar, Value )
        {
            if ( Value == 1 )
                uCube &= ~pSimInfo[iVar];
            else if ( Value == 2 )
                uCube &=  pSimInfo[iVar];
        }
        uResult |= uCube;
    }
    return uResult;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



/**CFile****************************************************************

  FileName    [ntkiRetime.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Retiming procedures.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkiFraigRetime.c,v 1.3 2005/07/08 01:43:21 alanmi Exp $]

***********************************************************************/
 
#include "ntkiInt.h"
//#include "retiInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

//extern void Reti_Experiment( Reti_Graph_t * p );
extern void Fraig_ManRetimeAig( Fraig_Man_t * pMan, int nLatches, int fVerbose );

//static void Ntk_NodeMarkTfo_rec( Ntk_Node_t * pNode );
//static int  Ntk_NodeComputeDelay_rec( Ntk_Node_t * pNode, Ntk_Node_t * pPivot );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Retimes the AIG with choice nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkFraigRetime( Ntk_Network_t * pNet, int fFuncRed, int fBalance, int fChoicing, int fVerbose )
{
    Fraig_Man_t * pMan;
    int fRefCount = 0;
    int fFeedBack = 1;
    int fDoSparse = 0;
    int clk;

    // quit if the network is not binary
    if ( !Ntk_NetworkIsBinary(pNet) )
    {
        printf( "Currently can only choice binary networks.\n" );
        return;
    }

    if ( Ntk_NetworkReadLatchNum(pNet) == 0 )
    {
        printf( "The network has no latches.\n" );
        return;
    }

    // derive the FRAIG manager containing the network
    pMan = Ntk_NetworkFraig( pNet, fFuncRed, fBalance, fDoSparse, fChoicing, fVerbose );

clk = clock();
    Fraig_ManRetimeAig( pMan, Ntk_NetworkReadLatchNum(pNet), 0 );
PRT( "FRAIG retiming", clock() - clk );

    // free the manager
    Fraig_ManFree( pMan );
}




#if 0

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Network_t * Ntk_NetworkRetime2( Ntk_Network_t * pNet, int fVerbose )
{
    Ntk_Latch_t * pLatch;
    Ntk_Node_t * pNode;
    Reti_Graph_t * p;
    int i, Delay;

    if ( Ntk_NetworkReadLatchNum(pNet) == 0 )
    {
        printf( "The network has no latches.\n" );
        return NULL;
    }

    // set the latch numbers
    i = 0;
    Ntk_NetworkForEachLatch( pNet, pLatch )
        pLatch->pInput->pData = (char *)i++;

    // compute the edges for each latch input
    p = Reti_GraphAlloc();
    i = 0;
    Ntk_NetworkForEachLatch( pNet, pLatch )
    {
        // mark the TFO of the latch output (and collect the dependent latch inputs)
        Ntk_NetworkIncrementTravId( pNet );
        Ntk_NetworkStartSpecial( pNet );
        Ntk_NodeMarkTfo_rec( pLatch->pOutput );
        Ntk_NetworkStopSpecial( pNet );

        // compute the delay of these POs
        Ntk_NetworkIncrementTravId( pNet );
        Ntk_NetworkForEachNodeSpecial( pNet, pNode )
        {
            Delay = Ntk_NodeComputeDelay_rec( pNode, pLatch->pOutput );
            Reti_GraphAddEdge( p, i, (int)pNode->pData, Delay );
        }
        i++;
    }

    // perform some kind of computation
    Reti_Experiment( p );
    
    return NULL;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeMarkTfo_rec( Ntk_Node_t * pNode )
{
	Ntk_Node_t * pFanout;
    Ntk_Pin_t * pPin;
    // if this node is already visited, skip
    if ( Ntk_NodeIsTravIdCurrent( pNode ) )
        return;
    // mark the node as visited
    Ntk_NodeSetTravIdCurrent( pNode );
    // add the node if it is a Latch Input
    if ( pNode->Type == MV_NODE_CO && pNode->Subtype == MV_BELONGS_TO_LATCH )
    {
        Ntk_NetworkAddToSpecial( pNode );
        return;
    }
    // visit the fanouts
    Ntk_NodeForEachFanout( pNode, pPin, pFanout )
        Ntk_NodeMarkTfo_rec( pFanout );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NodeComputeDelay_rec( Ntk_Node_t * pNode, Ntk_Node_t * pPivot )
{
    Ntk_Node_t * pFanin;
    Ntk_Pin_t * pPin;
    int DelayCur;
    // if this node is already visited, skip
    if ( Ntk_NodeIsTravIdCurrent( pNode ) )
        return pNode->Level;
    // if this node is not visited in the previous traversal, skip it
    if ( !Ntk_NodeIsTravIdPrevious( pNode ) )
        return -1;
    if ( pNode == pPivot )
        return 0;
    assert( pNode->Type != MV_NODE_CI );
    // mark the node as visited
    Ntk_NodeSetTravIdCurrent( pNode );
    // visit the fanins
    pNode->Level = 0;
    Ntk_NodeForEachFanin( pNode, pPin, pFanin )
    {
        DelayCur = Ntk_NodeComputeDelay_rec( pFanin, pPivot );
        if ( pNode->Level < DelayCur )
            pNode->Level = DelayCur;
    }
    pNode->Level = pFanin->Level + 1;
    return pNode->Level;
}

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



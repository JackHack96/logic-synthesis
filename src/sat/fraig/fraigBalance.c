/**CFile****************************************************************

  FileName    [fraigBalance.c]

  PackageName [FRAIG: Functionally reduced AND-INV graphs.]

  Synopsis    [Creating an AIG balanced by delay without increasing area.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.1. Started - October 1, 2004]

  Revision    [$Id: fraigBalance.c,v 1.6 2005/07/08 01:01:30 alanmi Exp $]

***********************************************************************/

#include "fraigInt.h"
#include "extra.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Fraig_Node_t * Fraig_ManBalanceNode_rec( Fraig_Man_t * pManNew, Fraig_Node_t * pNode, int fAreaDup );
static Fraig_Node_t * Fraig_ManBalanceMultiAnd_rec( Fraig_Man_t * pManNew, Fraig_NodeVec_t * vNodes );
static void Fraig_BalanceCollect( Fraig_Node_t * pNode, Fraig_NodeVec_t * vNodes, int fAreaDup );
static void Fraig_BalanceCollect_rec( Fraig_Node_t * pNode, Fraig_NodeVec_t * vNodes, int fAreaDup, int fFirst );
static Fraig_Node_t * Fraig_ManSelectNode_rec( Fraig_Man_t * pManNew, Fraig_Node_t * pNodeOld );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates a well-balanced AIG without increasing area.]

  Description [Looks at the AIG reachable from the PO nodes of the manager.
  Balances this AIG by delay (unit delay model) and returns it in the new
  manager.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_Man_t * Fraig_ManBalance( Fraig_Man_t * pMan, int fAreaDup, int * pInputArrivals )
{
    ProgressBar * pProgress;
    Fraig_Man_t * pManNew;
    Fraig_Node_t * pNodeNew;
    int TimePrint;
    int i;

    // clone the fraig manager
    pManNew = Fraig_ManClone( pMan );
    pManNew->fFuncRed = 0;

    // mark the fanout free cones in the old manager (also cleans pNode->pData0)
    Fraig_ManMarkRealFanouts( pMan );

    // set the correspondence between the PIs and the constant node
    pMan->pConst1->pData0 = pManNew->pConst1;
    for ( i = 0; i < pMan->vInputs->nSize; i++ )
    {
        // get the i-th input of the new manager
        pNodeNew = Fraig_ManReadIthVar( pManNew, i );
        assert( pNodeNew->Level == 0 );
        // set its level according to the arrival times
        if ( pInputArrivals ) pNodeNew->Level = pInputArrivals[i];
        // store the new input in the old input
        pMan->vInputs->pArray[i]->pData0 = pNodeNew;
    }

    // recreate the AIG in the new manager, while balancing it
    pProgress = Extra_ProgressBarStart( stdout, pMan->vOutputs->nSize );
    TimePrint = clock() + CLOCKS_PER_SEC/5;
    for ( i = 0; i < pMan->vOutputs->nSize; i++ )
    {
        pNodeNew = Fraig_ManBalanceNode_rec( pManNew, Fraig_Regular(pMan->vOutputs->pArray[i]), fAreaDup );
        pNodeNew = Fraig_NotCond( pNodeNew, Fraig_IsComplement(pMan->vOutputs->pArray[i]) );
        Fraig_ManSetPo( pManNew, pNodeNew );
        if ( clock() > TimePrint )
        {
            Extra_ProgressBarUpdate( pProgress, i, NULL );
            TimePrint = clock() + CLOCKS_PER_SEC/5;
        }
    }
    Extra_ProgressBarStop( pProgress );
    return pManNew;
}

/**Function*************************************************************

  Synopsis    [Balances the representation of the AND-tree rooted in the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_Node_t * Fraig_ManBalanceNode_rec( Fraig_Man_t * pManNew, Fraig_Node_t * pNodeOld, int fAreaDup )
{
    Fraig_Node_t * pNodeNew;
    Fraig_NodeVec_t * vNodes;
    int i;

    // make sure the node is not complemented
    assert( !Fraig_IsComplement(pNodeOld) );
    // if the node is visited, return the resulting node
    pNodeNew = (Fraig_Node_t *)pNodeOld->pData0;
    if ( pNodeNew )
        return pNodeNew;
    // make sure the node is an AND-gate
    assert( Fraig_NodeIsAnd(pNodeOld) );

    // get the nodes, which will become inputs of a multi-input AND gate
    vNodes = Fraig_NodeVecAlloc( 10 );
    Fraig_BalanceCollect( pNodeOld, vNodes, fAreaDup );
    // build the new nodes for them
    for ( i = 0; i < vNodes->nSize; i++ )
    {
        pNodeNew = Fraig_ManBalanceNode_rec( pManNew, Fraig_Regular(vNodes->pArray[i]), fAreaDup );
        vNodes->pArray[i] = Fraig_NotCond( pNodeNew, Fraig_IsComplement(vNodes->pArray[i]) );
    }

    // create the binary AND gate with inputs in the corresponding polarity
    pNodeNew = Fraig_ManBalanceMultiAnd_rec( pManNew, vNodes );
    Fraig_NodeVecFree( vNodes );

    // save the node
    pNodeOld->pData0 = (Fraig_Node_t *)pNodeNew;
    return pNodeNew;
}

/**Function*************************************************************

  Synopsis    [Returns the array of nodes to be combined into one multi-input AND-gate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_BalanceCollect( Fraig_Node_t * pNode, Fraig_NodeVec_t * vNodes, int fAreaDup )
{
    Fraig_BalanceCollect_rec( pNode, vNodes, fAreaDup, 1 );
}

/**Function*************************************************************

  Synopsis    [Returns the array of nodes to be combined into one multi-input AND-gate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_BalanceCollect_rec( Fraig_Node_t * pNode, Fraig_NodeVec_t * vNodes, int fAreaDup, int fFirst )
{
    // if the new node is complemented, another gate begins
    if ( Fraig_IsComplement(pNode) )
    {
        Fraig_NodeVecPushUnique( vNodes, pNode );
        return;
    }
    // if pNew is the PI node, return
    if ( !Fraig_NodeIsAnd(pNode) )
    {
        Fraig_NodeVecPushUnique( vNodes, pNode );
        return;    
    }
    if ( !fAreaDup && !fFirst && pNode->nFanouts > 1 )
    {
        Fraig_NodeVecPushUnique( vNodes, pNode );
        return;    
    }
    // go through the branches
    Fraig_BalanceCollect_rec( pNode->p1, vNodes, fAreaDup, 0 );
    Fraig_BalanceCollect_rec( pNode->p2, vNodes, fAreaDup, 0 );
}

/**Function*************************************************************

  Synopsis    [Compares the nodes according to the accepted order.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_NodeCompareByLevel( Fraig_Node_t ** ppN1, Fraig_Node_t ** ppN2 )
{
    Fraig_Node_t * pNode1 = Fraig_Regular(*ppN1);
    Fraig_Node_t * pNode2 = Fraig_Regular(*ppN2);
    if ( pNode1->Level > pNode2->Level )
        return -1;
    if ( pNode1->Level < pNode2->Level )
        return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Balances the representation of a multi-input AND-gate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_Node_t * Fraig_ManBalanceMultiAnd_rec( Fraig_Man_t * pManNew, Fraig_NodeVec_t * vNodes )
{
    Fraig_Node_t * pRoot, * pTemp;
    Fraig_Node_t ** ppNodesNew = vNodes->pArray;
    int nNodes = vNodes->nSize;
    int i;

    // q-sort the pointers by delay (the latest arriving go first)
    qsort( (void *)ppNodesNew, nNodes, sizeof(Fraig_Node_t *), 
            (int (*)(const void *, const void *)) Fraig_NodeCompareByLevel );
    assert( Fraig_NodeCompareByLevel( ppNodesNew, ppNodesNew + nNodes - 1 ) <= 0 );

    while ( nNodes > 1 )
    {
	    // merge the earliest two signals and create a new gate
	    assert( Fraig_Regular(ppNodesNew[nNodes-2])->Level >= Fraig_Regular(ppNodesNew[nNodes-1])->Level );
	    pRoot = Fraig_NodeAnd( pManNew, ppNodesNew[nNodes-2], ppNodesNew[nNodes-1] );

        // place it into the array
	    ppNodesNew[nNodes-2] = pRoot;

	    // Now push this newly introduced node into the correct position in the sorted array
        for ( i = nNodes-2; i > 0; i-- )
        {
            if ( Fraig_Regular(ppNodesNew[i-1])->Level >= Fraig_Regular(ppNodesNew[i])->Level )
                break;
            // swap the nodes
		    pTemp           = ppNodesNew[i-1];
		    ppNodesNew[i-1] = ppNodesNew[i];
		    ppNodesNew[i]   = pTemp;
	    }

	    // decrement the number of nodes
	    nNodes--;
    }

    // assign the root gate
    pRoot = ppNodesNew[0];
    return pRoot;
}



/**Function*************************************************************

  Synopsis    [Selects one choice at each choice node using levels and area flow.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_Man_t * Fraig_ManSelect( Fraig_Man_t * pMan )
{
    Fraig_Man_t * pManNew;
    Fraig_Node_t * pNode;
    int i;
    // clone the fraig manager
    pManNew = Fraig_ManClone( pMan );
    pManNew->fFuncRed = 0;
    // mark the best choice at each node (also clean pNode->pData0)
//    Fraig_ManSelectBestChoice( pMan );
    // set the correspondence between the PIs using pNode->pData0
    pMan->pConst1->pData0 = (Fraig_Node_t *)pManNew->pConst1->pData0;
    for ( i = 0; i < pMan->vInputs->nSize; i++ )
        pMan->vNodes->pArray[i]->pData0 = (Fraig_Node_t *)pManNew->vNodes->pArray[i];
    // recreate the AIG in the new manager, while selecting the best choice
    for ( i = 0; i < pManNew->vOutputs->nSize; i++ )
    {
        pNode = Fraig_ManSelectNode_rec( pManNew, Fraig_Regular(pMan->vOutputs->pArray[i]) );
        pNode = Fraig_NotCond( pNode, Fraig_IsComplement(pMan->vOutputs->pArray[i]) );
        Fraig_ManSetPo( pManNew, pNode );
    }
    return pManNew;
}

/**Function*************************************************************

  Synopsis    [Balances the representation of the AND-tree rooted in the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_Node_t * Fraig_ManSelectNode_rec( Fraig_Man_t * pManNew, Fraig_Node_t * pNodeOld )
{
    Fraig_Node_t * pNodeNew, * pNodeOldBest, * pNodeOldE;
    Fraig_Node_t * pNodeNew1, * pNodeNew2;

    // make sure the node is not complemented
    assert( !Fraig_IsComplement(pNodeOld) );
    // if the node is visited, return the resulting node
    pNodeNew = (Fraig_Node_t *)pNodeOld->pData0;
    if ( pNodeNew )
        return pNodeNew;
    // make sure the node is an AND-gate
    assert( Fraig_NodeIsAnd(pNodeOld) );

    // select the best node according to the cost functions
    pNodeOldBest = pNodeOld;
    for ( pNodeOldE = pNodeOld->pNextE; pNodeOldE; pNodeOldE = pNodeOldE->pNextE )
    {
//        if ( pNodeOldBest->NumTemp >  pNodeOldE->NumTemp || 
//             pNodeOldBest->NumTemp == pNodeOldE->NumTemp && pNodeOldBest->aFlow > pNodeOldE->aFlow )
             pNodeOldBest = pNodeOldE;
    }
//    assert( pNodeOldBest->NumTemp == pNodeOld->NumTemp );

    // solve the subproblems
    pNodeNew1 = Fraig_ManSelectNode_rec( pManNew, Fraig_Regular(pNodeOldBest->p1) );
    pNodeNew1 = Fraig_NotCond( pNodeNew1, Fraig_IsComplement(pNodeOldBest->p1) );

    pNodeNew2 = Fraig_ManSelectNode_rec( pManNew, Fraig_Regular(pNodeOldBest->p2) );
    pNodeNew2 = Fraig_NotCond( pNodeNew2, Fraig_IsComplement(pNodeOldBest->p2) );

    // complement the node if the selected one is complemented
    pNodeNew = Fraig_NodeAnd( pManNew, pNodeNew1, pNodeNew2 ); 
    pNodeNew = Fraig_NotCond( pNodeNew, Fraig_NodeComparePhase(pNodeOldBest, pNodeOld) );

    // save the node
    pNodeOld->pData0 = (Fraig_Node_t *)pNodeNew;
    return pNodeNew;
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



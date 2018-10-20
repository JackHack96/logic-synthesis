/**CFile****************************************************************

  FileName    [ntkiFxu.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    []

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkiFxu.c,v 1.5 2004/02/14 01:56:26 alanmi Exp $]

***********************************************************************/

#include "ntkiInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Ntk_NetworkFxCollectInfo( Ntk_Network_t * pNet, Fxu_Data_t * p );
static void Ntk_NetworkFxSetupLiterals( Ntk_Network_t * pNet, Ntk_Node_t * pNode, Mvc_Cover_t * pCover );
static void Ntk_NetworkFxReconstruct( Ntk_Network_t * pNet, Fxu_Data_t * p );
static void Ntk_NetworkFxReconstructNode( Ntk_Node_t * pNodeNew, Ntk_Node_t ** ppNodes, Fxu_Data_t * p, int iValue );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs fast_extract on binary or MV network.]

  Description [Takes the network and the maximum number of nodes to extract.
  Uses the concurrent double-cube and single cube divisor extraction procedure.
  Modifies the network in the end, after extracting all nodes. Note that 
  Ntk_NetworkSweep() may increase the performance of this procedure because 
  the single-literal nodes will not be created in the sparse matrix. Also, 
  make sure that the default value if used whenever possible (the def value 
  may be impossible in ND nodes). Otherwise, the common logic will be extracted 
  from the default covers where it is useless. Returns 1 if the network has 
  been changed.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Ntk_NetworkFastExtract( Ntk_Network_t * pNet, Fxu_Data_t * p )
{
    // sweep removes useless nodes
    Ntk_NetworkSweep( pNet, 1, 1, 1, 0 );

    // collect information about MV covers needed for FX
    // make sure all covers are SCC free
    // allocate literal array for each cover
    Ntk_NetworkFxCollectInfo( pNet, p );

    // call the fast extract procedure
    // returns the number of divisor extracted
    if ( Fxu_FastExtract(p) > 0 )
    {
        // update the network
        Ntk_NetworkFxReconstruct( pNet, p );
        // make sure everything is okay
        Ntk_NetworkCheck( pNet );
        return 1;
    }
    return 0;
}


/**Function*************************************************************

  Synopsis    [Collect information about the network for fast_extract.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkFxCollectInfo( Ntk_Network_t * pNet, Fxu_Data_t * p )
{
    Ntk_Node_t * pNode;
    Mvc_Cover_t ** ppIsets;
    int nNodesCi, nNodesInt;
    int nValuesCi, nValuesInt;
    int iNode, iValue, i;

    // the MVC manager
    p->pManMvc = Ntk_NetworkReadManMvc(pNet);

    // get the number of values
    nValuesCi  = 0;
    nValuesInt = 0;
    nNodesCi = 0;
    nNodesInt = 0;
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        // set values at the node
        pNode->pCopy = (Ntk_Node_t *)nValuesCi;
        // increment
        nValuesCi += pNode->nValues;
        nNodesCi++;
    }
    Ntk_NetworkForEachNode( pNet, pNode )
    {
        if ( Ntk_NodeReadFaninNum(pNode) == 0 )
            continue;

        // set values at the node
        pNode->pCopy = (Ntk_Node_t *)(nValuesCi + nValuesInt);
        // increment
        nValuesInt += pNode->nValues;
        nNodesInt++;
    }

    // nodes
    p->nNodesCi     = nNodesCi;
    p->nNodesInt    = nNodesInt;
    p->nNodesOld    = p->nNodesCi + p->nNodesInt;
    p->nNodesNew    = 0;
//    p->nNodesExt  is already assigned
    p->nNodesAlloc  = p->nNodesOld + p->nNodesExt;
    p->pNode2Value  = ALLOC( int, p->nNodesAlloc + 1 );  // first value of each node
    // values
    p->nValuesCi    = nValuesCi;  
    p->nValuesInt   = nValuesInt; 
    p->nValuesOld   = p->nValuesCi + p->nValuesInt;
    p->nValuesNew   = 0;
    p->nValuesExt   = p->nNodesExt * 2;
    p->nValuesAlloc = p->nValuesOld + p->nValuesExt;
    p->pValue2Node  = ALLOC( int, p->nValuesAlloc + 1 ); // the node of each value
    // covers
    p->ppCovers     = ALLOC( Mvc_Cover_t *, p->nValuesAlloc );
    p->ppCoversNew  = ALLOC( Mvc_Cover_t *, p->nValuesAlloc );
//    memset( p->ppCoversNew, 0, sizeof(Mvc_Cover_t *) + p->nValuesAlloc );

    // collect the info from the network
    iNode  = 0;
    iValue = 0;
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        // set the first value of this node
        p->pNode2Value[iNode] = iValue;
        // set the node of these values
        for ( i = iValue; i < iValue + pNode->nValues; i++ )
        {
            p->pValue2Node[i] = iNode;
            p->ppCovers[i]    = NULL;
        }
        // increment
        iNode++;
        iValue += pNode->nValues;
        // check if the network is MV
        if ( pNode->nValues > 2 )
            p->fMvNetwork = 1;
    }
    Ntk_NetworkForEachNode( pNet, pNode )
    {
        if ( Ntk_NodeReadFaninNum(pNode) == 0 )
            continue;

        // get the i-sets of this node
        ppIsets = Cvr_CoverReadIsets( Ntk_NodeGetFuncCvr(pNode) );

        // set the first value of this node
        p->pNode2Value[iNode] = iValue;
        // set the node of these values
        for ( i = 0; i < pNode->nValues; i++ )
        {
            p->pValue2Node[iValue+i] = iNode;
            p->ppCovers[iValue+i] = ppIsets[i];
            if ( ppIsets[i] )
            {
                // make sure the cover is SCC-free
                Mvc_CoverContain( ppIsets[i] );
                // this should be guaranteed by the reader

                // set up the literals of this i-set
                // the literal numbers are stored in pNode->pCopy
                Ntk_NetworkFxSetupLiterals( pNet, pNode, ppIsets[i] );
            }
        }
        // increment
        iNode++;
        iValue += pNode->nValues;
        // check if the network is MV
        if ( pNode->nValues > 2 )
            p->fMvNetwork = 1;
    }
    // set the last entry
    p->pNode2Value[iNode] = iValue;
    p->pValue2Node[iValue] = iNode;
    // verify
    assert( p->nNodesOld  == iNode  );
    assert( p->nValuesOld == iValue );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkFxFreeInfo( Fxu_Data_t * p )
{
    FREE( p->pNode2Value );
    FREE( p->pValue2Node );
    FREE( p->ppCovers );
    FREE( p->ppCoversNew );
    FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkFxSetupLiterals( Ntk_Network_t * pNet, Ntk_Node_t * pNode, Mvc_Cover_t * pCover )
{
    Ntk_Node_t * pFanin;
    Ntk_Pin_t * pPin;
    int iValue, i;

    // allocate storage for literals
    Mvc_CoverAllocateArrayLits( pCover );

    // set the literal numbers
    iValue = 0;
    Ntk_NodeForEachFanin( pNode, pPin, pFanin )
    {
        for ( i = 0; i < pFanin->nValues; i++ )
            pCover->pLits[iValue+i] = (int)pFanin->pCopy + i;
        iValue += pFanin->nValues;
    }
}

/**Function*************************************************************

  Synopsis    [Recostructs the network after FX.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkFxReconstruct( Ntk_Network_t * pNet, Fxu_Data_t * p )
{
    Ntk_Node_t ** ppNodes;
    Ntk_Node_t * pNode, * pNodeNew;
    int iNode, iValue, i, CountZeros;

    assert( p->nNodesNew > 0 );

    // collect the existent nodes
    ppNodes = ALLOC( Ntk_Node_t *, p->nNodesOld + p->nNodesNew );
    iNode = 0;
    Ntk_NetworkForEachCi( pNet, pNode )
        ppNodes[iNode++] = pNode;
    Ntk_NetworkForEachNode( pNet, pNode )
    {
        if ( Ntk_NodeReadFaninNum(pNode) == 0 )
            continue;
        ppNodes[iNode++] = pNode;
    }
    assert( iNode == p->nNodesOld );
 
    // create the new binary-output nodes for the extracted part
    for ( i = 0; i < p->nNodesNew; i++ )
    {
        pNodeNew = Ntk_NodeCreate( pNet, NULL, MV_NODE_INT, 2 );
        ppNodes[iNode + i] = pNodeNew;
    }

    // update the internal nodes, if they change
    iValue = p->nValuesCi;
    Ntk_NetworkForEachNode( pNet, pNode )
    {
        if ( Ntk_NodeReadFaninNum(pNode) == 0 )
            continue;

        // count the number of empty sets
        CountZeros = 0;
        for ( i = 0; i < pNode->nValues; i++ )
            if ( p->ppCoversNew[iValue+i] == NULL )
                CountZeros++;
        assert( CountZeros == 0 || CountZeros == 1 || CountZeros == pNode->nValues );

        if ( CountZeros == pNode->nValues )
        { // the node does not change
            // increment
            iValue += pNode->nValues;
            continue;
        }
        // the node is going to change

        // create a new node
        pNodeNew = Ntk_NodeCreate( pNet, NULL, MV_NODE_INT, pNode->nValues );
        // set up this node using extraction info
        Ntk_NetworkFxReconstructNode( pNodeNew, ppNodes, p, iValue );
        // update the node
        Ntk_NodeReplace( pNode, pNodeNew ); // disposes of pNodeNew
        // increment
        iValue += pNode->nValues;
    }
    assert( iValue == p->nValuesOld );

    // set up the new nodes
    for ( i = 0; i < p->nNodesNew; i++ )
    {
        // get the new node
        pNodeNew = ppNodes[p->nNodesOld + i];
        // set up this node using extraction info
        Ntk_NetworkFxReconstructNode( pNodeNew, ppNodes, p, iValue );
        // add the node
        Ntk_NetworkAddNode( pNet, pNodeNew, 1 );
        // increment
        iValue += pNodeNew->nValues;
    }
    FREE( ppNodes );

    // order the fanins of all nodes
    Ntk_NetworkForEachNode( pNet, pNode )
        Ntk_NodeOrderFanins( pNode );
}



/**Function*************************************************************

  Synopsis    [Create new node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkFxReconstructNode( Ntk_Node_t * pNodeNew, Ntk_Node_t ** ppNodes, Fxu_Data_t * p, int iValue )
{
    Cvr_Cover_t * pCvr;
    Vm_VarMap_t * pVm;
    Mvc_Cover_t ** ppIsets;
    Mvc_Cover_t * pCover;
    int iFanin, iFaninCur;
    int i;

    // find the first cover corresponding to this node
    pCover = NULL;
    for ( i = 0; i < pNodeNew->nValues; i++ )
        if ( p->ppCoversNew[iValue+i] && p->ppCoversNew[iValue+i]->pLits )
        {
            pCover = p->ppCoversNew[iValue+i];
            break;
        }
    // make sure all covers corresponding to one node has the same size
    for ( ; i < pNodeNew->nValues; i++ )
        if ( p->ppCoversNew[iValue+i] )
        {
            assert( pCover->nBits == p->ppCoversNew[iValue+i]->nBits );
        }

    // add the fanins
    iFanin = -1;
    for ( i = 0; i < pCover->nBits; i++ )
    {
        // get the node corresponding to this value
        iFaninCur = p->pValue2Node[pCover->pLits[i]];
        if ( iFanin != iFaninCur )
        {
            iFanin = iFaninCur;
            Ntk_NodeAddFanin( pNodeNew, ppNodes[iFanin] );
        }
    }

    // create the variable map
    pVm = Ntk_NodeAssignVm( pNodeNew );
    assert( pCover->nBits == Vm_VarMapReadValuesInNum(pVm) );

    // create the covers
    ppIsets = ALLOC( Mvc_Cover_t *, pNodeNew->nValues );
    memcpy( ppIsets, p->ppCoversNew + iValue, sizeof(Mvc_Cover_t *) * pNodeNew->nValues );
    pCvr = Cvr_CoverCreate( pVm, ppIsets );
    // insert the cover into the node
    Ntk_NodeWriteFuncCvr( pNodeNew, pCvr );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



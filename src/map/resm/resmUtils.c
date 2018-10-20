/**CFile****************************************************************

  FileName    [resmUtils.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic resynthesis engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: resmUtils.c,v 1.1 2005/01/23 06:59:54 alanmi Exp $]

***********************************************************************/

#include "resmInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static unsigned Resm_MatchExpandTruth_rec( unsigned uTruth, int nVars );
static void Resm_MappingDfs_rec( Resm_Node_t * pNode, Resm_NodeVec_t * vNodes );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes the DFS ordering of nodes used in the mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Resm_NodeVec_t * Resm_CollectNodesDfs( Resm_Man_t * p )
{
    Resm_NodeVec_t * vNodes;
    int i;
    // perform the traversal
    vNodes = Resm_NodeVecAlloc( 100 );
    Resm_MappingDfs_rec( p->pPseudo, vNodes );
    for ( i = 0; i < vNodes->nSize; i++ )
        vNodes->pArray[i]->fMark0 = 0;
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Recursively computes the DFS ordering of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Resm_MappingDfs_rec( Resm_Node_t * pNode, Resm_NodeVec_t * vNodes )
{
    int i;
    assert( !Resm_IsComplement(pNode) );
    if ( pNode->fMark0 )
        return;
    assert( Resm_NodeReadRefTotal(pNode) > 0 );
    // visit the transitive fanin
    for ( i = 0; i < (int)pNode->nLeaves; i++ )
        Resm_MappingDfs_rec( Resm_Regular(pNode->ppLeaves[i]), vNodes );
    // make sure the node is not visited through the equivalent nodes
    assert( pNode->fMark0 == 0 );
    // mark the node as visited
    pNode->fMark0 = 1;
    // add the node to the list
    Resm_NodeVecPush( vNodes, pNode );
}

/**Function*************************************************************

  Synopsis    [Computes the DFS ordering of nodes used in the mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Resm_VerifyRefs( Resm_Man_t * p )
{
    Resm_NodeVec_t * vNodes;
    Resm_Node_t * pNode;
    int i;
    for ( i = 0; i < p->vNodesAll->nSize; i++ )
        assert( p->vNodesAll->pArray[i]->fMark0 == 0 );
    // perform the traversal
    vNodes = Resm_NodeVecAlloc( 100 );
    Resm_MappingDfs_rec( p->pPseudo, vNodes );
    // make sure that the marked are refed and non-marked nodes are non-referenced
    for ( i = 0; i < p->vNodesAll->nSize; i++ )
    {
        pNode = p->vNodesAll->pArray[i];
        if(pNode->fMark0 == 0){
            assert(Resm_NodeReadRefTotal(pNode) == 0 );
        }
        else{
            assert(Resm_NodeReadRefTotal(pNode) > 0 );
        }
    }
    // clean the marks
    for ( i = 0; i < vNodes->nSize; i++ )
        vNodes->pArray[i]->fMark0 = 0;
    for ( i = 0; i < p->vNodesAll->nSize; i++ )
        assert( p->vNodesAll->pArray[i]->fMark0 == 0 );
    Resm_NodeVecFree( vNodes );
    return 1;
}


/**Function*************************************************************

  Synopsis    [Expand the truth table]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Resm_MatchExpandTruth( unsigned uTruth[2], int nVars )
{
    assert( nVars < 7 );
    if ( nVars == 6 )
        return;
    if ( nVars < 5 )
        uTruth[0] = Resm_MatchExpandTruth_rec( uTruth[0], nVars );
    uTruth[1] = uTruth[0];
}

/**Function*************************************************************

  Synopsis    [Expand the truth table]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Resm_MatchExpandTruth_rec( unsigned uTruth, int nVars )
{
    assert( nVars < 6 );
    if ( nVars == 5 )
        return uTruth;
    return Resm_MatchExpandTruth_rec( uTruth | (uTruth << (1 << nVars)), nVars + 1 );    
}


/**function*************************************************************

  synopsis    [Computes the exact area associated with the cut.]

  description []
               
  sideeffects []

  seealso     []

***********************************************************************/
float Resm_NodeGetAreaRefed( Resm_Node_t * pNode )
{
    float aResult, aResult2;
    aResult  = Resm_NodeAreaDeref( pNode );
    aResult2 = Resm_NodeAreaRef( pNode );
    assert( aResult == aResult2 );
    if ( pNode->nRefs[0] > 0 )
        aResult += pNode->p->AreaInv;
    return aResult;
}

/**function*************************************************************

  synopsis    [Computes the exact area associated with the cut.]

  description []
               
  sideeffects []

  seealso     []

***********************************************************************/
float Resm_NodeGetAreaDerefed( Resm_Node_t * pNode )
{
    float aResult, aResult2;
    aResult2 = Resm_NodeAreaRef( pNode );
    aResult  = Resm_NodeAreaDeref( pNode );
    assert( aResult == aResult2 );
    if ( pNode->nRefs[0] > 0 )
        aResult += pNode->p->AreaInv;
    return aResult;
}

/**function*************************************************************

  synopsis    [References the cut.]

  description [This procedure is similar to the procedure NodeReclaim.]
               
  sideeffects []

  seealso     []

***********************************************************************/
float Resm_NodeAreaRef( Resm_Node_t * pNode )
{
    Resm_Node_t * pChild;
    int i, fPhaseC;
    float aArea;

    // consider the elementary variable
    assert( !Resm_IsComplement(pNode) );
    if ( pNode->nLeaves == 0 )
        return 0;
    // start the area of this cut
    aArea = pNode->pGate? (float)Mio_GateReadArea( pNode->pGate ) : 0;
    // go through the children
    for ( i = 0; i < (int)pNode->nLeaves; i++ )
    {
        pChild  =  Resm_Regular(pNode->ppLeaves[i]);
        fPhaseC = !Resm_IsComplement(pNode->ppLeaves[i]);
        assert( Resm_NodeReadRefTotal(pChild) >= 0 );
        if ( fPhaseC == 0 && pChild->nRefs[0] == 0 ) 
            aArea += pNode->p->AreaInv;
        if ( Resm_NodeReadRefTotal(pChild) == 0 )  
            aArea += Resm_NodeAreaRef( pChild );
        Resm_NodeRef(pNode->ppLeaves[i]);
    }
    return aArea;
}


/**function*************************************************************

  synopsis    [Dereferences the cut.]

  description [This procedure is similar to the procedure NodeRecusiveDeref.]
               
  sideeffects []

  seealso     []

***********************************************************************/
float Resm_NodeAreaDeref( Resm_Node_t * pNode )
{
    Resm_Node_t * pChild;
    int i, fPhaseC;
    float aArea;

    // consider the elementary variable
    assert( !Resm_IsComplement(pNode) );
    if ( pNode->nLeaves == 0 )
        return 0;
    // start the area of this cut
    aArea = pNode->pGate? (float)Mio_GateReadArea( pNode->pGate ) : 0;
    // go through the children
    for ( i = 0; i < (int)pNode->nLeaves; i++ )
    {
        pChild  =  Resm_Regular(pNode->ppLeaves[i]);
        fPhaseC = !Resm_IsComplement(pNode->ppLeaves[i]);
        Resm_NodeDeref(pNode->ppLeaves[i]);
        assert( Resm_NodeReadRefTotal(pChild) >= 0 );
        if ( fPhaseC == 0 && pChild->nRefs[0] == 0 ) 
            aArea += pNode->p->AreaInv;
        if ( Resm_NodeReadRefTotal(pChild) == 0 )  
            aArea += Resm_NodeAreaDeref( pChild );
    }
    return aArea;
}

/**function*************************************************************

  synopsis    [Computes the total area of all nodes.]

  description []
               
  sideeffects []

  seealso     []

***********************************************************************/
float Resm_MappingArea( Resm_Man_t * p )
{
    Resm_NodeVec_t * vNodes;
    Resm_Node_t * pNode;
    float AreaTemp;
    int i;

    // collect the nodes used in the mapping
    vNodes = Resm_CollectNodesDfs( p );
    // for each cut used in the mapping, in the topological order
    AreaTemp = 0;
    for ( i = 0; i < vNodes->nSize; i++ )
    {
        pNode = vNodes->pArray[i];
        // make sure the node is referenced
        assert( Resm_NodeReadRefTotal(pNode) > 0 );
        if ( pNode->nRefs[0] > 0 )
            AreaTemp += p->AreaInv;
        // skip the PI node
        if ( Resm_NodeIsVar(pNode) || Resm_NodeIsConst(pNode) )
            continue;
        // add to the area of the network
        AreaTemp += (float)Mio_GateReadArea( pNode->pGate );
    }
    Resm_NodeVecFree( vNodes );
    return AreaTemp;
}

/**Function*************************************************************

  Synopsis    [Computes the static area.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Resm_ComputeStaticArea( Resm_Man_t * p )
{
    float AreaTemp;
    int i;
    // add two invertors for each buffer
    AreaTemp = 0;
    for ( i = 0; i < p->nOutputs; i++ )
        if ( !Resm_IsComplement(p->pOutputs[i]) && Resm_NodeIsVar(p->pOutputs[i]) )
            AreaTemp += 2 * p->AreaInv;
    return AreaTemp;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Resm_Node_t * Resm_NodeListAppend( Resm_Node_t * pNodeAll, Resm_Node_t * pNodes )
{
    Resm_Node_t * pPrev, * pTemp;
    if ( pNodeAll == NULL )
        return pNodes;
    if ( pNodes == NULL )
        return pNodeAll;
    // find the last one
    for ( pTemp = pNodes; pTemp; pTemp = pTemp->pNext )
        pPrev = pTemp;
    // append all the end of the current set
    assert( pPrev->pNext == NULL );
    pPrev->pNext = pNodeAll;
    return pNodes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Resm_NodeListRecycle( Resm_Man_t * p, Resm_Node_t * pNodeList, Resm_Node_t * pSave )
{
    Resm_Node_t * pNext, * pTemp;
    for ( pTemp = pNodeList, pNext = pTemp? pTemp->pNext : NULL; 
          pTemp; 
          pTemp = pNext, pNext = pNext? pNext->pNext : NULL )
        if ( pTemp != pSave )
            Extra_MmFixedEntryRecycle( p->mmNodes, (char *)pTemp );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Resm_NodeListCount( Resm_Node_t * pNodes )
{
    Resm_Node_t * pTemp;
    int i;
    for ( i = 0, pTemp = pNodes; pTemp; pTemp = pTemp->pNext, i++ );
    return i;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Resm_NodePrint( Resm_Node_t * pRoot )
{
    int i;
    printf( "NODE: %c At=(%5.2f, %5.2f) ", (pRoot->fCompl? 'c':' '), pRoot->tArrival.Rise, pRoot->tArrival.Fall );
    if ( pRoot->tRequired[1].Rise == RESM_FLOAT_LARGE )
        printf( " Rt=(--.--, --.--) " );
    else
        printf( " Rt=(%5.2f, %5.2f) ", pRoot->tRequired[1].Rise, pRoot->tRequired[1].Fall );
    printf( " A=%5.2f  %4d -> {", pRoot->aArea, pRoot->Num );
    for ( i = 0; i < (int)pRoot->nLeaves; i++ )
        printf( " %4d", Resm_Regular(pRoot->ppLeaves[i])->Num );
    printf( " } \n" );
}

/**Function*************************************************************

  Synopsis    [Check that there is no duplicates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Resm_CheckNoDuplicates( Resm_Node_t * pRoot, Resm_Node_t * ppNodes[], int nNodes )
{
    int i, v;
    if ( pRoot )
    for ( i = 0; i < nNodes; i++ )
        if ( pRoot == Resm_Regular(ppNodes[i]) )
        {
            printf( "Resm_CheckNoDuplicates(): Duplicated root and fanin nodes.\n" );
            assert( 0 );
            return 0;
        }
    for ( i = 0; i < nNodes; i++ )
    for ( v = i + 1; v < nNodes; v++ )
        if ( Resm_Regular(ppNodes[v]) == Resm_Regular(ppNodes[i]) )
        {
            printf( "Resm_CheckNoDuplicates(): Duplicated fanins of node %d.\n", Resm_Regular(ppNodes[i])->Num );
            assert( 0 );
            return 0;
        }
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



/**CFile****************************************************************

  FileName    [mntkUtils.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic mapped network.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: mntkUtils.c,v 1.2 2005/04/10 23:27:05 alanmi Exp $]

***********************************************************************/

#include "mntkInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static unsigned Mntk_MatchExpandTruth_rec( unsigned uTruth, int nVars );
static void Mntk_MappingDfs_rec( Mntk_Node_t * pNode, Mntk_NodeVec_t * vNodes );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes the DFS ordering of nodes used in the mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mntk_NodeVec_t * Mntk_CollectNodesDfs( Mntk_Man_t * p )
{
    Mntk_NodeVec_t * vNodes;
    int i;
    // perform the traversal
    vNodes = Mntk_NodeVecAlloc( 100 );
    Mntk_MappingDfs_rec( p->pPseudo, vNodes );
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
void Mntk_MappingDfs_rec( Mntk_Node_t * pNode, Mntk_NodeVec_t * vNodes )
{
    int i;
    assert( !Mntk_IsComplement(pNode) );
    if ( pNode->fMark0 )
        return;
    assert( Mntk_NodeReadRefTotal(pNode) > 0 );
    // visit the transitive fanin
    for ( i = 0; i < (int)pNode->nLeaves; i++ )
        Mntk_MappingDfs_rec( Mntk_Regular(pNode->ppLeaves[i]), vNodes );
    // make sure the node is not visited through the equivalent nodes
    assert( pNode->fMark0 == 0 );
    // mark the node as visited
    pNode->fMark0 = 1;
    // add the node to the list
    Mntk_NodeVecPush( vNodes, pNode );
}

/**Function*************************************************************

  Synopsis    [Computes the DFS ordering of nodes used in the mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mntk_VerifyRefs( Mntk_Man_t * p )
{
    Mntk_NodeVec_t * vNodes;
    Mntk_Node_t * pNode;
    int i;
    for ( i = 0; i < p->vNodesAll->nSize; i++ )
        assert( p->vNodesAll->pArray[i]->fMark0 == 0 );
    // perform the traversal
    vNodes = Mntk_NodeVecAlloc( 100 );
    Mntk_MappingDfs_rec( p->pPseudo, vNodes );
    // make sure that the marked are refed and non-marked nodes are non-referenced
    for ( i = 0; i < p->vNodesAll->nSize; i++ )
    {
        pNode = p->vNodesAll->pArray[i];
        if (pNode->fMark0 == 0){
            assert( Mntk_NodeReadRefTotal(pNode) == 0 );
        }
        else{
            assert( Mntk_NodeReadRefTotal(pNode) > 0 );
        }
    }
    // clean the marks
    for ( i = 0; i < vNodes->nSize; i++ )
        vNodes->pArray[i]->fMark0 = 0;
    for ( i = 0; i < p->vNodesAll->nSize; i++ )
        assert( p->vNodesAll->pArray[i]->fMark0 == 0 );
    Mntk_NodeVecFree( vNodes );
    return 1;
}


/**Function*************************************************************

  Synopsis    [Expand the truth table]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mntk_MatchExpandTruth( unsigned uTruth[2], int nVars )
{
    assert( nVars < 7 );
    if ( nVars == 6 )
        return;
    if ( nVars < 5 )
        uTruth[0] = Mntk_MatchExpandTruth_rec( uTruth[0], nVars );
    uTruth[1] = uTruth[0];
}

/**Function*************************************************************

  Synopsis    [Expand the truth table]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Mntk_MatchExpandTruth_rec( unsigned uTruth, int nVars )
{
    assert( nVars < 6 );
    if ( nVars == 5 )
        return uTruth;
    return Mntk_MatchExpandTruth_rec( uTruth | (uTruth << (1 << nVars)), nVars + 1 );    
}


/**function*************************************************************

  synopsis    [Computes the exact area associated with the cut.]

  description []
               
  sideeffects []

  seealso     []

***********************************************************************/
float Mntk_NodeGetAreaRefed( Mntk_Node_t * pNode )
{
    float aResult, aResult2;
    aResult  = Mntk_NodeAreaDeref( pNode );
    aResult2 = Mntk_NodeAreaRef( pNode );
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
float Mntk_NodeGetAreaDerefed( Mntk_Node_t * pNode )
{
    float aResult, aResult2;
    aResult2 = Mntk_NodeAreaRef( pNode );
    aResult  = Mntk_NodeAreaDeref( pNode );
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
float Mntk_NodeAreaRef( Mntk_Node_t * pNode )
{
    Mntk_Node_t * pChild;
    int i, fPhaseC;
    float aArea;

    // consider the elementary variable
    assert( !Mntk_IsComplement(pNode) );
    if ( pNode->nLeaves == 0 )
        return 0;
    // start the area of this cut
    aArea = pNode->pGate? (float)Mio_GateReadArea( pNode->pGate ) : 0;
    // go through the children
    for ( i = 0; i < (int)pNode->nLeaves; i++ )
    {
        pChild  =  Mntk_Regular(pNode->ppLeaves[i]);
        fPhaseC = !Mntk_IsComplement(pNode->ppLeaves[i]);
        assert( Mntk_NodeReadRefTotal(pChild) >= 0 );
        if ( fPhaseC == 0 && pChild->nRefs[0] == 0 ) 
            aArea += pNode->p->AreaInv;
        if ( Mntk_NodeReadRefTotal(pChild) == 0 )  
            aArea += Mntk_NodeAreaRef( pChild );
        Mntk_NodeRef(pNode->ppLeaves[i]);
    }
    return aArea;
}


/**function*************************************************************

  synopsis    [Dereferences the cut.]

  description [This procedure is similar to the procedure NodeRecusiveDeref.]
               
  sideeffects []

  seealso     []

***********************************************************************/
float Mntk_NodeAreaDeref( Mntk_Node_t * pNode )
{
    Mntk_Node_t * pChild;
    int i, fPhaseC;
    float aArea;

    // consider the elementary variable
    assert( !Mntk_IsComplement(pNode) );
    if ( pNode->nLeaves == 0 )
        return 0;
    // start the area of this cut
    aArea = pNode->pGate? (float)Mio_GateReadArea( pNode->pGate ) : 0;
    // go through the children
    for ( i = 0; i < (int)pNode->nLeaves; i++ )
    {
        pChild  =  Mntk_Regular(pNode->ppLeaves[i]);
        fPhaseC = !Mntk_IsComplement(pNode->ppLeaves[i]);
        Mntk_NodeDeref(pNode->ppLeaves[i]);
        assert( Mntk_NodeReadRefTotal(pChild) >= 0 );
        if ( fPhaseC == 0 && pChild->nRefs[0] == 0 ) 
            aArea += pNode->p->AreaInv;
        if ( Mntk_NodeReadRefTotal(pChild) == 0 )  
            aArea += Mntk_NodeAreaDeref( pChild );
    }
    return aArea;
}

/**function*************************************************************

  synopsis    [Computes the total area of all nodes.]

  description []
               
  sideeffects []

  seealso     []

***********************************************************************/
float Mntk_MappingArea( Mntk_Man_t * p )
{
    Mntk_NodeVec_t * vNodes;
    Mntk_Node_t * pNode;
    float AreaTemp;
    int i;

    // collect the nodes used in the mapping
    vNodes = Mntk_CollectNodesDfs( p );
    // for each cut used in the mapping, in the topological order
    AreaTemp = 0;
    for ( i = 0; i < vNodes->nSize; i++ )
    {
        pNode = vNodes->pArray[i];
        // make sure the node is referenced
        assert( Mntk_NodeReadRefTotal(pNode) > 0 );
        if ( pNode->nRefs[0] > 0 )
            AreaTemp += p->AreaInv;
        // skip the PI node
        if ( Mntk_NodeIsVar(pNode) || Mntk_NodeIsConst(pNode) )
            continue;
        // add to the area of the network
        AreaTemp += (float)Mio_GateReadArea( pNode->pGate );
    }
    Mntk_NodeVecFree( vNodes );
    return AreaTemp;
}

/**Function*************************************************************

  Synopsis    [Computes the static area.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Mntk_ComputeStaticArea( Mntk_Man_t * p )
{
    float AreaTemp;
    int i;
    // add two invertors for each buffer
    AreaTemp = 0;
    for ( i = 0; i < p->nOutputs; i++ )
        if ( !Mntk_IsComplement(p->pOutputs[i]) && Mntk_NodeIsVar(p->pOutputs[i]) )
            AreaTemp += 2 * p->AreaInv;
    return AreaTemp;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mntk_Node_t * Mntk_NodeListAppend( Mntk_Node_t * pNodeAll, Mntk_Node_t * pNodes )
{
    Mntk_Node_t * pPrev, * pTemp;
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
void Mntk_NodeListRecycle( Mntk_Man_t * p, Mntk_Node_t * pNodeList, Mntk_Node_t * pSave )
{
    Mntk_Node_t * pNext, * pTemp;
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
int Mntk_NodeListCount( Mntk_Node_t * pNodes )
{
    Mntk_Node_t * pTemp;
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
void Mntk_NodePrint( Mntk_Node_t * pRoot )
{
    int i;
    printf( "NODE: %c At=(%5.2f, %5.2f) ", (pRoot->fCompl? 'c':' '), pRoot->tArrival.Rise, pRoot->tArrival.Fall );
    if ( pRoot->tRequired[1].Rise == MNTK_FLOAT_LARGE )
        printf( " Rt=(--.--, --.--) " );
    else
        printf( " Rt=(%5.2f, %5.2f) ", pRoot->tRequired[1].Rise, pRoot->tRequired[1].Fall );
    printf( " A=%5.2f  %4d -> {", pRoot->aArea, pRoot->Num );
    for ( i = 0; i < (int)pRoot->nLeaves; i++ )
        printf( " %4d", Mntk_Regular(pRoot->ppLeaves[i])->Num );
    printf( " } \n" );
}

/**Function*************************************************************

  Synopsis    [Check that there is no duplicates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mntk_CheckNoDuplicates( Mntk_Node_t * pRoot, Mntk_Node_t * ppNodes[], int nNodes )
{
    int i, v;
    if ( pRoot )
    for ( i = 0; i < nNodes; i++ )
        if ( pRoot == Mntk_Regular(ppNodes[i]) )
        {
            printf( "Mntk_CheckNoDuplicates(): Duplicated root and fanin nodes.\n" );
            assert( 0 );
            return 0;
        }
    for ( i = 0; i < nNodes; i++ )
    for ( v = i + 1; v < nNodes; v++ )
        if ( Mntk_Regular(ppNodes[v]) == Mntk_Regular(ppNodes[i]) )
        {
            printf( "Mntk_CheckNoDuplicates(): Duplicated fanins of node %d.\n", Mntk_Regular(ppNodes[i])->Num );
            assert( 0 );
            return 0;
        }
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



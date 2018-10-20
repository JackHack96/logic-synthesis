/**CFile****************************************************************

  FileName    [fpgaUtils.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Technology mapping for variable-size-LUT FPGAs.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 18, 2004.]

  Revision    [$Id: fpgaUtils.c,v 1.7 2005/07/08 01:01:23 alanmi Exp $]

***********************************************************************/

#include "fpgaInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void  Fpga_MappingDfs_rec( Fpga_Node_t * pNode, Fpga_NodeVec_t * vNodes, int fCollectEquiv );
static void  Fpga_MappingDfsCuts_rec( Fpga_Node_t * pNode, Fpga_NodeVec_t * vNodes );
static float Fpga_MappingArea_rec( Fpga_Man_t * pMan, Fpga_Node_t * pNode, Fpga_NodeVec_t * vNodes );
static int   Fpga_MappingCountLevels_rec( Fpga_Node_t * pNode );
static void  Fpga_MappingMarkUsed_rec( Fpga_Node_t * pNode );
static int   Fpga_MappingCompareOutputDelay( int * pOut1, int * pOut2 );
static float Fpga_MappingSetRefsAndArea_rec( Fpga_Man_t * pMan, Fpga_Node_t * pNode );
static Fpga_Man_t * s_pMan = NULL;

static void  Fpga_DfsLim_rec( Fpga_Node_t * pNode, int Level, Fpga_NodeVec_t * vNodes );
static int Fpga_CollectNodeTfo_rec( Fpga_Node_t * pNode, Fpga_Node_t * pPivot, Fpga_NodeVec_t * vVisited, Fpga_NodeVec_t * vTfo );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    [Computes the DFS ordering of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_NodeVec_t * Fpga_MappingDfs( Fpga_Man_t * pMan, int fCollectEquiv )
{
    Fpga_NodeVec_t * vNodes;
    Fpga_Node_t * pNode;
    int i;
    // start the array
    vNodes = Fpga_NodeVecAlloc( 100 );
    // collect the PIs
    for ( i = 0; i < pMan->nInputs; i++ )
    {
        pNode = pMan->pInputs[i];
        Fpga_NodeVecPush( vNodes, pNode );
        pNode->fMark0 = 1;
    }
    // perform the traversal
    for ( i = 0; i < pMan->nOutputs; i++ )
        Fpga_MappingDfs_rec( Fpga_Regular(pMan->pOutputs[i]), vNodes, fCollectEquiv );
    for ( i = 0; i < vNodes->nSize; i++ )
        vNodes->pArray[i]->fMark0 = 0;
//    for ( i = 0; i < pMan->nOutputs; i++ )
//        Fpga_MappingUnmark_rec( Fpga_Regular(pMan->pOutputs[i]) );
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Recursively computes the DFS ordering of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_MappingDfs_rec( Fpga_Node_t * pNode, Fpga_NodeVec_t * vNodes, int fCollectEquiv )
{
    assert( !Fpga_IsComplement(pNode) );
    if ( pNode->fMark0 )
        return;
    // visit the transitive fanin
    if ( Fpga_NodeIsAnd(pNode) )
    {
        Fpga_MappingDfs_rec( Fpga_Regular(pNode->p1), vNodes, fCollectEquiv );
        Fpga_MappingDfs_rec( Fpga_Regular(pNode->p2), vNodes, fCollectEquiv );
    }
    // visit the equivalent nodes
    if ( fCollectEquiv && pNode->pNextE )
        Fpga_MappingDfs_rec( pNode->pNextE, vNodes, fCollectEquiv );
    // make sure the node is not visited through the equivalent nodes
    assert( pNode->fMark0 == 0 );
    // mark the node as visited
    pNode->fMark0 = 1;
    // add the node to the list
    Fpga_NodeVecPush( vNodes, pNode );
}

/**Function*************************************************************

  Synopsis    [Computes the DFS ordering of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_NodeVec_t * Fpga_MappingDfsNodes( Fpga_Man_t * pMan, Fpga_Node_t ** ppNodes, int nNodes, int fEquiv )
{
    Fpga_NodeVec_t * vNodes;
    int i;
    // perform the traversal
    vNodes = Fpga_NodeVecAlloc( 200 );
    for ( i = 0; i < nNodes; i++ )
        Fpga_MappingDfs_rec( ppNodes[i], vNodes, fEquiv );
    for ( i = 0; i < vNodes->nSize; i++ )
        vNodes->pArray[i]->fMark0 = 0;
    return vNodes;
}


/**Function*************************************************************

  Synopsis    [Computes the number of logic levels not counting PIs/POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fpga_CountLevels( Fpga_Man_t * pMan )
{
    int i, LevelsMax, LevelsCur;
    // perform the traversal
    LevelsMax = -1;
    for ( i = 0; i < pMan->nOutputs; i++ )
    {
        LevelsCur = Fpga_Regular(pMan->pOutputs[i])->Level;
        if ( LevelsMax < LevelsCur )
            LevelsMax = LevelsCur;
    }
    return LevelsMax;
}

/**Function*************************************************************

  Synopsis    [Computes the number of logic levels not counting PIs/POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fpga_CountLevelsNodes( Fpga_Man_t * pMan, Fpga_Node_t ** ppRoots, int nRoots )
{
    int i, LevelsMax, LevelsCur;
    // perform the traversal
    LevelsMax = -1;
    for ( i = 0; i < nRoots; i++ )
    {
        LevelsCur = Fpga_Regular(ppRoots[i])->Level;
        if ( LevelsMax < LevelsCur )
            LevelsMax = LevelsCur;
    }
    return LevelsMax;
}

/**Function*************************************************************

  Synopsis    [Computes the DFS ordering of the nodes visible in current mapping.]

  Description [The node is visible if it appears as a root of one of the best
  cuts (that is cuts selected for the current mapping).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_NodeVec_t * Fpga_MappingDfsCuts( Fpga_Man_t * pMan )
{
    Fpga_NodeVec_t * vNodes;
    int i;
    // perform the traversal
    vNodes = Fpga_NodeVecAlloc( 100 );
    for ( i = 0; i < pMan->nOutputs; i++ )
        Fpga_MappingDfsCuts_rec( Fpga_Regular(pMan->pOutputs[i]), vNodes );
    for ( i = 0; i < vNodes->nSize; i++ )
        vNodes->pArray[i]->fMark0 = 0;
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Computes the DFS ordering of the nodes visible in current mapping.]

  Description [The node is visible if it appears as a root of one of the best
  cuts (that is cuts selected for the current mapping).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_NodeVec_t * Fpga_MappingDfsCutsNode( Fpga_Man_t * pMan, Fpga_Node_t * pNode )
{
    Fpga_NodeVec_t * vNodes;
    int i;
    // perform the traversal
    vNodes = Fpga_NodeVecAlloc( 100 );
    Fpga_MappingDfsCuts_rec( pNode, vNodes );
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
void Fpga_MappingDfsCuts_rec( Fpga_Node_t * pNode, Fpga_NodeVec_t * vNodes )
{
    int i;
    assert( !Fpga_IsComplement(pNode) );
    if ( !Fpga_NodeIsAnd(pNode) )
        return;
    if ( pNode->fMark0 )
        return;
    assert( pNode->pCutBest != NULL );
    // visit the transitive fanin of the selected cut
    for ( i = 0; i < pNode->pCutBest->nLeaves; i++ )
        Fpga_MappingDfsCuts_rec( pNode->pCutBest->ppLeaves[i], vNodes );
    // make sure the node is not visited through the fanin nodes
    assert( pNode->fMark0 == 0 );
    // mark the node as visited
    pNode->fMark0 = 1;
    // add the node to the list
    Fpga_NodeVecPush( vNodes, pNode );
}


/**Function*************************************************************

  Synopsis    [Marks the nodes used in the mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_MappingMarkUsed( Fpga_Man_t * pMan )
{
    int i;
    for ( i = 0; i < pMan->nOutputs; i++ )
        Fpga_MappingMarkUsed_rec( Fpga_Regular(pMan->pOutputs[i]) );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_MappingMarkUsed_rec( Fpga_Node_t * pNode )
{
    int i;
    assert( !Fpga_IsComplement(pNode) );
    if ( pNode->fUsed )
        return;
    pNode->fUsed = 1;
    if ( !Fpga_NodeIsAnd(pNode) )
        return;
    assert( pNode->pCutBest != NULL );
    // visit the transitive fanin of the selected cut
    for ( i = 0; i < pNode->pCutBest->nLeaves; i++ )
        Fpga_MappingMarkUsed_rec( pNode->pCutBest->ppLeaves[i] );
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Fpga_MappingGetAreaFlow( Fpga_Man_t * p )
{
    float aFlowFlowTotal = 0;
    int i;
    for ( i = 0; i < p->nOutputs; i++ )
    {
        if ( Fpga_NodeIsConst(p->pOutputs[i]) )
            continue;
        aFlowFlowTotal += Fpga_Regular(p->pOutputs[i])->pCutBest->aFlow;
    }
    return aFlowFlowTotal;
}

/**Function*************************************************************

  Synopsis    [Computes the area of the current mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Fpga_MappingArea( Fpga_Man_t * pMan )
{
    Fpga_NodeVec_t * vNodes;
    float aTotal;
    int i;
    // perform the traversal
    aTotal = 0;
    vNodes = Fpga_NodeVecAlloc( 100 );
    for ( i = 0; i < pMan->nOutputs; i++ )
    {
        aTotal += Fpga_MappingArea_rec( pMan, Fpga_Regular(pMan->pOutputs[i]), vNodes );
        // add the area for single-input nodes (if any) at the POs
//        if ( Fpga_NodeIsVar(pMan->pOutputs[i]) || Fpga_IsComplement(pMan->pOutputs[i]) )
//            aTotal += pMan->pLutLib->pLutAreas[1];
    }
    for ( i = 0; i < vNodes->nSize; i++ )
        vNodes->pArray[i]->fMark0 = 0;
    Fpga_NodeVecFree( vNodes );
    return aTotal;
}

/**Function*************************************************************

  Synopsis    [Recursively computes the DFS ordering of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Fpga_MappingArea_rec( Fpga_Man_t * pMan, Fpga_Node_t * pNode, Fpga_NodeVec_t * vNodes )
{
    float aArea;
    int i;
    assert( !Fpga_IsComplement(pNode) );
    if ( !Fpga_NodeIsAnd(pNode) )
        return 0;
    if ( pNode->fMark0 )
        return 0;
    assert( pNode->pCutBest != NULL );
    // visit the transitive fanin of the selected cut
    aArea = 0;
    for ( i = 0; i < pNode->pCutBest->nLeaves; i++ )
        aArea += Fpga_MappingArea_rec( pMan, pNode->pCutBest->ppLeaves[i], vNodes );
    // make sure the node is not visited through the fanin nodes
    assert( pNode->fMark0 == 0 );
    // mark the node as visited
    pNode->fMark0 = 1;
    // add the node to the list
    aArea += pMan->pLutLib->pLutAreas[pNode->pCutBest->nLeaves];
    // add the node to the list
    Fpga_NodeVecPush( vNodes, pNode );
    return aArea;
}


/**Function*************************************************************

  Synopsis    [Sets the correct reference counts for the mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Fpga_MappingComputeCutAreas( Fpga_Man_t * pMan )
{
    Fpga_NodeVec_t * vNodes;
    Fpga_Node_t * pNode;
    float Area = 0;
    int i;
    // collect nodes reachable from POs in the DFS order through the best cuts
    vNodes = Fpga_MappingDfsCuts( pMan );
    for ( i = 0; i < vNodes->nSize; i++ )
    {
        pNode = vNodes->pArray[i];
        pNode->pCutBest->aFlow = Fpga_CutGetAreaRefed( pMan, pNode->pCutBest );
        Area += pMan->pLutLib->pLutAreas[pNode->pCutBest->nLeaves];
    }
    Fpga_NodeVecFree( vNodes );
    return Area;
}

/**Function*************************************************************

  Synopsis    [Sets the correct reference counts for the mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Fpga_MappingSetRefsAndArea( Fpga_Man_t * pMan )
{
    Fpga_Node_t * pNode;
    float aArea;
    int i;
    // clean all references
    for ( i = 0; i < pMan->vNodesAll->nSize; i++ )
        pMan->vNodesAll->pArray[i]->nRefs = 0;
    // collect nodes reachable from POs in the DFS order through the best cuts
    aArea = 0;
    for ( i = 0; i < pMan->nOutputs; i++ )
    {
        pNode = Fpga_Regular(pMan->pOutputs[i]);
        if ( pNode == pMan->pConst1 )
            continue;
        aArea += Fpga_MappingSetRefsAndArea_rec( pMan, pNode );
        pNode->nRefs++;
    }
    return aArea;
}

/**Function*************************************************************

  Synopsis    [Recursively computes the DFS ordering of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Fpga_MappingSetRefsAndArea_rec( Fpga_Man_t * pMan, Fpga_Node_t * pNode )
{
    float aArea;
    int i;
    assert( !Fpga_IsComplement(pNode) );
    if ( pNode->nRefs++ )
        return 0;
    if ( !Fpga_NodeIsAnd(pNode) )
        return 0;
    assert( pNode->pCutBest != NULL );
    // visit the transitive fanin of the selected cut
    aArea = pMan->pLutLib->pLutAreas[pNode->pCutBest->nLeaves];
    for ( i = 0; i < pNode->pCutBest->nLeaves; i++ )
        aArea += Fpga_MappingSetRefsAndArea_rec( pMan, pNode->pCutBest->ppLeaves[i] );
    return aArea;
}

/**Function*************************************************************

  Synopsis    [Computes the number of logic levels not counting PIs/POs.]

  Description []
               
  SideEffects [Note that this procedure will reassign the levels assigned
  originally by NodeCreate() because it counts the number of levels with 
  choices differently!]

  SeeAlso     []

***********************************************************************/
int Fpga_MappingCountLevels( Fpga_Man_t * pMan )
{
    int i, LevelsMax, LevelsCur;
    // perform the traversal
    LevelsMax = -1;
    for ( i = 0; i < pMan->nOutputs; i++ )
    {
        LevelsCur = Fpga_MappingCountLevels_rec( Fpga_Regular(pMan->pOutputs[i]) );
        if ( LevelsMax < LevelsCur )
            LevelsMax = LevelsCur;
    }
    for ( i = 0; i < pMan->nOutputs; i++ )
        Fpga_MappingUnmark_rec( Fpga_Regular(pMan->pOutputs[i]) );
    return LevelsMax;
}

/**Function*************************************************************

  Synopsis    [Recursively computes the number of logic levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fpga_MappingCountLevels_rec( Fpga_Node_t * pNode )
{
    int Level1, Level2;
    assert( !Fpga_IsComplement(pNode) );
    if ( !Fpga_NodeIsAnd(pNode) )
    {
        pNode->Level = 0;
        return 0;
    }
    if ( pNode->fMark0 )
        return pNode->Level;
    pNode->fMark0 = 1;
    // visit the transitive fanin
    Level1 = Fpga_MappingCountLevels_rec( Fpga_Regular(pNode->p1) );
    Level2 = Fpga_MappingCountLevels_rec( Fpga_Regular(pNode->p2) );
    // set the number of levels
    pNode->Level = 1 + ((Level1>Level2)? Level1: Level2);
    return pNode->Level;
}

/**Function*************************************************************

  Synopsis    [Unmarks the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_MappingUnmark( Fpga_Man_t * pMan )
{
    int i;
    for ( i = 0; i < pMan->nOutputs; i++ )
        Fpga_MappingUnmark_rec( Fpga_Regular(pMan->pOutputs[i]) );
}

/**Function*************************************************************

  Synopsis    [Recursively unmarks the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_MappingUnmark_rec( Fpga_Node_t * pNode )
{
    assert( !Fpga_IsComplement(pNode) );
    if ( pNode->fMark0 == 0 )
        return;
    pNode->fMark0 = 0;
    if ( !Fpga_NodeIsAnd(pNode) )
        return;
    Fpga_MappingUnmark_rec( Fpga_Regular(pNode->p1) );
    Fpga_MappingUnmark_rec( Fpga_Regular(pNode->p2) );
    // visit the equivalent nodes
    if ( pNode->pNextE )
        Fpga_MappingUnmark_rec( pNode->pNextE );
}

/**Function*************************************************************

  Synopsis    [Recursively unmarks the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_MappingMark_rec( Fpga_Node_t * pNode )
{
    assert( !Fpga_IsComplement(pNode) );
    if ( pNode->fMark0 == 1 )
        return;
    pNode->fMark0 = 1;
    if ( !Fpga_NodeIsAnd(pNode) )
        return;
    Fpga_MappingMark_rec( Fpga_Regular(pNode->p1) );
    Fpga_MappingMark_rec( Fpga_Regular(pNode->p2) );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_MappedMark_rec( Fpga_Node_t * pNode )
{
    int i;
    assert( !Fpga_IsComplement(pNode) );
    if ( pNode->fMark0 == 1 )
        return;
    pNode->fMark0 = 1;
    if ( !Fpga_NodeIsAnd(pNode) )
        return;
    assert( pNode->pCutBest != NULL );
    // visit the transitive fanin of the selected cut
    for ( i = 0; i < pNode->pCutBest->nLeaves; i++ )
        Fpga_MappedMark_rec( pNode->pCutBest->ppLeaves[i] );
}

/**Function*************************************************************

  Synopsis    [Recursively unmarks the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_MappedUnmark_rec( Fpga_Node_t * pNode )
{
    int i;
    assert( !Fpga_IsComplement(pNode) );
    if ( pNode->fMark0 == 0 )
        return;
    pNode->fMark0 = 0;
    if ( !Fpga_NodeIsAnd(pNode) )
        return;
    assert( pNode->pCutBest != NULL );
    // visit the transitive fanin of the selected cut
    for ( i = 0; i < pNode->pCutBest->nLeaves; i++ )
        Fpga_MappedUnmark_rec( pNode->pCutBest->ppLeaves[i] );
}

/**Function*************************************************************

  Synopsis    [Prints a bunch of latest arriving outputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_MappingPrintOutputArrivals( Fpga_Man_t * p )
{
    Fpga_Node_t * pNode;
    int fCompl, Limit, i;
    int * pSorted;

    // sort outputs by arrival time
    s_pMan = p;
    pSorted = ALLOC( int, p->nOutputs );
    for ( i = 0; i < p->nOutputs; i++ )
        pSorted[i] = i;
    qsort( (void *)pSorted, p->nOutputs, sizeof(int), 
            (int (*)(const void *, const void *)) Fpga_MappingCompareOutputDelay );
    assert( Fpga_MappingCompareOutputDelay( pSorted, pSorted + p->nOutputs - 1 ) <= 0 );
    s_pMan = NULL;

    // print the latest outputs
    Limit = (p->nOutputs > 5)? 5 : p->nOutputs;
    for ( i = 0; i < Limit; i++ )
    {
        // get the i-th latest output
        pNode  = Fpga_Regular(p->pOutputs[pSorted[i]]);
        fCompl = Fpga_IsComplement(p->pOutputs[pSorted[i]]);
        // print out the best arrival time
        printf( "Output  %20s : ", p->ppOutputNames[pSorted[i]] );
        printf( "Delay = %8.2f  ",     (double)pNode->pCutBest->tArrival );
        if ( fCompl )
            printf( "NEG" );
        else
            printf( "POS" );
        printf( "\n" );
    }
    free( pSorted );
}

/**Function*************************************************************

  Synopsis    [Compares the outputs by their arrival times.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fpga_MappingCompareOutputDelay( int * pOut1, int * pOut2 )
{
    Fpga_Node_t * pNode1 = Fpga_Regular(s_pMan->pOutputs[*pOut1]);
    Fpga_Node_t * pNode2 = Fpga_Regular(s_pMan->pOutputs[*pOut2]);
    float pTime1 = pNode1->pCutBest? pNode1->pCutBest->tArrival : 0;
    float pTime2 = pNode2->pCutBest? pNode2->pCutBest->tArrival : 0;
    if ( pTime1 > pTime2 )
        return -1;
    if ( pTime1 < pTime2 )
        return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Sets up the truth tables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_MappingSetupTruthTables( unsigned uTruths[][2] )
{
    int m, v;
    // set up the truth tables
    for ( m = 0; m < 32; m++ )
        for ( v = 0; v < 5; v++ )
            if ( m & (1 << v) )
                uTruths[v][0] |= (1 << m);
    // make adjustments for the case of 6 variables
    for ( v = 0; v < 5; v++ )
        uTruths[v][1] = uTruths[v][0];
    uTruths[5][0] = 0;
    uTruths[5][1] = FPGA_FULL;
}

/**Function*************************************************************

  Synopsis    [Sets up the mask.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_MappingSetupMask( unsigned uMask[], int nVarsMax )
{
    if ( nVarsMax == 6 )
        uMask[0] = uMask[1] = FPGA_FULL;
    else
    {
        uMask[0] = FPGA_MASK(1 << nVarsMax);
        uMask[1] = 0;
    }
}

/**Function*************************************************************

  Synopsis    [Verify one useful property.]

  Description [This procedure verifies one useful property. After 
  the FRAIG construction with choice nodes is over, each primary node 
  should have fanins that are primary nodes. The primary nodes is the 
  one that does not have pNode->pRepr set to point to another node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fpga_ManCheckConsistency( Fpga_Man_t * p )
{
    Fpga_Node_t * pNode;
    Fpga_NodeVec_t * pVec;
    int i;
    pVec = Fpga_MappingDfs( p, 0 );
    for ( i = 0; i < pVec->nSize; i++ )
    {
        pNode = pVec->pArray[i];
        if ( Fpga_NodeIsVar(pNode) )
        {
            if ( pNode->pRepr )
                printf( "Primary input %d is a secondary node.\n", pNode->Num );
        }
        else if ( Fpga_NodeIsConst(pNode) )
        {
            if ( pNode->pRepr )
                printf( "Constant 1 %d is a secondary node.\n", pNode->Num );
        }
        else
        {
            if ( pNode->pRepr )
                printf( "Internal node %d is a secondary node.\n", pNode->Num );
            if ( Fpga_Regular(pNode->p1)->pRepr )
                printf( "Internal node %d has first fanin that is a secondary node.\n", pNode->Num );
            if ( Fpga_Regular(pNode->p2)->pRepr )
                printf( "Internal node %d has second fanin that is a secondary node.\n", pNode->Num );
        }
    }
    Fpga_NodeVecFree( pVec );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Compares the supergates by their level.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fpga_CompareNodesByLevelDecreasing( Fpga_Node_t ** ppS1, Fpga_Node_t ** ppS2 )
{
    if ( Fpga_Regular(*ppS1)->Level > Fpga_Regular(*ppS2)->Level )
        return -1;
    if ( Fpga_Regular(*ppS1)->Level < Fpga_Regular(*ppS2)->Level )
        return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Compares the supergates by their level.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fpga_CompareNodesByLevelIncreasing( Fpga_Node_t ** ppS1, Fpga_Node_t ** ppS2 )
{
    if ( Fpga_Regular(*ppS1)->Level < Fpga_Regular(*ppS2)->Level )
        return -1;
    if ( Fpga_Regular(*ppS1)->Level > Fpga_Regular(*ppS2)->Level )
        return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Orders the nodes in the decreasing order of levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_MappingSortByLevel( Fpga_Man_t * pMan, Fpga_NodeVec_t * vNodes, int fIncreasing )
{
    if ( fIncreasing )
        qsort( (void *)vNodes->pArray, vNodes->nSize, sizeof(Fpga_Node_t *), 
                (int (*)(const void *, const void *)) Fpga_CompareNodesByLevelIncreasing );
    else
        qsort( (void *)vNodes->pArray, vNodes->nSize, sizeof(Fpga_Node_t *), 
                (int (*)(const void *, const void *)) Fpga_CompareNodesByLevelDecreasing );
//    assert( Fpga_CompareNodesByLevel( vNodes->pArray, vNodes->pArray + vNodes->nSize - 1 ) <= 0 );
}

/**Function*************************************************************

  Synopsis    [Computes the limited DFS ordering for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_NodeVec_t * Fpga_DfsLim( Fpga_Man_t * pMan, Fpga_Node_t * pNode, int nLevels )
{
    Fpga_NodeVec_t * vNodes;
    int i;
    // perform the traversal
    vNodes = Fpga_NodeVecAlloc( 100 );
    Fpga_DfsLim_rec( pNode, nLevels, vNodes );
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
void Fpga_DfsLim_rec( Fpga_Node_t * pNode, int Level, Fpga_NodeVec_t * vNodes )
{
    assert( !Fpga_IsComplement(pNode) );
    if ( pNode->fMark0 )
        return;
    pNode->fMark0 = 1;
    // visit the transitive fanin
    Level--;
    if ( Level > 0 && Fpga_NodeIsAnd(pNode) )
    {
        Fpga_DfsLim_rec( Fpga_Regular(pNode->p1), Level, vNodes );
        Fpga_DfsLim_rec( Fpga_Regular(pNode->p2), Level, vNodes );
    }
    // add the node to the list
    Fpga_NodeVecPush( vNodes, pNode );
}

/**Function*************************************************************

  Synopsis    [Computes the limited DFS ordering for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_ManCleanData0( Fpga_Man_t * pMan )
{
    int i;
    for ( i = 0; i < pMan->vNodesAll->nSize; i++ )
        pMan->vNodesAll->pArray[i]->pData0 = 0;
}

/**Function*************************************************************

  Synopsis    [Collects the TFO of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_NodeVec_t * Fpga_CollectNodeTfo( Fpga_Man_t * pMan, Fpga_Node_t * pNode )
{
    Fpga_NodeVec_t * vVisited, * vTfo;
    int i;
    // perform the traversal
    vVisited = Fpga_NodeVecAlloc( 100 );
    vTfo     = Fpga_NodeVecAlloc( 100 );
    for ( i = 0; i < pMan->nOutputs; i++ )
        Fpga_CollectNodeTfo_rec( Fpga_Regular(pMan->pOutputs[i]), pNode, vVisited, vTfo );
    for ( i = 0; i < vVisited->nSize; i++ )
        vVisited->pArray[i]->fMark0 = vVisited->pArray[i]->fMark1 = 0;
    Fpga_NodeVecFree( vVisited );
    return vTfo;
}

/**Function*************************************************************

  Synopsis    [Collects the TFO of the node.]

  Description [Returns 1 if the node should be collected.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fpga_CollectNodeTfo_rec( Fpga_Node_t * pNode, Fpga_Node_t * pPivot, Fpga_NodeVec_t * vVisited, Fpga_NodeVec_t * vTfo )
{
    int Ret1, Ret2;
    assert( !Fpga_IsComplement(pNode) );
    // skip visited nodes
    if ( pNode->fMark0 )
        return pNode->fMark1;
    pNode->fMark0 = 1;
    Fpga_NodeVecPush( vVisited, pNode );

    // return the pivot node
    if ( pNode == pPivot )
    {
        pNode->fMark1 = 1;
        return 1;
    }
    if ( pNode->Level < pPivot->Level )
    {
        pNode->fMark1 = 0;
        return 0;
    }
    // visit the transitive fanin
    assert( Fpga_NodeIsAnd(pNode) );
    Ret1 = Fpga_CollectNodeTfo_rec( Fpga_Regular(pNode->p1), pPivot, vVisited, vTfo );
    Ret2 = Fpga_CollectNodeTfo_rec( Fpga_Regular(pNode->p2), pPivot, vVisited, vTfo );
    if ( Ret1 || Ret2 )
    {
        pNode->fMark1 = 1;
        Fpga_NodeVecPush( vTfo, pNode );
    }
    else
        pNode->fMark1 = 0;
    return pNode->fMark1;
}

/**Function*************************************************************

  Synopsis    [Levelizes the nodes accessible from the POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_NodeVec_t * Fpga_MappingLevelize( Fpga_Man_t * pMan, Fpga_NodeVec_t * vNodes )
{
    Fpga_NodeVec_t * vLevels;
    Fpga_Node_t ** ppNodes;
    Fpga_Node_t * pNode;
    int nNodes, nLevelsMax, i;

    // reassign the levels (this may be necessary for networks which choices)
    ppNodes = vNodes->pArray;
    nNodes  = vNodes->nSize;
    for ( i = 0; i < nNodes; i++ )
    {
        pNode = ppNodes[i];
        if ( !Fpga_NodeIsAnd(pNode) )
        {
            pNode->Level = 0;
            continue;
        }
        pNode->Level = 1 + FPGA_MAX( Fpga_Regular(pNode->p1)->Level, Fpga_Regular(pNode->p2)->Level );
    }

    // get the max levels
    nLevelsMax = 0;
    for ( i = 0; i < pMan->nOutputs; i++ )
        nLevelsMax = FPGA_MAX( nLevelsMax, (int)Fpga_Regular(pMan->pOutputs[i])->Level );
    nLevelsMax++;

    // allocate storage for levels
    vLevels = Fpga_NodeVecAlloc( nLevelsMax );
    for ( i = 0; i < nLevelsMax; i++ )
        Fpga_NodeVecPush( vLevels, NULL );

    // go through the nodes and add them to the levels
    for ( i = 0; i < nNodes; i++ )
    {
        pNode = ppNodes[i];
        pNode->pLevel = NULL;
        if ( !Fpga_NodeIsAnd(pNode) )
            continue;
        // attach the node to this level
        pNode->pLevel = Fpga_NodeVecReadEntry( vLevels, pNode->Level );
        Fpga_NodeVecWriteEntry( vLevels, pNode->Level, pNode );
    }
    return vLevels;
}
/**Function*************************************************************

  Synopsis    [Prints the switching activity changes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Fpga_MappingPrintSwitching( Fpga_Man_t * p )
{
    Fpga_Node_t * pNode;
    float SwitchTotal = 0.0;
    int nNodes = 0;
    int i;
    for ( i = 0; i < p->vNodesAll->nSize; i++ )
    {
        // skip primary inputs
        pNode = p->vNodesAll->pArray[i];
//        if ( !Fpga_NodeIsAnd( pNode ) )
//            continue;
        // skip a secondary node
        if ( pNode->pRepr )
            continue;
        // count the switching nodes
        if ( pNode->nRefs > 0 )
        {
            SwitchTotal += pNode->SwitchProb;
            nNodes++;
        }
    }
    if ( p->fVerbose )
    printf( "Total switching = %10.2f. Average switching = %6.4f.\n", SwitchTotal, SwitchTotal/nNodes );
    return SwitchTotal;
}

/**Function*************************************************************

  Synopsis    [Sets up the mask.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fpga_GetMaxLevel( Fpga_Man_t * pMan )
{
    int nLevelMax, i;
    nLevelMax = 0;
    for ( i = 0; i < pMan->nOutputs; i++ )
        nLevelMax = nLevelMax > Fpga_Regular(pMan->pOutputs[i])->Level? 
                nLevelMax : Fpga_Regular(pMan->pOutputs[i])->Level;
    return nLevelMax;
}

/**Function*************************************************************

  Synopsis    [Analyses choice nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fpga_ManUpdateLevel_rec( Fpga_Man_t * pMan, Fpga_Node_t * pNode )
{
    Fpga_Node_t * pTemp;
    int Level1, Level2, LevelE;
    assert( !Fpga_IsComplement(pNode) );
    if ( !Fpga_NodeIsAnd(pNode) )
        return pNode->Level;
    // skip the visited node
    if ( pNode->TravId == pMan->nTravIds )
        return pNode->Level;
    pNode->TravId = pMan->nTravIds;
    // compute levels of the children nodes
    Level1 = Fpga_ManUpdateLevel_rec( pMan, Fpga_Regular(pNode->p1) );
    Level2 = Fpga_ManUpdateLevel_rec( pMan, Fpga_Regular(pNode->p2) );
    pNode->Level = 1 + FPGA_MAX( Level1, Level2 );
    if ( pNode->pNextE )
    {
        LevelE = Fpga_ManUpdateLevel_rec( pMan, pNode->pNextE );
        if ( pNode->Level > LevelE )
            pNode->Level = LevelE;
        // set the level of all equivalent nodes to be the same minimum
        if ( pNode->pRepr == NULL ) // the primary node
            for ( pTemp = pNode->pNextE; pTemp; pTemp = pTemp->pNextE )
                pTemp->Level = pNode->Level;
    }
    return pNode->Level;
}

/**Function*************************************************************

  Synopsis    [Reports statistics on choice nodes.]

  Description [The number of choice nodes is the number of primary nodes,
  which has pNextE set to a pointer. The number of choices is the number
  of entries in the equivalent-node lists of the primary nodes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_ManReportChoices( Fpga_Man_t * pMan )
{
    Fpga_Node_t * pNode, * pTemp;
    int nChoiceNodes, nChoices;
    int i, LevelMax1, LevelMax2;

    // report the number of levels
    LevelMax1 = Fpga_GetMaxLevel( pMan );
    pMan->nTravIds++;
    for ( i = 0; i < pMan->nOutputs; i++ )
        Fpga_ManUpdateLevel_rec( pMan, Fpga_Regular(pMan->pOutputs[i]) );
    LevelMax2 = Fpga_GetMaxLevel( pMan );

    // report statistics about choices
    nChoiceNodes = nChoices = 0;
    for ( i = 0; i < pMan->vAnds->nSize; i++ )
    {
        pNode = pMan->vAnds->pArray[i];
        if ( pNode->pRepr == NULL && pNode->pNextE != NULL )
        { // this is a choice node = the primary node that has equivalent nodes
            nChoiceNodes++;
            for ( pTemp = pNode->pNextE; pTemp; pTemp = pTemp->pNextE )
                nChoices++;
        }
    }
    if ( pMan->fVerbose )
    {
    printf( "Maximum level: Original = %d. Reduced due to choices = %d.\n", LevelMax1, LevelMax2 );
    printf( "Choice stats:  Choice nodes = %d. Total choices = %d.\n", nChoiceNodes, nChoices );
    }
/*
    {
        FILE * pTable;
        pTable = fopen( "stats_choice.txt", "a+" );
        fprintf( pTable, "%s ", pMan->pFileName );
        fprintf( pTable, "%4d ", LevelMax1 );
        fprintf( pTable, "%4d ", pMan->vAnds->nSize - pMan->nInputs );
        fprintf( pTable, "%4d ", LevelMax2 );
        fprintf( pTable, "%7d ", nChoiceNodes );
        fprintf( pTable, "%7d ", nChoices + nChoiceNodes );
        fprintf( pTable, "\n" );
        fclose( pTable );
    }
*/
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



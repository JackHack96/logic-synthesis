/**CFile****************************************************************

  FileName    [mapperCut.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic technology mapping engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: mapperCut.c,v 1.15 2005/07/08 01:01:24 alanmi Exp $]

***********************************************************************/

#include "mapperInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// the largest number of cuts considered
#define  MAP_CUTS_MAX_COMPUTE   200
// the largest number of cuts used
#define  MAP_CUTS_MAX_USE       50

// temporary hash table to store the cuts
typedef struct Map_CutTableStrutct_t Map_CutTable_t;
struct Map_CutTableStrutct_t
{
    Map_Cut_t ** pBins;        // the table used for linear probing
    int          nBins;        // the size of the table
    int *        pCuts;        // the array of cuts currently stored 
    int          nCuts;        // the number of cuts currently stored 
    Map_Cut_t ** pArray;       // the temporary array of cuts
    Map_Cut_t ** pCuts1;       // the temporary array of cuts
    Map_Cut_t ** pCuts2;       // the temporary array of cuts
};

// primes used to compute the hash key
static int s_HashPrimes[10] = { 109, 499, 557, 619, 631, 709, 797, 881, 907, 991 };

static Map_Cut_t *      Map_CutCompute( Map_Man_t * p, Map_CutTable_t * pTable, Map_Node_t * pNode );
static void             Map_CutFilter( Map_Man_t * p, Map_Node_t * pNode );
static Map_Cut_t *      Map_CutMergeLists( Map_Man_t * p, Map_CutTable_t * pTable, Map_Cut_t * pList1, Map_Cut_t * pList2, int fComp1, int fComp2 );
static int              Map_CutMergeTwo( Map_Cut_t * pCut1, Map_Cut_t * pCut2, Map_Node_t * ppNodes[], int nNodesMax );
static Map_Cut_t *      Map_CutUnionLists( Map_Cut_t * pList1, Map_Cut_t * pList2 );
static int              Map_CutBelongsToList( Map_Cut_t * pList, Map_Node_t * ppNodes[], int nNodes );

static void             Map_CutListPrint( Map_Man_t * pMan, Map_Node_t * pRoot );
static void             Map_CutListPrint2( Map_Man_t * pMan, Map_Node_t * pRoot );
static void             Map_CutPrint_( Map_Man_t * pMan, Map_Cut_t * pCut, Map_Node_t * pRoot );

static Map_CutTable_t * Map_CutTableStart( Map_Man_t * pMan );
static void             Map_CutTableStop( Map_CutTable_t * p );
static unsigned         Map_CutTableHash( Map_Node_t * ppNodes[], int nNodes );
static int              Map_CutTableLookup( Map_CutTable_t * p, Map_Node_t * ppNodes[], int nNodes );
static Map_Cut_t *      Map_CutTableConsider( Map_Man_t * pMan, Map_CutTable_t * p, Map_Node_t * ppNodes[], int nNodes );
static void             Map_CutTableRestart( Map_CutTable_t * p );

static Map_Cut_t *      Map_CutSortCuts( Map_Man_t * pMan, Map_CutTable_t * p, Map_Cut_t * pList );
static int              Map_CutList2Array( Map_Cut_t ** pArray, Map_Cut_t * pList );
static Map_Cut_t *      Map_CutArray2List( Map_Cut_t ** pArray, int nCuts );


// iterator through all the cuts of the list
#define Map_ListForEachCut( pList, pCut )                 \
    for ( pCut = pList;                                   \
          pCut;                                           \
          pCut = pCut->pNext )
#define Map_ListForEachCutSafe( pList, pCut, pCut2 )      \
    for ( pCut = pList,                                   \
          pCut2 = pCut? pCut->pNext: NULL;                \
          pCut;                                           \
          pCut = pCut2,                                   \
          pCut2 = pCut? pCut->pNext: NULL )


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes the cuts for each node in the object graph.]

  Description [The cuts are computed in one sweep over the mapping graph. 
  First, the elementary cuts, which include the node itself, are assigned 
  to the PI nodes. The internal nodes are considered in the DFS order.
  Each node is two-input AND-gate. So to compute the cuts at a node, we
  need to merge the sets of cuts of its two predecessors. The merged set
  contains only unique cuts with the number of inputs equal to k or less.
  Finally, the elementary cut, composed of the node itself, is added to
  the set of cuts for the node.
  
  This procedure is pretty fast for 5-feasible cuts, but it dramatically
  slows down on some "dense" networks when computing 6-feasible cuts.
  The problem is that there are too many cuts in this case. We should
  think how to heuristically trim the number of cuts in such cases, 
  to have reasonable runtime.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_MappingCuts( Map_Man_t * p )
{
    ProgressBar * pProgress;
    Map_CutTable_t * pTable;
    Map_Node_t * pNode;
    Map_Cut_t * pCut;
    int nCuts, nNodes, i;
    int TimePrint;

    // set the elementary cuts for the PI variables
    assert( p->nVarsMax > 1 && p->nVarsMax < 7 );
    for ( i = 0; i < p->nInputs; i++ )
    {
        pCut = Map_CutAlloc( p );
        pCut->nLeaves = 1;
        pCut->ppLeaves[0] = p->pInputs[i];
//        pCut->fLevel = (float)pCut->ppLeaves[0]->Level;
        p->pInputs[i]->pCuts   = pCut;
        p->pInputs[i]->pCutBest[0] = NULL; // negative polarity is not mapped
        p->pInputs[i]->pCutBest[1] = pCut; // positive polarity is a trivial cut
        pCut->M[0].AreaFlow = 0.0;
        pCut->M[1].AreaFlow = 0.0;
    }

    // compute the cuts for the internal nodes
    nNodes = p->vAnds->nSize;
    pProgress = Extra_ProgressBarStart( stdout, nNodes );
    pTable = Map_CutTableStart( p );
    TimePrint = clock() + CLOCKS_PER_SEC;
    for ( i = 0; i < nNodes; i++ )
    {
        pNode = p->vAnds->pArray[i];
        if ( !Map_NodeIsAnd( pNode ) )
            continue;
        Map_CutCompute( p, pTable, pNode );
        if ( i % 10 == 0 && clock() > TimePrint )
        {
            Extra_ProgressBarUpdate( pProgress, i, "Cuts ..." );
            TimePrint = clock() + CLOCKS_PER_SEC;
        }
    }
    Extra_ProgressBarStop( pProgress );
    Map_CutTableStop( pTable );

    // report the stats
    if ( p->fVerbose )
    {
        nCuts = Map_MappingCountAllCuts(p);
        printf( "Nodes = %6d. Total %d-feasible cuts = %d. Cuts per node = %.1f.\n", 
               p->nNodes, p->nVarsMax, nCuts, ((float)nCuts)/p->nNodes );
    }

    // print the cuts for the first primary output
//    Map_CutListPrint( p, Map_Regular(p->pOutputs[0]) );
}

/**Function*************************************************************

  Synopsis    [Performs technology mapping for variable-size-LUTs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_MappingCreatePiCuts( Map_Man_t * p )
{
    Map_Cut_t * pCut;
    int i;

    // set the elementary cuts for the PI variables
    for ( i = 0; i < p->nInputs; i++ )
    {
        pCut = Map_CutAlloc( p );
        pCut->nLeaves = 1;
        pCut->ppLeaves[0] = p->pInputs[i];
//        pCut->fLevel = (float)pCut->ppLeaves[0]->Level;
        p->pInputs[i]->pCuts   = pCut;
        p->pInputs[i]->pCutBest[1] = pCut;
        p->pInputs[i]->pCutBest[0] = pCut;
        // set the input arrival times
//        p->pInputs[i]->pCut[1]->tArrival = p->pInputArrivals[i];

        // set the input arrival times
        pCut = p->pInputs[i]->pCutBest[1];
        pCut->M[1].tArrive = p->pInputArrivals[i];
        pCut->M[1].tArrive.Worst = MAP_MAX( pCut->M[1].tArrive.Rise, pCut->M[1].tArrive.Fall );
        // set the arrival times of the negative phases of the PI nodes
        pCut = p->pInputs[i]->pCutBest[0];
        pCut->M[0].tArrive.Rise = p->pInputArrivals[i].Fall + p->pSuperLib->tDelayInv.Rise;
        pCut->M[0].tArrive.Fall = p->pInputArrivals[i].Rise + p->pSuperLib->tDelayInv.Fall;
        pCut->M[0].tArrive.Worst = MAP_MAX( pCut->M[0].tArrive.Rise, pCut->M[0].tArrive.Fall );

    }
}

/**Function*************************************************************

  Synopsis    [Computes the cuts for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Map_Cut_t * Map_CutCompute( Map_Man_t * p, Map_CutTable_t * pTable, Map_Node_t * pNode )
{
    Map_Node_t * pTemp;
    Map_Cut_t * pList, * pList1, * pList2;
    Map_Cut_t * pCut;

    // if the cuts are computed return them
    if ( pNode->pCuts )
        return pNode->pCuts;

    // compute the cuts for the children
    pList1 = Map_Regular(pNode->p1)->pCuts;
    pList2 = Map_Regular(pNode->p2)->pCuts;
    // merge the lists
    pList = Map_CutMergeLists( p, pTable, pList1, pList2, 
        Map_IsComplement(pNode->p1), Map_IsComplement(pNode->p2) );
    // if there are functionally equivalent nodes, union them with this list
    assert( pList );
    // only add to the list of cuts if the node is a representative one
    if ( pNode->pRepr == NULL )
    {
        for ( pTemp = pNode->pNextE; pTemp; pTemp = pTemp->pNextE )
        {
            assert( pTemp->pCuts );
            pList = Map_CutUnionLists( pList, pTemp->pCuts );
            assert( pTemp->pCuts );
            pList = Map_CutSortCuts( p, pTable, pList );
        }
    }
    // add the new cut
    pCut = Map_CutAlloc( p );
    pCut->nLeaves = 1;
    pCut->ppLeaves[0] = pNode;
//    pCut->fLevel = (float)pCut->ppLeaves[0]->Level;
    // append (it is important that the elementary cut is appended first)
    pCut->pNext = pList;
    // set at the node
    pNode->pCuts = pCut;
    // remove the dominated cuts
//    Map_CutFilter( p, pNode );
    // set the phase correctly
    if ( pNode->pRepr && Map_NodeComparePhase(pNode, pNode->pRepr) )
    {
        Map_ListForEachCut( pNode->pCuts, pCut )
            pCut->Phase = 1;
    }
/*
    { 
        int i, Counter = 0;;
        for ( pCut = pNode->pCuts->pNext; pCut; pCut = pCut->pNext )
            for ( i = 0; i < pCut->nLeaves; i++ )
                Counter += (pCut->ppLeaves[i]->Level >= pNode->Level);
//        if ( Counter )
//            printf( " %d", Counter );
    }
*/
    return pCut;
}

/**Function*************************************************************

  Synopsis    [Filter the cuts using dominance.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_CutFilter( Map_Man_t * p, Map_Node_t * pNode )
{ 
    Map_Cut_t * pTemp, * pPrev, * pCut, * pCut2;
    int i, k, Counter;

    Counter = 0;
    pPrev = pNode->pCuts;
    Map_ListForEachCutSafe( pNode->pCuts->pNext, pCut, pCut2 )
    {
        // go through all the previous cuts up to pCut
        for ( pTemp = pNode->pCuts->pNext; pTemp != pCut; pTemp = pTemp->pNext )
        {
            // check if every node in pTemp is contained in pCut
            for ( i = 0; i < pTemp->nLeaves; i++ )
            {
                for ( k = 0; k < pCut->nLeaves; k++ )
                    if ( pTemp->ppLeaves[i] == pCut->ppLeaves[k] )
                        break;
                if ( k == pCut->nLeaves ) // node i in pTemp is not contained in pCut
                    break;
            }
            if ( i == pTemp->nLeaves ) // every node in pTemp is contained in pCut
            {
                Counter++;
                break;
            }
        }
        if ( pTemp != pCut ) // pTemp contain pCut
        {
            pPrev->pNext = pCut->pNext;  // skip pCut
            // recycle pCut
            Map_CutFree( p, pCut );
        }
        else 
            pPrev = pCut; 
    }
//  printf( "Dominated = %3d. \n", Counter );
}

/**Function*************************************************************

  Synopsis    [Merges two lists of cuts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Map_Cut_t * Map_CutMergeLists( Map_Man_t * p, Map_CutTable_t * pTable, 
    Map_Cut_t * pList1, Map_Cut_t * pList2, int fComp1, int fComp2 )
{
    Map_Node_t * ppNodes[6];
    Map_Cut_t * pListNew, ** ppListNew, * pLists[7] = { NULL };
    Map_Cut_t * pCut, * pPrev, * pTemp1, * pTemp2;
    int nNodes, Counter, i;
    Map_Cut_t ** ppArray1, ** ppArray2, ** ppArray3;
    int nCuts1, nCuts2, nCuts3, k, fComp3;

    ppArray1 = pTable->pCuts1;
    ppArray2 = pTable->pCuts2;
    nCuts1 = Map_CutList2Array( ppArray1, pList1 );
    nCuts2 = Map_CutList2Array( ppArray2, pList2 );
    // swap the lists based on their length
    if ( nCuts1 > nCuts2 )
    {
         ppArray3 = ppArray1;
         ppArray1 = ppArray2;
         ppArray2 = ppArray3;

         nCuts3 = nCuts1;
         nCuts1 = nCuts2;
         nCuts2 = nCuts3;

         fComp3 = fComp1;
         fComp1 = fComp2;
         fComp2 = fComp3;
    }
    // pList1 is shorter or equal length compared to pList2
 
    // prepare the manager for the cut computation
    Map_CutTableRestart( pTable );
    // go through the cut pairs
    Counter = 0;
//    for ( pTemp1 = pList1; pTemp1; pTemp1 = fPivot1? NULL: pTemp1->pNext )
//        for ( pTemp2 = pList2; pTemp2; pTemp2 = fPivot2? NULL: pTemp2->pNext )
    for ( i = 0; i < nCuts1; i++ )
    {
        for ( k = 0; k <= i; k++ )
        {
            pTemp1 = ppArray1[i];
            pTemp2 = ppArray2[k];

            // check if k-feasible cut exists
            nNodes = Map_CutMergeTwo( pTemp1, pTemp2, ppNodes, p->nVarsMax );
            if ( nNodes == 0 )
                continue;
            // consider the cut for possible addition to the set of new cuts
            pCut = Map_CutTableConsider( p, pTable, ppNodes, nNodes );
            if ( pCut == NULL )
                continue;
            // add data to the cut
            pCut->pOne = Map_CutNotCond( pTemp1, fComp1 );
            pCut->pTwo = Map_CutNotCond( pTemp2, fComp2 );
            // add it to the corresponding list
            pCut->pNext = pLists[pCut->nLeaves];
            pLists[pCut->nLeaves] = pCut;
            // count this cut and quit if limit is reached
            Counter++;
            if ( Counter == MAP_CUTS_MAX_COMPUTE )
                goto QUITS;
        }
        for ( k = 0; k < i; k++ )
        {
            pTemp1 = ppArray1[k];
            pTemp2 = ppArray2[i];

            // check if k-feasible cut exists
            nNodes = Map_CutMergeTwo( pTemp1, pTemp2, ppNodes, p->nVarsMax );
            if ( nNodes == 0 )
                continue;
            // consider the cut for possible addition to the set of new cuts
            pCut = Map_CutTableConsider( p, pTable, ppNodes, nNodes );
            if ( pCut == NULL )
                continue;
            // add data to the cut
            pCut->pOne = Map_CutNotCond( pTemp1, fComp1 );
            pCut->pTwo = Map_CutNotCond( pTemp2, fComp2 );
            // add it to the corresponding list
            pCut->pNext = pLists[pCut->nLeaves];
            pLists[pCut->nLeaves] = pCut;
            // count this cut and quit if limit is reached
            Counter++;
            if ( Counter == MAP_CUTS_MAX_COMPUTE )
                goto QUITS;
        }
    }
    // consider the rest of them
    for ( i = nCuts1; i < nCuts2; i++ )
        for ( k = 0; k < nCuts1; k++ )
        {
            pTemp1 = ppArray1[k];
            pTemp2 = ppArray2[i];

            // check if k-feasible cut exists
            nNodes = Map_CutMergeTwo( pTemp1, pTemp2, ppNodes, p->nVarsMax );
            if ( nNodes == 0 )
                continue;
            // consider the cut for possible addition to the set of new cuts
            pCut = Map_CutTableConsider( p, pTable, ppNodes, nNodes );
            if ( pCut == NULL )
                continue;
            // add data to the cut
            pCut->pOne = Map_CutNotCond( pTemp1, fComp1 );
            pCut->pTwo = Map_CutNotCond( pTemp2, fComp2 );
            // add it to the corresponding list
            pCut->pNext = pLists[pCut->nLeaves];
            pLists[pCut->nLeaves] = pCut;
            // count this cut and quit if limit is reached
            Counter++;
            if ( Counter == MAP_CUTS_MAX_COMPUTE )
                goto QUITS;
        }
QUITS :
    // combine all the lists into one
    pListNew  = NULL;
    ppListNew = &pListNew;
    for ( i = 1; i <= p->nVarsMax; i++ )
    {
        if ( pLists[i] == NULL )
            continue;
        // find the last entry
        for ( pPrev = pLists[i], pCut = pPrev->pNext; pCut; 
            pPrev = pCut, pCut = pCut->pNext );
        // connect these lists
        *ppListNew = pLists[i];
        ppListNew  = &pPrev->pNext;
    }
    *ppListNew = NULL;
    // soft the cuts by arrival times and use only the first MAP_CUTS_MAX_USE
    pListNew = Map_CutSortCuts( p, pTable, pListNew );
    return pListNew;
}


/**Function*************************************************************

  Synopsis    [Merges two lists of cuts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Map_Cut_t * Map_CutMergeLists2( Map_Man_t * p, Map_CutTable_t * pTable, 
    Map_Cut_t * pList1, Map_Cut_t * pList2, int fComp1, int fComp2 )
{
    Map_Node_t * ppNodes[6];
    Map_Cut_t * pListNew, ** ppListNew, * pLists[7] = { NULL };
    Map_Cut_t * pCut, * pPrev, * pTemp1, * pTemp2;
    int nNodes, Counter, i;

    // prepare the manager for the cut computation
    Map_CutTableRestart( pTable );
    // go through the cut pairs
    Counter = 0;
    for ( pTemp1 = pList1; pTemp1; pTemp1 = pTemp1->pNext )
        for ( pTemp2 = pList2; pTemp2; pTemp2 = pTemp2->pNext )
        {
            // check if k-feasible cut exists
            nNodes = Map_CutMergeTwo( pTemp1, pTemp2, ppNodes, p->nVarsMax );
            if ( nNodes == 0 )
                continue;
            // consider the cut for possible addition to the set of new cuts
            pCut = Map_CutTableConsider( p, pTable, ppNodes, nNodes );
            if ( pCut == NULL )
                continue;
            // add data to the cut
            pCut->pOne = Map_CutNotCond( pTemp1, fComp1 );
            pCut->pTwo = Map_CutNotCond( pTemp2, fComp2 );
            // add it to the corresponding list
            pCut->pNext = pLists[pCut->nLeaves];
            pLists[pCut->nLeaves] = pCut;
            // count this cut and quit if limit is reached
            Counter++;
            if ( Counter == MAP_CUTS_MAX_COMPUTE )
                goto QUITS;
        }
QUITS :
    // combine all the lists into one
    pListNew  = NULL;
    ppListNew = &pListNew;
    for ( i = 1; i <= p->nVarsMax; i++ )
    {
        if ( pLists[i] == NULL )
            continue;
        // find the last entry
        for ( pPrev = pLists[i], pCut = pPrev->pNext; pCut; 
            pPrev = pCut, pCut = pCut->pNext );
        // connect these lists
        *ppListNew = pLists[i];
        ppListNew  = &pPrev->pNext;
    }
    *ppListNew = NULL;
    // soft the cuts by arrival times and use only the first MAP_CUTS_MAX_USE
    pListNew = Map_CutSortCuts( p, pTable, pListNew );
    return pListNew;
}

/**Function*************************************************************

  Synopsis    [Merges two cuts.]

  Description [Returns the number of nodes in the resulting cut, or 0 if the
  cut is infeasible. Returns the resulting nodes in the array ppNodes[].]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_CutMergeTwo( Map_Cut_t * pCut1, Map_Cut_t * pCut2, Map_Node_t * ppNodes[], int nNodesMax )
{
    Map_Node_t * pNodeTemp;
    int nTotal, i, k, min, fMismatch;

    // check the special case when at least of the cuts is the largest
    if ( pCut1->nLeaves == nNodesMax )
    {
        if ( pCut2->nLeaves == nNodesMax )
        {
            // return 0 if the cuts are different
            for ( i = 0; i < nNodesMax; i++ )
                if ( pCut1->ppLeaves[i] != pCut2->ppLeaves[i] )
                    return 0;
            // return nNodesMax if they are the same
            for ( i = 0; i < nNodesMax; i++ )
                ppNodes[i] = pCut1->ppLeaves[i];
            return nNodesMax;
        }
        else if ( pCut2->nLeaves == nNodesMax - 1 ) 
        {
            // return 0 if the cuts are different
            fMismatch = 0;
            for ( i = 0; i < nNodesMax; i++ )
                if ( pCut1->ppLeaves[i] != pCut2->ppLeaves[i - fMismatch] )
                {
                    if ( fMismatch == 1 )
                        return 0;
                    fMismatch = 1;
                }
            // return nNodesMax if they are the same
            for ( i = 0; i < nNodesMax; i++ )
                ppNodes[i] = pCut1->ppLeaves[i];
            return nNodesMax;
        }
    }
    else if ( pCut1->nLeaves == nNodesMax - 1 && pCut2->nLeaves == nNodesMax )
    {
        // return 0 if the cuts are different
        fMismatch = 0;
        for ( i = 0; i < nNodesMax; i++ )
            if ( pCut1->ppLeaves[i - fMismatch] != pCut2->ppLeaves[i] )
            {
                if ( fMismatch == 1 )
                    return 0;
                fMismatch = 1;
            }
        // return nNodesMax if they are the same
        for ( i = 0; i < nNodesMax; i++ )
            ppNodes[i] = pCut2->ppLeaves[i];
        return nNodesMax;
    }

    // count the number of unique entries in pCut2
    nTotal = pCut1->nLeaves;
    for ( i = 0; i < pCut2->nLeaves; i++ )
    {
        // try to find this entry among the leaves of pCut1
        for ( k = 0; k < pCut1->nLeaves; k++ )
            if ( pCut2->ppLeaves[i] == pCut1->ppLeaves[k] )
                break;
        if ( k < pCut1->nLeaves ) // found
            continue;
        // we found a new entry to add
        if ( nTotal == nNodesMax )
            return 0;
        ppNodes[nTotal++] = pCut2->ppLeaves[i];
    }
    // we know that the feasible cut exists

    // add the starting entries
    for ( k = 0; k < pCut1->nLeaves; k++ )
        ppNodes[k] = pCut1->ppLeaves[k];

    // selection-sort the entries
    for ( i = 0; i < nTotal - 1; i++ )
    {
        min = i;
        for ( k = i+1; k < nTotal; k++ )
            if ( ppNodes[k] < ppNodes[min] )
                min = k;
        pNodeTemp    = ppNodes[i];
        ppNodes[i]   = ppNodes[min];
        ppNodes[min] = pNodeTemp;
    }

    return nTotal;
}

/**Function*************************************************************

  Synopsis    [Computes the union of the two lists of cuts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Map_Cut_t * Map_CutUnionLists( Map_Cut_t * pList1, Map_Cut_t * pList2 )
{
    Map_Cut_t * pTemp, * pRoot;
    // find the last cut in the first list
    pRoot = pList1;
    Map_ListForEachCut( pList1, pTemp )
        pRoot = pTemp;
    // attach the non-trival part of the second cut to the end of the first
    assert( pRoot->pNext == NULL );
    pRoot->pNext = pList2->pNext;   
    pList2->pNext = NULL;
    return pList1;
}


/**Function*************************************************************

  Synopsis    [Checks whether the given cut belongs to the list.]

  Description [This procedure takes most of the runtime in the cut 
  computation.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_CutBelongsToList( Map_Cut_t * pList, Map_Node_t * ppNodes[], int nNodes )
{
    Map_Cut_t * pTemp;
    int i;
    for ( pTemp = pList; pTemp; pTemp = pTemp->pNext )
    {
        for ( i = 0; i < nNodes; i++ )
            if ( pTemp->ppLeaves[i] != ppNodes[i] )
                break;
        if ( i == nNodes )
            return 1;
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Allocates the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
/*
Map_Cut_t * Map_CutAlloc( Map_Man_t * p )
{
    Map_Cut_t * pCut;
    pCut = (Map_Cut_t *)Extra_MmFixedEntryFetch( p->mmCuts );
    memset( pCut, 0, sizeof(Map_Cut_t) );
    return pCut;
}
*/

/**Function*************************************************************

  Synopsis    [Counts all the cuts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_MappingCountAllCuts( Map_Man_t * pMan )
{
    Map_Node_t * pNode;
    Map_Cut_t * pCut;
    int i, nCuts;
    // go through all the nodes in the unique table of the manager
    nCuts = 0;
    for ( i = 0; i < pMan->nBins; i++ )
        for ( pNode = pMan->pBins[i]; pNode; pNode = pNode->pNext )
            for ( pCut = pNode->pCuts; pCut; pCut = pCut->pNext )
                if ( pCut->nLeaves > 1 ) // skip the elementary cuts
                    nCuts++;
    return nCuts;
}





/**Function*************************************************************

  Synopsis    [Prints the cuts in the list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_CutListPrint( Map_Man_t * pMan, Map_Node_t * pRoot )
{
    Map_Cut_t * pTemp;
    int Counter;
    for ( Counter = 0, pTemp = pRoot->pCuts; pTemp; pTemp = pTemp->pNext, Counter++ )
    {
        printf( "%2d : ", Counter + 1 );
        Map_CutPrint_( pMan, pTemp, pRoot );
    }
}

/**Function*************************************************************

  Synopsis    [Prints the cuts in the list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_CutListPrint2( Map_Man_t * pMan, Map_Node_t * pRoot )
{
    Map_Cut_t * pTemp;
    int Counter;
    for ( Counter = 0, pTemp = pRoot->pCuts; pTemp; pTemp = pTemp->pNext, Counter++ )
    {
        printf( "%2d : ", Counter + 1 );
        Map_CutPrint_( pMan, pTemp, pRoot );
    }
}

/**Function*************************************************************

  Synopsis    [Prints the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_CutPrint_( Map_Man_t * pMan, Map_Cut_t * pCut, Map_Node_t * pRoot )
{
    int i;
    printf( "(%3d)  {", pRoot->Num );
    for ( i = 0; i < pMan->nVarsMax; i++ )
        if ( pCut->ppLeaves[i] )
            printf( " %3d", pCut->ppLeaves[i]->Num );
    printf( " }\n" );
}








/**Function*************************************************************

  Synopsis    [Starts the hash table to canonicize cuts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Map_CutTable_t * Map_CutTableStart( Map_Man_t * pMan )
{
    Map_CutTable_t * p;
    // allocate the table
    p = ALLOC( Map_CutTable_t, 1 );
    memset( p, 0, sizeof(Map_CutTable_t) );
    p->nBins = Cudd_Prime( 10 * MAP_CUTS_MAX_COMPUTE );
    p->pBins = ALLOC( Map_Cut_t *, p->nBins );
    memset( p->pBins, 0, sizeof(Map_Cut_t *) * p->nBins );
    p->pCuts = ALLOC( int, 2 * MAP_CUTS_MAX_COMPUTE );
    p->pArray = ALLOC( Map_Cut_t *, 2 * MAP_CUTS_MAX_COMPUTE );
    p->pCuts1 = ALLOC( Map_Cut_t *, 2 * MAP_CUTS_MAX_COMPUTE );
    p->pCuts2 = ALLOC( Map_Cut_t *, 2 * MAP_CUTS_MAX_COMPUTE );
    return p;
}

/**Function*************************************************************

  Synopsis    [Stops the hash table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_CutTableStop( Map_CutTable_t * p )
{
    free( p->pCuts1 );
    free( p->pCuts2 );
    free( p->pArray );
    free( p->pBins );
    free( p->pCuts );
    free( p );
}

/**Function*************************************************************

  Synopsis    [Computes the hash value of the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Map_CutTableHash( Map_Node_t * ppNodes[], int nNodes )
{
    unsigned uRes;
    int i;
    uRes = 0;
    for ( i = 0; i < nNodes; i++ )
        uRes += s_HashPrimes[i] * ppNodes[i]->Num;
    return uRes;
}

/**Function*************************************************************

  Synopsis    [Looks up the table for the available cut.]

  Description [Returns -1 if the same cut is found. Returns the index
  of the cell where the cut should be added, if it does not exist.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_CutTableLookup( Map_CutTable_t * p, Map_Node_t * ppNodes[], int nNodes )
{
    Map_Cut_t * pCut;
    unsigned Key;
    int b, i;

    Key = Map_CutTableHash(ppNodes, nNodes) % p->nBins; 
    for ( b = Key; p->pBins[b]; b = (b+1) % p->nBins )
    {
        pCut = p->pBins[b];
        if ( pCut->nLeaves != nNodes )
            continue;
        for ( i = 0; i < nNodes; i++ )
            if ( pCut->ppLeaves[i] != ppNodes[i] )
                break;
        if ( i == nNodes )
            return -1;
    }
    return b;
}


/**Function*************************************************************

  Synopsis    [Starts the hash table to canonicize cuts.]

  Description [Considers addition of the cut to the hash table.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Map_Cut_t * Map_CutTableConsider( Map_Man_t * pMan, Map_CutTable_t * p, Map_Node_t * ppNodes[], int nNodes )
{
    Map_Cut_t * pCut;
    int Place, i;
    // check the cut
    Place = Map_CutTableLookup( p, ppNodes, nNodes );
    if ( Place == -1 )
        return NULL;
    assert( nNodes > 0 );
    // create the new cut
    pCut = Map_CutAlloc( pMan );
    pCut->nLeaves = nNodes;
//    pCut->fLevel = 0;
    for ( i = 0; i < nNodes; i++ )
    {
        pCut->ppLeaves[i] = ppNodes[i];
//        pCut->fLevel += ppNodes[i]->Level;
    }
//    pCut->fLevel /= nNodes;
    // add the cut to the table
    assert( p->pBins[Place] == NULL );
    p->pBins[Place] = pCut;
    // add the cut to the new list
    p->pCuts[ p->nCuts++ ] = Place;
    return pCut;
}

/**Function*************************************************************

  Synopsis    [Prepares the table to be used with other cuts.]

  Description [Restarts the table by cleaning the info about cuts stored
  when the previous node was considered.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_CutTableRestart( Map_CutTable_t * p )
{
    int i;
    for ( i = 0; i < p->nCuts; i++ )
    {
        assert( p->pBins[ p->pCuts[i] ] );
        p->pBins[ p->pCuts[i] ] = NULL;
    }
    p->nCuts = 0;
}



/**Function*************************************************************

  Synopsis    [Compares the cuts by the number of leaves and then by delay.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_CutSortCutsCompare( Map_Cut_t ** pC1, Map_Cut_t ** pC2 )
{
    if ( (*pC1)->nLeaves < (*pC2)->nLeaves )
        return -1;
    if ( (*pC1)->nLeaves > (*pC2)->nLeaves )
        return 1;
/*
    if ( (*pC1)->fLevel < (*pC2)->fLevel )
        return -1;
    if ( (*pC1)->fLevel > (*pC2)->fLevel )
        return 1;
*/
    return 0;
}

/**Function*************************************************************

  Synopsis    [Sorts the cuts by average arrival time.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Map_Cut_t * Map_CutSortCuts( Map_Man_t * pMan, Map_CutTable_t * p, Map_Cut_t * pList )
{
    Map_Cut_t * pListNew;
    int nCuts, i;
    // move the cuts from the list into the array
    nCuts = Map_CutList2Array( p->pCuts1, pList );
    assert( nCuts <= MAP_CUTS_MAX_COMPUTE );
    // sort the cuts
    qsort( (void *)p->pCuts1, nCuts, sizeof(Map_Cut_t *), 
            (int (*)(const void *, const void *)) Map_CutSortCutsCompare );
    // move them back into the list
    if ( nCuts > MAP_CUTS_MAX_USE - 1 )
    {
        // free the remaining cuts
        for ( i = MAP_CUTS_MAX_USE - 1; i < nCuts; i++ )
            Extra_MmFixedEntryRecycle( pMan->mmCuts, (char *)p->pCuts1[i] );
        // update the number of cuts
        nCuts = MAP_CUTS_MAX_USE - 1;
    }
    pListNew = Map_CutArray2List( p->pCuts1, nCuts );
    return pListNew;
}

/**Function*************************************************************

  Synopsis    [Moves the nodes from the list into the array.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_CutList2Array( Map_Cut_t ** pArray, Map_Cut_t * pList )
{
    int i;
    for ( i = 0; pList; pList = pList->pNext, i++ )
        pArray[i] = pList;
    return i;
}

/**Function*************************************************************

  Synopsis    [Moves the nodes from the array into the list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Map_Cut_t * Map_CutArray2List( Map_Cut_t ** pArray, int nCuts )
{
    Map_Cut_t * pListNew, ** ppListNew;
    int i;
    pListNew  = NULL;
    ppListNew = &pListNew;
    for ( i = 0; i < nCuts; i++ )
    {
        // connect these lists
        *ppListNew = pArray[i];
        ppListNew  = &pArray[i]->pNext;
//printf( " %d(%.2f)", pArray[i]->nLeaves, pArray[i]->fLevel );
    }
//printf( "\n" );

    *ppListNew = NULL;
    return pListNew;
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


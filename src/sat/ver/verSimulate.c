/**CFile****************************************************************

  FileName    [verSimulate.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [The simulation part of the verification engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: verSimulate.c,v 1.3 2004/07/13 18:42:02 satrajit Exp $]

***********************************************************************/

#include "verInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Ver_VerificationSimulateRound( Ver_Manager_t * p );
static int Ver_VerificationSimulateRoundFirst( Ver_Manager_t * p );
static int Ver_VerificationSimulatePropagate( Ver_Manager_t * p );
extern void Ver_VerificationSortClasses( Ver_Manager_t * p );
static int Ver_VerifyCompareData( Sh_Node_t ** ppC1, Sh_Node_t ** ppC2 );
static int Ver_VerifyCompareRefs( Sh_Node_t ** ppC1, Sh_Node_t ** ppC2 );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs simulation of the strashed network.]

  Description [This procedure simulates the network until it performs
  the given number of rounds of simulation (nRounds) without refining
  the equivalence classes, or until it finds the error. In the former
  case, it returns 1. In the latter, case it returns -1.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ver_VerificationSimulate( Ver_Manager_t * p, int nRounds )
{
    int fChanges, RetValue, i, seed;

    seed = time(NULL);
    seed = 1089742958;
    fprintf (stderr, "Simulation: Using seed %d (for tracking Heisenbug)\n", seed);
    srand( seed );

    // perform the first round of simulation using the hash table
    if ( Ver_VerificationSimulateRoundFirst( p ) == -1 )
        return -1;

    // count the number of care minterms found
    do {
        fChanges = 0;
        for ( i = 0; i < nRounds; i++ )
        {
            RetValue = Ver_VerificationSimulateRound( p );
            if ( RetValue == -1 )
                return -1;
            fChanges += RetValue;
        }
//        printf( "Simulating %d\n", fChanges );
    } while ( fChanges > 0 );


    // find the largest number in each class
    Ver_VerificationSortClasses( p );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ver_VerificationSimulateRoundFirst( Ver_Manager_t * p )
{
    Sh_Node_t ** ppNodes, * pNode;
    st_generator * gen;
    int nNodes;
    st_table * tTable;
    unsigned uData;
    Sh_Node_t ** ppSlot;
    int i;
    if ( Ver_VerificationSimulatePropagate( p ) == -1 )
        return -1;
    // start the table
//    tTable = st_init_table(st_numcmp,st_numhash);
    tTable = st_init_table(st_ptrcmp,st_ptrhash);
    if ( p->fMiter )
    {
        // get the info about internal nodes
        ppNodes   = Sh_NetworkReadNodes( p->pShNet );
        nNodes    = Sh_NetworkReadNodeNum( p->pShNet );
        // sort by the value of data
        for ( i = 0; i < nNodes; i++ )
        {
            uData = (unsigned)ppNodes[i]->pData;
            if ( !st_find_or_add( tTable, (char *)uData, (char ***)&ppSlot ) )
                *ppSlot = NULL;
            ppNodes[i]->pOrder = *ppSlot;
            *ppSlot = ppNodes[i];
        }
    }
    else
    { // get info about the PO nodes
        // get the info about internal nodes
        ppNodes   = Sh_NetworkReadOutputs( p->pShNet );
        nNodes    = Sh_NetworkReadOutputNum( p->pShNet );
        // sort by the value of data
        Sh_ManagerIncrementTravId( p->pShMan );
        for ( i = 0; i < nNodes; i++ )
        {
            // make sure this node is encountered for the first ime
            pNode = Sh_Regular(ppNodes[i]);
            if ( Sh_NodeIsTravIdCurrent(p->pShMan, pNode) )
                continue;
            Sh_NodeSetTravIdCurrent( p->pShMan, pNode );

            uData = (unsigned)pNode->pData;
            if ( !st_find_or_add( tTable, (char *)uData, (char ***)&ppSlot ) )
                *ppSlot = NULL;
            pNode->pOrder = *ppSlot;
            *ppSlot = pNode;
        }
    }
    // create classes
    p->nClassesAlloc = nNodes;
    p->ppClasses = ALLOC( Sh_Node_t *, nNodes );
    i = 0;
    st_foreach_item( tTable, gen, (char **)&uData, (char **)&pNode )
    {
        // skip the unique classes
        if ( pNode->pOrder == NULL )
            continue;
        p->ppClasses[i++] = pNode;
    }
//    assert( i == st_count(tTable) );
    p->nClasses = i;
    st_free_table( tTable );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ver_VerificationSimulatePropagate( Ver_Manager_t * p )
{
    Sh_Node_t ** ppInputs, ** ppOutputs, ** ppNodes;
    int nInputs, nOutputs, nNodes;
    unsigned Data, Data1, Data2;
    int i;

    p->nRounds++;

    ppNodes   = Sh_NetworkReadNodes( p->pShNet );
    nNodes    = Sh_NetworkReadNodeNum( p->pShNet );
    ppInputs  = Sh_NetworkReadInputs( p->pShNet );
    nInputs   = Sh_NetworkReadInputNum( p->pShNet );
    ppOutputs = Sh_NetworkReadOutputs( p->pShNet );
    nOutputs  = Sh_NetworkReadOutputNum( p->pShNet );

    // assign random number to the inputs
    for ( i = 0; i < nInputs; i++ )
    {
        Data = rand();
        Data = ((113 * Data) << 24) ^
               ((115 * Data) << 16) ^
               ((117 * Data) <<  8) ^
               ((119 * Data) <<  0);
        ppInputs[i]->pData = Data;
    }
    // simulate all the way through the nodes
    for ( i = 0; i < nNodes; i++ )
    {
        Data1 = Sh_NodeReadDataCompl( ppNodes[i]->pOne );
        Data2 = Sh_NodeReadDataCompl( ppNodes[i]->pTwo );
        ppNodes[i]->pData = Data1 & Data2;
    }
    // get the final data
    if ( p->fMiter )
    {
        Data = Sh_NodeReadDataCompl( ppOutputs[0] );
        if ( Data )
            return -1;
    }
    // collapse polarity
    for ( i = 0; i < nNodes; i++ )
    {
        if ( ppNodes[i]->pData & 1 )
        {
            ppNodes[i]->fMark = 1;
            ppNodes[i]->pData = ~ppNodes[i]->pData;
        }
        else
            ppNodes[i]->fMark = 0;
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ver_VerificationSimulateRound( Ver_Manager_t * p )
{
    Sh_Node_t * pShNodeCur, * pShNodePrev, * pShNode;
    Sh_Node_t ** pNodeArray;
    int nClassesOld, i, j, k, fChanges, nNodes;

    if ( Ver_VerificationSimulatePropagate( p ) == -1 )
        return -1;

    // break up the equivalence classes
    fChanges = 0;
    nClassesOld = p->nClasses;
    for ( i = 0; i < nClassesOld; i++ )
    {
        if ( p->ppClasses[i]->pOrder == NULL )
            continue;
        if ( p->ppClasses[i]->pOrder->pOrder == NULL && p->ppClasses[i]->pData == p->ppClasses[i]->pOrder->pData )
            continue;
        // put the class into the array
        pNodeArray = p->ppClasses + p->nClasses;
        for ( pShNode = p->ppClasses[i], nNodes = 0; pShNode; pShNode = pShNode->pOrder, nNodes++ )
            pNodeArray[nNodes] = pShNode;
        // sort representatives of this class
        qsort( pNodeArray, nNodes, sizeof(Sh_Node_t *), 
            (int (*)(const void *, const void *)) Ver_VerifyCompareData );
        // link them back into the linked list
        p->ppClasses[i] = NULL;
        for ( j = 0; j < nNodes; j++ )
        {
            pNodeArray[j]->pOrder = p->ppClasses[i];
            p->ppClasses[i] = pNodeArray[j];
        }
        // find the beginning of the second part
        for ( pShNodePrev = NULL, pShNode = p->ppClasses[i];   pShNode; 
              pShNodePrev = pShNode, pShNode = pShNode->pOrder )
                if ( pShNode->pData != p->ppClasses[i]->pData )
                    break;
        if ( pShNode == NULL )
            continue;
        assert( pShNodePrev );
        assert ( pShNode != p->ppClasses[i] );  // To track Heisenbug
        // cut this class
        pShNodePrev->pOrder = NULL;
        pShNodeCur = pShNode;
        // partition the remaining part
        do
        {
            for ( pShNodePrev = NULL, pShNode = pShNodeCur;   pShNode; 
                  pShNodePrev = pShNode, pShNode = pShNode->pOrder )
                    if ( pShNode->pData != pShNodeCur->pData )
                        break;
            pShNodePrev->pOrder = NULL;
            p->ppClasses[ p->nClasses++ ] = pShNodeCur;
            assert ( pShNodeCur != pShNode );  // Debug heisenbug; ensure never add same node to two equiv classes
            // get the beginning of the new class
            pShNodeCur = pShNode;
        }
        while ( pShNodeCur );
        fChanges = 1;
    }
    // compact the classes
    for ( i = 0, k = 0; i < p->nClasses; i++ )
    {
        if ( p->ppClasses[i]->pOrder == NULL )
            continue;
        p->ppClasses[k++] = p->ppClasses[i];
    }
    p->nClasses = k;

    return fChanges;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ver_VerificationSortClasses( Ver_Manager_t * p )
{
    Sh_Node_t * pShNode, * pShNodePrev, * pShNodeMax, * pShNodeMaxPrev;
    int i, k, Counter;

    if ( p->nClasses == 0 )
        return;

    // compact the classes
    for ( i = 0, k = 0; i < p->nClasses; i++ )
    {
        if ( p->ppClasses[i]->pOrder == NULL )
            continue;
        p->ppClasses[k++] = p->ppClasses[i];
    }
    p->nClasses = k;

    // Added by Sat to track down Hiesenbug on Linux
    for ( i = 0; i < p->nClasses; i++ )
    {
        assert ( p->ppClasses[i]->pOrder != NULL );
    }

    // The assertion in above for loop doesn't trigger, but 
    // still the one below triggers! This means that sometimes
    // one ShNode is present in two equivalence classes
    for ( i = 0; i < p->nClasses; i++ )
    {
        assert ( p->ppClasses[i]->pOrder != NULL );
        // find the largest item
        pShNodeMaxPrev = NULL;
        pShNodeMax     = p->ppClasses[i];
        for ( pShNodePrev = NULL,    pShNode = p->ppClasses[i];    pShNode; 
              pShNodePrev = pShNode, pShNode = pShNode->pOrder )
                  if ( pShNodeMax->pData2 < pShNode->pData2 )
                  {
                      pShNodeMax     = pShNode;
                      pShNodeMaxPrev = pShNodePrev;
                  }
        if ( pShNodeMaxPrev == NULL )
            continue;
        // remove this item
        pShNodeMaxPrev->pOrder = pShNodeMax->pOrder;
        // insert it at the beginning
        pShNodeMax->pOrder = p->ppClasses[i];
        p->ppClasses[i] = pShNodeMax;
    }
    // sort the classes in the topological order
    qsort( p->ppClasses, p->nClasses, sizeof(Sh_Node_t *), 
            (int (*)(const void *, const void *)) Ver_VerifyCompareRefs );
    assert( Ver_VerifyCompareRefs( p->ppClasses, p->ppClasses + p->nClasses - 1 ) <= 0 );

    for ( i = 0; i < p->nClasses; i++ )
    {
        for ( pShNode = p->ppClasses[i], Counter = 0; pShNode; pShNode = pShNode->pOrder, Counter++ )
        {
//            printf( " %d(%s)", pShNode->Num, (pShNode->fMark?"c":"d") );
        }
//        printf( "\n" );
//printf( " %d ", Counter );
//        Extra_PrintBinary( stdout, &p->ppClasses[i]->pData, 32 );
//        printf( "\n" );

    }
//    printf( "\n" );

//Sh_NetworkShow( p->pShNet );
//Sh_NetworkWriteBlif( p->pShNet, NULL, NULL, "cord.blif" );
}
 
/**Function*************************************************************

  Synopsis    [Comparision function used for sorting commands.]

  Description []
                
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ver_VerifyCompareData( Sh_Node_t ** ppC1, Sh_Node_t ** ppC2 )
{
    return (int)((*ppC1)->pData - (*ppC2)->pData);
}
 
/**Function*************************************************************

  Synopsis    [Comparision function used for sorting commands.]

  Description []
                
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ver_VerifyCompareRefs( Sh_Node_t ** ppC1, Sh_Node_t ** ppC2 )
{
    return (int)((*ppC1)->pData2 - (*ppC2)->pData2);
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



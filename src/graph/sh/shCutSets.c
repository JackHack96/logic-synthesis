/**CFile****************************************************************

  FileName    [shCutSets.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Computation of k-feasible cutsets of all nodes.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: shCutSets.c,v 1.2 2004/04/12 20:41:08 alanmi Exp $]

***********************************************************************/

#include "shInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define   MAXNODES   5

typedef struct Sh_CutSet_t_ Sh_CutSet_t;
struct Sh_CutSet_t_
{
    Sh_Node_t *    pRoot;
    Sh_Node_t *    pLeaves[4];
    int            Volume;
    unsigned       uTruth;
    Sh_CutSet_t *  pNext;
};

static Sh_CutSet_t * Sh_CutSetAlloc( Extra_MmFixed_t * pMemMan );
static Sh_CutSet_t * Sh_CutSetDup( Extra_MmFixed_t * pMemMan, Sh_CutSet_t * pCut );
static Sh_CutSet_t * Sh_CutSetMerge( Extra_MmFixed_t * pMemMan, 
    Sh_CutSet_t * pCut1, Sh_CutSet_t * pCut2, int Limit );
static Sh_CutSet_t * Sh_CutSetMergeLists( Extra_MmFixed_t * pMemMan, 
    Sh_CutSet_t * pList1, Sh_CutSet_t * pList2, int Limit );
static int           Sh_CutSetListCount( Sh_CutSet_t * pList );
static void          Sh_CutSetListPrint( Sh_Manager_t * pMan, Sh_Node_t * pNode, Sh_CutSet_t * pList, int Limit );
static void          Sh_CutSetPrint( Sh_Manager_t * pMan, Sh_Node_t * pNode, Sh_CutSet_t * pCut, int Limit );
static int           Sh_CutSetBelongsToList( Sh_CutSet_t * pList, Sh_CutSet_t * pCut, int Limit );

static char * Sh_CutSetFormula( Sh_Manager_t * pMan, Sh_Node_t * pRoot, Sh_CutSet_t * pCut, int Limit );
static char * Sh_CutSetFormula_rec( Sh_Manager_t * pMan, Sh_Node_t * pNode, int fCompl );

static unsigned      Sh_CutSetTruthTable( Sh_Manager_t * pMan, Sh_Node_t * pNode, Sh_CutSet_t * pCut, unsigned uVarTruths[], int Limit, int * pVolume );
static unsigned      Sh_CutSetTruthTable_rec( Sh_Manager_t * pMan, Sh_Node_t * pNode, int * pVolume );
static int           Sh_CutSetCountGates( char * pForm );

static int clockMerge;
static int clockCompare;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computation of all k-feasible cuts in the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_CutSetsCompute( Sh_Manager_t * pMan, Sh_Node_t * ppNodes[], int nNodes )
{
    unsigned uVarTruths[10] = { 0xAAAA,    // 1010 1010 1010 1010
                                0xCCCC,    // 1100 1100 1100 1100
                                0xF0F0,    // 1111 0000 1111 0000
                                0xFF00 };  // 1111 1111 0000 0000
    Sh_CutSet_t * pCut, * pList;
    Sh_CutSet_t * pList1, * pList2;
    Sh_Node_t * pNode;
    int Volume, VolumeMax, VolumeMaxTotal, VolumeM;
    int nCuts, nInputs = 4;
    int i, clk;

    // start the memory manager for the cutsets
    pMan->pMemCut = Extra_MmFixedStart( sizeof(Sh_CutSet_t), 10000, 1000 );
    // create elementary cutsets for the PI variables
    for ( i = 0; i < pMan->nVars; i++ )
    {
        pCut = Sh_CutSetAlloc( pMan->pMemCut );
        pCut->pLeaves[0] = pMan->pVars[i];
        pMan->pVars[i]->pData2 = (unsigned)pCut;
    }

clockMerge = 0;
clockCompare = 0;

    // go through all the nodes in the network
clk = clock();
    for ( i = 0; i < nNodes; i++ )
    {
        pNode = ppNodes[i];
        // merge the cuts for two fanins
        pList1  = (Sh_CutSet_t *) Sh_Regular(pNode->pOne)->pData2;
        pList2  = (Sh_CutSet_t *) Sh_Regular(pNode->pTwo)->pData2;
        // derive the sorted lists
        pList = Sh_CutSetMergeLists( pMan->pMemCut, pList1, pList2, nInputs );
        // add the new cut
        pCut = Sh_CutSetAlloc( pMan->pMemCut );
        pCut->pLeaves[0] = pNode;
        // append
        pCut->pNext = pList;
        // set at the node
        pNode->pData2 = (unsigned)pCut;
    }
PRT( "Cuts",    clock() - clk );
PRT( "Merge",   clockMerge );
PRT( "Compare", clockCompare );

    // generate truth tables and compute volume for each cut
clk = clock();
    VolumeMax = -1;
    VolumeMaxTotal = 0;
    for ( i = 0; i < nNodes; i++ )
    {
        VolumeM = -1;
        pNode = ppNodes[i];
        pList = (Sh_CutSet_t *)pNode->pData2;
        for ( pCut = pList; pCut; pCut = pCut->pNext )
        {
            Volume = 0;
            pCut->uTruth = Sh_CutSetTruthTable( pMan, pNode, pCut, uVarTruths, nInputs, &Volume );
            pCut->Volume = Volume;            
            if ( VolumeM < Volume )
                VolumeM = Volume;
        }
        VolumeMaxTotal += VolumeM;
        if ( VolumeMax < VolumeM )
            VolumeMax = VolumeM;
    }
PRT( "Truth", clock() - clk );

    nCuts = 0;
//    for ( i = 0; i < nNodes; i++ )
    for ( i = nNodes/2; i < nNodes/2 + 20; i++ )
    {
        nCuts += Sh_CutSetListCount( (Sh_CutSet_t *)ppNodes[i]->pData2 );
Sh_CutSetListPrint( pMan, ppNodes[i], (Sh_CutSet_t *)ppNodes[i]->pData2, nInputs );
printf( "\n" );
    }

printf( "The number of all %d-feasible cuts = %d.\n", nInputs, nCuts );
printf( "The maximum volume = %d.\n", VolumeMax );
printf( "The average maximum volume = %d.\n", VolumeMaxTotal/nNodes );
}

/**Function*************************************************************

  Synopsis    [Allocates the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_CutSet_t * Sh_CutSetAlloc( Extra_MmFixed_t * pMemMan )
{
    Sh_CutSet_t * pCut;
    pCut = (Sh_CutSet_t *)Extra_MmFixedEntryFetch( pMemMan );
    memset( pCut, 0, sizeof(Sh_CutSet_t) );
    return pCut;
}

/**Function*************************************************************

  Synopsis    [Duplicate the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_CutSet_t * Sh_CutSetDup( Extra_MmFixed_t * pMemMan, Sh_CutSet_t * pCut )
{
    Sh_CutSet_t * pCutNew;
    pCutNew = (Sh_CutSet_t *)Extra_MmFixedEntryFetch( pMemMan );
    memcpy( pCutNew, pCut, sizeof(Sh_CutSet_t) );
    return pCutNew;
}

/**Function*************************************************************

  Synopsis    [Counts the number of entries in the list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sh_CutSetListCount( Sh_CutSet_t * pList )
{
    Sh_CutSet_t * pTemp;
    int Counter = 0;
    for ( pTemp = pList; pTemp; pTemp = pTemp->pNext )
        Counter++;
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Prints the cuts in the list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_CutSetListPrint( Sh_Manager_t * pMan, Sh_Node_t * pNode, Sh_CutSet_t * pList, int Limit )
{
    Sh_CutSet_t * pTemp;
    int Counter = 0;
    for ( pTemp = pList; pTemp; pTemp = pTemp->pNext )
    {
        printf( "%2d : ", Counter + 1 );
        Sh_CutSetPrint( pMan, pNode, pTemp, Limit );
        Counter++;
    }
}

/**Function*************************************************************

  Synopsis    [Prints the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_CutSetPrint( Sh_Manager_t * pMan, Sh_Node_t * pNode, Sh_CutSet_t * pCut, int Limit )
{
    char * pFormula = NULL;
    int nVolMin = -1;
    unsigned uTruth;
//    int i;    

    printf( "(%4d)", pNode->index );
    printf( "  vol=%2d  ", pCut->Volume );
    printf( "  %15s  ", Sh_CutSetFormula(pMan, pNode, pCut, Limit) );
    Extra_PrintBinary( stdout, &pCut->uTruth, 16 );


    uTruth = pCut->uTruth;
    if ( uTruth & ((unsigned)1) )
        printf( " *" );
    else
        printf( "  " );
    if ( uTruth & ((unsigned)1) )
        uTruth ^= 0xFFFF;

    if ( st_lookup( pMan->tSubgraphs, (char *)uTruth, &pFormula ) )
        nVolMin = Sh_CutSetCountGates( pFormula );

    printf( "  vol=%2d  ", nVolMin );
    printf( "  %15s  ", pFormula );
/*
    printf( "  {", pNode->Num );
    for ( i = 0; i < Limit; i++ )
    {
        if ( pCut->pLeaves[i] == NULL )
            break;
        printf( " %d", pCut->pLeaves[i]->Num );
    }
    printf( " }" );
*/
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Merge two lists.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_CutSet_t * Sh_CutSetMergeLists( Extra_MmFixed_t * pMemMan, 
    Sh_CutSet_t * pList1, Sh_CutSet_t * pList2, int Limit )
{
    Sh_CutSet_t * pTemp1, * pTemp2, * pMerge;
    Sh_CutSet_t * pListNew, ** ppListNew;
    int clk;

    pListNew  = NULL;
    ppListNew = &pListNew;

    // go through the cut pairs
    for ( pTemp1 = pList1; pTemp1; pTemp1 = pTemp1->pNext )
        for ( pTemp2 = pList2; pTemp2; pTemp2 = pTemp2->pNext )
        {
clk = clock();
            pMerge = Sh_CutSetMerge( pMemMan, pTemp1, pTemp2, Limit );
clockMerge += clock() - clk;
            if ( pMerge == NULL )
                continue;

            // check if this cut exists in the list
clk = clock();
            if ( Sh_CutSetBelongsToList( pListNew, pMerge, Limit ) )
            {
                // recycle this cut
                Extra_MmFixedEntryRecycle( pMemMan, (char *)pMerge );
                pMerge = NULL;
            }
clockCompare += clock() - clk;

            if ( pMerge ) // unique
            {
                *ppListNew = pMerge;
                ppListNew  = &pMerge->pNext;
            }
        }

    *ppListNew = NULL;
    return pListNew;
}

/**Function*************************************************************

  Synopsis    [Merge two lists.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sh_CutSetBelongsToList( Sh_CutSet_t * pList, Sh_CutSet_t * pCut, int Limit )
{
    Sh_CutSet_t * pTemp;
    int nChars = Limit * sizeof(int);
    for ( pTemp = pList; pTemp; pTemp = pTemp->pNext )
        if ( memcmp( pCut->pLeaves, pTemp->pLeaves, nChars ) == 0 )
            return 1;
    return 0;
}


/**Function*************************************************************

  Synopsis    [Merge two cuts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_CutSet_t * Sh_CutSetMerge( Extra_MmFixed_t * pMemMan, 
    Sh_CutSet_t * pCut1, Sh_CutSet_t * pCut2, int Limit )
{
    Sh_Node_t * pLeaves[4];
    Sh_CutSet_t * pRes;
    int i1, i2, i, k;

    for ( i = i1 = i2 = 0; i1 < Limit && i2 < Limit; i++ )
    {
        if ( pCut1->pLeaves[i1] == 0 || pCut2->pLeaves[i2] == 0 )
            break;

        if ( i == Limit )
            return NULL;

        if ( pCut1->pLeaves[i1] == pCut2->pLeaves[i2] )
        {
            pLeaves[i] = pCut1->pLeaves[i1];
            i1++;
            i2++;
        }
        else if ( pCut1->pLeaves[i1]->index < pCut2->pLeaves[i2]->index )
        {
            pLeaves[i] = pCut1->pLeaves[i1];
            i1++;
        }
        else // if ( pCut1->pLeaves[i1]->index > pCut2->pLeaves[i2]->index )
        {
            pLeaves[i] = pCut2->pLeaves[i2];
            i2++;
        }
    }

    // add the remaining entries in Cut1
    for ( ; i1 < Limit; i++, i1++ )
    {
        if ( pCut1->pLeaves[i1] == 0 )
            break;
        if ( i == Limit )
            return NULL;
        pLeaves[i] = pCut1->pLeaves[i1];
    }

    // add the remaining entries in Cut2
    for ( ; i2 < Limit; i++, i2++ )
    {
        if ( pCut2->pLeaves[i2] == 0 )
            break;
        if ( i == Limit )
            return NULL;
        pLeaves[i] = pCut2->pLeaves[i2];
    }

//    pRes = Exp_CutAlloc( pMemMan, iRoot );
    pRes = (Sh_CutSet_t *)Extra_MmFixedEntryFetch( pMemMan );
    for ( k = 0; k < i; k++ )
        pRes->pLeaves[k] = pLeaves[k];
    for ( ; k < 4; k++ )
        pRes->pLeaves[k] = 0;
    pRes->pNext = NULL;
    return pRes;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Sh_CutSetFormula( Sh_Manager_t * pMan, Sh_Node_t * pRoot, Sh_CutSet_t * pCut, int Limit )
{
    Sh_Node_t * pNode;
    char Buffer[10];
    int i;
    // set the traversal ID
    Sh_ManagerIncrementTravId( pMan );
    // set up the PIs of the cut
    for ( i = 0; i < Limit; i++ )
    {
        // get the given node
        pNode = pCut->pLeaves[i];
        if ( pNode == 0 )
            break;
        // mark the node as visited
        Sh_NodeSetTravIdCurrent( pMan, pNode );
        // set the truth table of this node
        sprintf( Buffer, "%c", i+'A' );
        pNode->pData = (unsigned)util_strsav(Buffer);
    }
    return Sh_CutSetFormula_rec( pMan, pRoot, 0 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Sh_CutSetFormula_rec( Sh_Manager_t * pMan, Sh_Node_t * pNode, int fCompl )
{
    char * pForm1, * pForm2;
    char Buffer[100];
    // if this node is already visited, skip
    if ( Sh_NodeIsTravIdCurrent( pMan, pNode ) )
    {
        if ( strlen((char *)pNode->pData) == 1 )
        {
            if ( fCompl )
            {
                Buffer[0] = 'a' + ((char *)pNode->pData)[0] - 'A';
                Buffer[1] = 0;
                return util_strsav(Buffer);
            }
            return util_strsav((char *)pNode->pData);
        }
    }
    // mark the node as visited
    Sh_NodeSetTravIdCurrent( pMan, pNode );
    // get the node's truth table
    pForm1 = Sh_CutSetFormula_rec( pMan, Sh_Regular(pNode->pOne), Sh_IsComplement(pNode->pOne) );
    pForm2 = Sh_CutSetFormula_rec( pMan, Sh_Regular(pNode->pTwo), Sh_IsComplement(pNode->pTwo) );
    sprintf( Buffer, "%s%s", pForm1, pForm2 );
    pNode->pData = (unsigned)util_strsav( Buffer );
    FREE( pForm1 );
    FREE( pForm2 );

    sprintf( Buffer, "%c%s%c", (fCompl? '<':'('), (char *)pNode->pData, (fCompl? '>':')') );
    return util_strsav(Buffer);
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Sh_CutSetTruthTable( Sh_Manager_t * pMan, Sh_Node_t * pRoot, Sh_CutSet_t * pCut, unsigned uVarTruths[], int Limit, int * pVolume )
{
    Sh_Node_t * pNode;
    int i;
    // set the traversal ID
    Sh_ManagerIncrementTravId( pMan );
    // set up the PIs of the cut
    for ( i = 0; i < Limit; i++ )
    {
        // get the given node
        pNode = pCut->pLeaves[i];
        if ( pNode == 0 )
            break;
        // mark the node as visited
        Sh_NodeSetTravIdCurrent( pMan, pNode );
        // set the truth table of this node
        pNode->pData = uVarTruths[i];
    }
    return Sh_CutSetTruthTable_rec( pMan, pRoot, pVolume ) & 0xFFFF;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Sh_CutSetTruthTable_rec( Sh_Manager_t * pMan, Sh_Node_t * pNode, int * pVolume )
{
    unsigned uTable1, uTable2;
    // if this node is already visited, skip
    if ( Sh_NodeIsTravIdCurrent( pMan, pNode ) )
        return pNode->pData;
    // mark the node as visited
    Sh_NodeSetTravIdCurrent( pMan, pNode );
    // update the volume
    (*pVolume)++;
    // get the node's truth table
    uTable1 = Sh_CutSetTruthTable_rec( pMan, Sh_Regular(pNode->pOne), pVolume );
    uTable2 = Sh_CutSetTruthTable_rec( pMan, Sh_Regular(pNode->pTwo), pVolume );
    if ( Sh_IsComplement(pNode->pOne) )
        uTable1 = ~uTable1;
    if ( Sh_IsComplement(pNode->pTwo) )
        uTable2 = ~uTable2;
    return uTable1 & uTable2;
}


/**Function*************************************************************

  Synopsis    [Returns the number of AND-gates in the formula.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sh_CutSetCountGates( char * pForm )
{
    int Counter = 0;
    for ( ; *pForm; pForm++ )
        if ( *pForm == '(' || *pForm == '<' )
            Counter++;
    return Counter;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



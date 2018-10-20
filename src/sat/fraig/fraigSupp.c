/**CFile****************************************************************

  FileName    [resSupp.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic resynthesis engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: fraigSupp.c,v 1.4 2005/07/08 01:01:33 alanmi Exp $]

***********************************************************************/

#include "fraigInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Fraig_NodeCollectTfo_rec( Fraig_Node_t * pNode, int i, unsigned ** pSuppS, Fraig_NodeVec_t * vNodes );
static int  Fraig_NodeTestSuppInfo( Fraig_Man_t * p );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes the true support of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned ** Fraig_ManGetTrueSupports( Fraig_Man_t * p, int fExact )
{
    Fraig_NodeVec_t * vNodes;
    Fraig_Node_t * pNode, * pNode1, * pNode2;
    unsigned ** pSimm0, ** pSimm1, ** pSuppS, ** pSuppF;
    unsigned * pSimmNode1, * pSimmNode2;
    unsigned * pSuppNode1, * pSuppNode2;
    int fComp1, fComp2, i, j, k;
    int nSimRounds = FRAIG_SIM_ROUNDS + p->iWordStart;

    // allocate memory for the simulation info
    pSimm0    = ALLOC( unsigned *, p->vNodes->nSize );
    pSimm0[0] = ALLOC( unsigned, p->vNodes->nSize * nSimRounds );
    for ( i = 1; i < p->vNodes->nSize; i++ )
        pSimm0[i] = pSimm0[i-1] + nSimRounds;
    // allocate memory for the simulation info
    pSimm1    = ALLOC( unsigned *, p->vNodes->nSize );
    pSimm1[0] = ALLOC( unsigned, p->vNodes->nSize * nSimRounds );
    for ( i = 1; i < p->vNodes->nSize; i++ )
        pSimm1[i] = pSimm1[i-1] + nSimRounds;

    // assign the random/systematic simulation info to the PIs
    for ( i = 0; i < p->vInputs->nSize; i++ )
    {
        pNode = p->vInputs->pArray[i];
        for ( k = 0; k < FRAIG_SIM_ROUNDS; k++ )
            pSimm0[pNode->Num][k] = pNode->pSimsR->uTests[k];
        for ( k = FRAIG_SIM_ROUNDS; k < nSimRounds; k++ )
            pSimm0[pNode->Num][k] = pNode->pSimsD->uTests[k];
    }

    // simulate the internal nodes
    for ( i = 0; i < p->vNodes->nSize; i++ )
    {
        pNode = p->vNodes->pArray[i];
        if ( !Fraig_NodeIsAnd(pNode) )
            continue;
        pSimmNode1 = pSimm0[Fraig_Regular(pNode->p1)->Num];
        pSimmNode2 = pSimm0[Fraig_Regular(pNode->p2)->Num];
        fComp1 = Fraig_IsComplement(pNode->p1);
        fComp2 = Fraig_IsComplement(pNode->p2);
        if ( fComp1 && fComp2 )
            for ( k = 0; k < nSimRounds; k++ )
                pSimm0[pNode->Num][k] = ~pSimmNode1[k] & ~pSimmNode2[k];
        else if ( fComp1 && !fComp2 )
            for ( k = 0; k < nSimRounds; k++ )
                pSimm0[pNode->Num][k] = ~pSimmNode1[k] &  pSimmNode2[k];
        else if ( !fComp1 && fComp2 )
            for ( k = 0; k < nSimRounds; k++ )
                pSimm0[pNode->Num][k] =  pSimmNode1[k] & ~pSimmNode2[k];
        else // if ( fComp1 && fComp2 )
            for ( k = 0; k < nSimRounds; k++ )
                pSimm0[pNode->Num][k] =  pSimmNode1[k] &  pSimmNode2[k];
    }

    // allocate memory for supports
    p->nSuppWords = (p->vInputs->nSize + sizeof(unsigned) * 8) / sizeof(unsigned) / 8;
    // structural support
    pSuppS     = ALLOC( unsigned *, p->vNodes->nSize );
    pSuppS[0]  = ALLOC( unsigned, p->vNodes->nSize * p->nSuppWords );
    memset( pSuppS[0], 0, sizeof(unsigned) * p->vNodes->nSize * p->nSuppWords );
    for ( i = 1; i < p->vNodes->nSize; i++ )
        pSuppS[i] = pSuppS[i-1] + p->nSuppWords;
    // functional support
    pSuppF     = ALLOC( unsigned *, p->vNodes->nSize );
    pSuppF[0]  = ALLOC( unsigned, p->vNodes->nSize * p->nSuppWords );
    memset( pSuppF[0], 0, sizeof(unsigned) * p->vNodes->nSize * p->nSuppWords );
    for ( i = 1; i < p->vNodes->nSize; i++ )
        pSuppF[i] = pSuppF[i-1] + p->nSuppWords;

    // assign the structural support to the PIs
    for ( i = 0; i < p->vInputs->nSize; i++ )
    {
        pNode = p->vInputs->pArray[i];
        Fraig_BitStringSetBit( pSuppS[pNode->Num], i );
    }
    // derive the structural supports
    for ( i = 0; i < p->vNodes->nSize; i++ )
    {
        pNode = p->vNodes->pArray[i];
        if ( !Fraig_NodeIsAnd(pNode) )
            continue;
        pSuppNode1 = pSuppS[Fraig_Regular(pNode->p1)->Num];
        pSuppNode2 = pSuppS[Fraig_Regular(pNode->p2)->Num];
        for ( k = 0; k < p->nSuppWords; k++ )
            pSuppS[pNode->Num][k] = (pSuppNode1[k] | pSuppNode2[k]);
    }

    // compute the functional supports
    vNodes = Fraig_NodeVecAlloc( 1000 );
    for ( i = 0; i < p->vInputs->nSize; i++ )
    {
        pNode = p->vInputs->pArray[i];
        assert( pNode->NumPi == i );
        Fraig_BitStringSetBit( pSuppF[pNode->Num], i );
        // set the new simulation info
        for ( k = 0; k < nSimRounds; k++ )
            pSimm1[pNode->Num][k] = ~pSimm0[pNode->Num][k];

        // collect the nodes in the TFO cone of this PI
        Fraig_NodeVecClear( vNodes );
        for ( k = 0; k < p->vOutputs->nSize; k++ )
            if ( !Fraig_NodeIsConst(p->vOutputs->pArray[k]) )
                Fraig_NodeCollectTfo_rec( Fraig_Regular(p->vOutputs->pArray[k]), i, pSuppS, vNodes );
        if ( vNodes->nSize == 0 )
            continue;
        assert( vNodes->pArray[0] == p->vNodes->pArray[pNode->Num] );

        // derive the true supports of the nodes
        for ( j = 1; j < vNodes->nSize; j++ )
        {
            pNode  = vNodes->pArray[j];
            pNode1 = Fraig_Regular(pNode->p1);
            pNode2 = Fraig_Regular(pNode->p2);

            fComp1 = Fraig_IsComplement(pNode->p1);
            fComp2 = Fraig_IsComplement(pNode->p2);

            pSimmNode1 = pNode1->fMark0? pSimm1[pNode1->Num] : pSimm0[pNode1->Num];
            pSimmNode2 = pNode2->fMark0? pSimm1[pNode2->Num] : pSimm0[pNode2->Num];

            if ( fComp1 && fComp2 )
                for ( k = 0; k < nSimRounds; k++ )
                    pSimm1[pNode->Num][k] = ~pSimmNode1[k] & ~pSimmNode2[k];
            else if ( fComp1 && !fComp2 )
                for ( k = 0; k < nSimRounds; k++ )
                    pSimm1[pNode->Num][k] = ~pSimmNode1[k] &  pSimmNode2[k];
            else if ( !fComp1 && fComp2 )
                for ( k = 0; k < nSimRounds; k++ )
                    pSimm1[pNode->Num][k] =  pSimmNode1[k] & ~pSimmNode2[k];
            else // if ( fComp1 && fComp2 )
                for ( k = 0; k < nSimRounds; k++ )
                    pSimm1[pNode->Num][k] =  pSimmNode1[k] &  pSimmNode2[k];

            // compare the nodes
            for ( k = 0; k < nSimRounds; k++ )
                if ( pSimm0[pNode->Num][k] != pSimm1[pNode->Num][k] )
                {
                    Fraig_BitStringSetBit( pSuppF[pNode->Num], i );
                    break;
                }
        }
        // clean the collected nodes
        for ( j = 0; j < vNodes->nSize; j++ )
            vNodes->pArray[j]->fMark0 = 0;
    }
    Fraig_NodeVecFree( vNodes );
 
    // if this is an exact computation, compute exact true supports
    if ( fExact )
    {
        int nSuppStruct, nSuppFunc, nSuppSatTotal, nSuppSatProve;
        int clk;

        p->fDoSparse = 0;

        for ( k = 0; k < p->vOutputs->nSize; k++ )
        {
            printf( "Output %3d : ", k );

            pNode = Fraig_Regular(p->vOutputs->pArray[k]);
            if ( !Fraig_NodeIsAnd(pNode) )
            {
                printf( "A constant.\n" );
                continue;
            }

//            if ( Fraig_CountStructuralSupport( p, pNode ) > 35 )
//                continue;

            clk = clock();
            nSuppStruct = nSuppFunc = nSuppSatTotal = nSuppSatProve = 0;
            for ( i = 0; i < p->vInputs->nSize; i++ )
            {
                if ( Fraig_BitStringHasBit( pSuppS[pNode->Num], i ) )
                    nSuppStruct++;
            }
            printf( "Struct = %3d. ", nSuppStruct );


            for ( i = 0; i < p->vInputs->nSize; i++ )
            {
                if ( Fraig_BitStringHasBit( pSuppS[pNode->Num], i ) &&
                    !Fraig_BitStringHasBit( pSuppF[pNode->Num], i ) )
                {
                    Fraig_Node_t * pCofactor;
                    nSuppSatTotal++;

                    // variable is absent in the true support
                    // iff the cof w.r.t. this var is equal to the function
//                    p->nBackTrackLimit = 3;

                    pCofactor = Fraig_Cofactor( p, pNode, p->vInputs->pArray[i] ); Fraig_Ref( pCofactor );
//                    if ( pCofactor != pNode )
                    if ( !Fraig_NodesAreEqual( p, pCofactor, pNode, -1 ) )
                        Fraig_BitStringSetBit( pSuppF[pNode->Num], i );
                    else
                        nSuppSatProve++;
                    Fraig_RecursiveDeref( p, pCofactor );
                }

                if ( Fraig_BitStringHasBit( pSuppF[pNode->Num], i ) )
                    nSuppFunc++;
            }
            printf( " Func = %3d. SAT calls = %3d. Proofs = %3d. ", 
                nSuppFunc, nSuppSatTotal, nSuppSatProve );
            PRT( "Time", clock() - clk );


        }
    }
 
    // free the memory
    FREE( pSimm0[0] );
    FREE( pSimm0 );
    FREE( pSimm1[0] );
    FREE( pSimm1 );
//    FREE( pSuppS[0] );
//    FREE( pSuppS );
//    FREE( pSuppF[0] );
//    FREE( pSuppF );
    p->pSuppS = pSuppS;
    p->pSuppF = pSuppF;
//    printf( "The number of support mismatches = %d.\n", Fraig_NodeTestSuppInfo(p) );
    return pSuppF;
}


/**Function*************************************************************

  Synopsis    [Compute the structural support.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_ManStructuralSupport( Fraig_Man_t * p )
{
    Fraig_Node_t * pNode;
    unsigned ** pSuppS, * pSuppNode1, * pSuppNode2;
    int i, k;

    // allocate memory for supports
    p->nSuppWords = (p->vInputs->nSize + sizeof(unsigned) * 8) / sizeof(unsigned) / 8;

    // structural support
    pSuppS     = ALLOC( unsigned *, p->vNodes->nSize );
    pSuppS[0]  = ALLOC( unsigned, p->vNodes->nSize * p->nSuppWords );
    memset( pSuppS[0], 0, sizeof(unsigned) * p->vNodes->nSize * p->nSuppWords );
    for ( i = 1; i < p->vNodes->nSize; i++ )
        pSuppS[i] = pSuppS[i-1] + p->nSuppWords;

    // assign the structural support to the PIs
    for ( i = 0; i < p->vInputs->nSize; i++ )
    {
        pNode = p->vInputs->pArray[i];
        Fraig_BitStringSetBit( pSuppS[pNode->Num], i );
    }
    // derive the structural supports
    for ( i = 0; i < p->vNodes->nSize; i++ )
    {
        pNode = p->vNodes->pArray[i];
        if ( !Fraig_NodeIsAnd(pNode) )
            continue;
        pSuppNode1 = pSuppS[Fraig_Regular(pNode->p1)->Num];
        pSuppNode2 = pSuppS[Fraig_Regular(pNode->p2)->Num];
        for ( k = 0; k < p->nSuppWords; k++ )
            pSuppS[pNode->Num][k] = (pSuppNode1[k] | pSuppNode2[k]);
    }
    p->pSuppS = pSuppS;
}



/**Function*************************************************************

  Synopsis    [Recursively computes the functional support of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_NodeCollectTfo_rec( Fraig_Node_t * pNode, int i, unsigned ** pSuppS, Fraig_NodeVec_t * vNodes )
{
    assert( !Fraig_IsComplement(pNode) );
    // if the node has been visited, quit
    if ( pNode->fMark0 )
        return;
    // if the node is not in the TFO of the PI, quit
    if ( !Fraig_BitStringHasBit( pSuppS[pNode->Num], i ) )
        return;
    // call recursively
    if ( Fraig_NodeIsAnd(pNode) )
    {
        Fraig_NodeCollectTfo_rec( Fraig_Regular(pNode->p1), i, pSuppS, vNodes );
        Fraig_NodeCollectTfo_rec( Fraig_Regular(pNode->p2), i, pSuppS, vNodes );
    }
    // add the node
    Fraig_NodeVecPush( vNodes, pNode );
    // mark the node
    assert( pNode->fMark0 == 0 ); // check for no loops
    pNode->fMark0 = 1;
}





#if 0
/**Function*************************************************************

  Synopsis    [Test support info for mismatches.]

  Description [This procedure is supposed to detect the case when the node
  depends on a variable, while both of its children do not depend on it.
  Clearly, this happens because of the weakness of random simulation.
  So this procedure can be used to measure the strength of simulation.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_NodeTestSuppInfo( Fraig_Man_t * p )
{
    Fraig_Node_t * pNode;
    unsigned * pSuppNode1, * pSuppNode2;
    unsigned Supp1, Supp2;
    int i, k, Counter;
    // get the number of words in the support info
    assert( p->nSuppWords == (p->vInputs->nSize + (int)sizeof(unsigned) * 8) / (int)sizeof(unsigned) / 8 );
    // go through the nodes and count the mismatches
    Counter = 0;
    for ( i = 0; i < p->vNodes->nSize; i++ )
    {
        pNode = p->vNodes->pArray[i];
        if ( !Fraig_NodeIsAnd(pNode) )
            continue;
        pSuppNode1 = p->pSuppF[Fraig_Regular(pNode->p1)->Num];
        pSuppNode2 = p->pSuppF[Fraig_Regular(pNode->p2)->Num];

        // only one child has it (Supp1) => the node has it (Supp2)
        // Supp1 => Supp2    or    Supp1 & !Supp2 == 0
        for ( k = 0; k < p->nSuppWords; k++ )
        {
            Supp1 = pSuppNode1[k] ^ pSuppNode2[k]; // only one of the children has it
            Supp2 = p->pSuppF[pNode->Num][k];      // the node has it
            if ( Supp1 & ~Supp2 )
            {
                Counter++;
/*
        printf( "Node %3d : \n", pNode->Num );
        Extra_PrintBinary( stdout, pSuppNode1, p->vInputs->nSize ); printf( "\n" );
        Extra_PrintBinary( stdout, pSuppNode2, p->vInputs->nSize ); printf( "\n" );
        Extra_PrintBinary( stdout, p->pSuppF[pNode->Num], p->vInputs->nSize ); printf( "\n" );
        printf( "structure : \n", pNode->Num );
        Extra_PrintBinary( stdout, p->pSuppS[Fraig_Regular(pNode->p1)->Num], p->vInputs->nSize ); printf( "\n" );
        Extra_PrintBinary( stdout, p->pSuppS[Fraig_Regular(pNode->p2)->Num], p->vInputs->nSize ); printf( "\n" );
        Extra_PrintBinary( stdout, p->pSuppS[pNode->Num], p->vInputs->nSize ); printf( "\n" );
*/
                break;
            }
        }


        // the node has it (Supp1) => at least one of the children has it (Supp2)
        // Supp1 => Supp2    or    Supp1 & !Supp2 == 0
        for ( k = 0; k < p->nSuppWords; k++ )
        {
            Supp1 = p->pSuppF[pNode->Num][k];      // the node has it
            Supp2 = pSuppNode1[k] | pSuppNode2[k]; // at least one of the children has it
            if ( Supp1 & ~Supp2 )
            {
                Counter++;
/*
        printf( "Node %3d : \n", pNode->Num );
        Extra_PrintBinary( stdout, pSuppNode1, p->vInputs->nSize ); printf( "\n" );
        Extra_PrintBinary( stdout, pSuppNode2, p->vInputs->nSize ); printf( "\n" );
        Extra_PrintBinary( stdout, p->pSuppF[pNode->Num], p->vInputs->nSize ); printf( "\n" );
*/
                break;
            }
        }
    }
/*
    // print the supports for the primary outputs
    for ( k = 0; k < p->vOutputs->nSize; k++ )
    {
        pNode = Fraig_Regular(p->vOutputs->pArray[k]);
        if ( !Fraig_NodeIsConst(pNode) )
        {
            printf( "Node %3d : \n", pNode->Num );
            Extra_PrintBinary( stdout, p->pSuppF[pNode->Num], p->vInputs->nSize ); printf( "\n" );
        }
    }
*/
    return Counter;
}
#endif

#if 0

typedef struct Fraig_PatSet_t_ Fraig_PatSet_t;
struct Fraig_PatSet_t_
{
    int         fOne;        // patterns are for 1 or 0
    float       aProb;       // the (small) probability of getting 1 or 0
    int         nPats;       // the number of patterns collected
    int         nPatsAlloc;  // the number of patterns allocated
    int         nVars;       // the number of variables 
    int         nWords;      // the number of machine words in each pattern
    unsigned ** pPats;       // the two dimensional array of bit-patterns
};


// access of the temporary simulation information
#define FRAIG_SUPP_GET_SIMINFO(pNode)          ((unsigned *)(pNode)->pData0)
#define FRAIG_SUPP_SET_SIMINFO(pNode, pInfo)   ((pNode)->pData0 = (Fraig_Node_t *)(pInfo))
#define FRAIG_SUPP_GET_SIMINFO2(pNode)         ((unsigned *)(pNode)->pData1)
#define FRAIG_SUPP_SET_SIMINFO2(pNode, pInfo)  ((pNode)->pData1 = (Fraig_Node_t *)(pInfo))

// static procedures
static int Fraig_NodeSuppSimulate( Fraig_NodeVec_t * vNodes, int * pVarsToCheck, int nVarsToCheck );
static int Fraig_NodeVecMovePisForward( Fraig_Man_t * p, Fraig_Node_t * pRoot, Fraig_NodeVec_t * vNodes );
static Fraig_PatSet_t * Fraig_PatternsCompute( Fraig_NodeVec_t * vNodes, int nPatsMax );
static void Fraig_PatternsDelete( Fraig_PatSet_t * pPats );

/**Function*************************************************************

  Synopsis    [Computes support of all nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_ManComputeSupports( Fraig_Man_t * p, int nSimLimit, int fExact, int fVerbose )
{
    Fraig_Node_t * pNode;
    int i, v, nSuppStr, nSuppFun;
    int nTotalStr = 0, nTotalFun = 0;

    for ( i = 0; i < p->vOutputs->nSize; i++ )
    {
        pNode    = Fraig_Regular(p->vOutputs->pArray[i]);
        nSuppStr = Fraig_BitStringCountOnes( pNode->pSuppStr, p->nSuppWords );
        nSuppFun = Fraig_NodeComputeTrueSupport( p, pNode, nSimLimit, fExact );
        nTotalStr += nSuppStr;
        nTotalFun += nSuppFun;

        // print out the number of variables in the structural and the functional supports
        printf( "Out #%3d  %6s: ", i, p->ppOutputNames[i] );
        printf( "Structural = %3d. ", nSuppStr );
        printf( "Functional = %3d. ", nSuppFun );
        printf( "\n" );
        // print out the variables
        if ( fVerbose && p->ppInputNames )
        {
            printf( "Structural:  {" );
            for ( v = 0; v < p->vInputs->nSize; v++ )
                if ( Fraig_BitStringHasBit( pNode->pSuppStr, i ) )
                    printf( " %s", p->ppInputNames[ p->vNodes->pArray[i]->Num ] );
            printf( " }\n" );
            printf( "Functional:  {" );
            for ( v = 0; v < p->vInputs->nSize; v++ )
                if ( Fraig_BitStringHasBit( pNode->pSuppFun, i ) )
                    printf( " %s", p->ppInputNames[ p->vNodes->pArray[i]->Num ] );
            printf( " }\n" );
        }
    }
    printf( "nTotalStr = %5d. nTotalFun = %5d.\n", nTotalStr, nTotalFun );
}

/**Function*************************************************************

  Synopsis    [Returns the number of variales in the true support.]

  Description [Uses the fact that the function may be sparse.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_NodeComputeTrueSupport( Fraig_Man_t * p, Fraig_Node_t * pRoot, int nSimLimit, int fExact )
{
    Fraig_NodeVec_t * vNodes; // the nodes of the TFI cone of pRoot
    unsigned * pSimInfo; // the temporary simulation info used in this procedure
    int * pVarsToCheck;  // the indices of variables to check for belonging to the support
    int nSuppStr;        // the number of variables in the structural support
    int nSuppVac;        // the number of variables prooved vacuous
    int nRemNew;         // the current number of remaining variables to check
    int nRemOld;         // the previous number of remaining variables to check
    int i, nCounter;

    // collect the nodes in the cone
    pRoot  = Fraig_Regular(pRoot);
    vNodes = Fraig_DfsOne( p, pRoot, 0 );
    assert( pRoot == vNodes->pArray[vNodes->nSize - 1] );
    // move the PIs forward in the given order
    nSuppStr = Fraig_NodeVecMovePisForward( p, pRoot, vNodes );

    // allocate memory for the simulation info and attach it to the nodes
    pSimInfo = ALLOC( unsigned, vNodes->nSize * FRAIG_SIM_ROUNDS );
    for ( i = 0; i < vNodes->nSize; i++ )
        FRAIG_SUPP_SET_SIMINFO( vNodes->pArray[i], pSimInfo + i * FRAIG_SIM_ROUNDS );

    // start the array of vars to be checked with the variables in the structural support
    pVarsToCheck = ALLOC( int, nSuppStr );
    for ( i = 0; i < nSuppStr; i++ )
        pVarsToCheck[i] = i;

    // run the simulation rounds to detect false support
    nCounter = 0;
    nRemOld = nSuppStr;
    while ( 1 )
    {
        nRemNew = Fraig_NodeSuppSimulate( vNodes, pVarsToCheck, nRemOld );
        if ( nRemNew == 0 ) // no vars remaining or no change
            break;
        // perform at most 'nSimLimit' rounds when there is no change
        if ( nRemNew == nRemOld )
        {
            nCounter++;
//            if ( nCounter % (nSimLimit/2) == 0 )
//                printf( "." );
            if ( nCounter == nSimLimit )
                break;
        }
        else if ( nRemNew < nRemOld )
            nCounter = 0;
        nRemOld = nRemNew;
    }

    if ( nRemNew > 0 && fExact )
    {
        for ( i = 0; i < nRemNew; i++ )
        {
            Fraig_Node_t * pCofactor;
            int nBackTrackLimit = p->nBackTrackLimit;

            // variable is absent in the true support
            // iff the cof w.r.t. this var is equal to the function
            p->nBackTrackLimit = 3;

            pCofactor = Fraig_Cofactor( p, pRoot, vNodes->pArray[ pVarsToCheck[i] ] ); Fraig_Ref( pCofactor );
            if ( !Fraig_NodesAreaEqualLimit( p, pCofactor, pRoot ) ) // var belongs
                pVarsToCheck[i] = -1;
            Fraig_RecursiveDeref( p, pCofactor );

            p->nBackTrackLimit = nBackTrackLimit;
        }

        // compact the array of variables
        for ( i = nSuppVac = 0; i < nRemNew; i++ )
            if ( pVarsToCheck[i] >= 0 )
                pVarsToCheck[nSuppVac++] = pVarsToCheck[i];
    }
    else
        nSuppVac = nRemNew;


    // collect the true support memory for the true support
    pRoot->pSuppFun = (unsigned *)Fraig_MemFixedEntryFetch( p->mmSupps );
    memcpy( pRoot->pSuppFun, pRoot->pSuppStr, sizeof(unsigned) * p->nSuppWords );
    for ( i = 0; i < nSuppVac; i++ )
        Fraig_BitStringXorBit( pRoot->pSuppFun, vNodes->pArray[ pVarsToCheck[i] ]->Num );

    for ( i = 0; i < vNodes->nSize; i++ )
        FRAIG_SUPP_SET_SIMINFO( vNodes->pArray[i], NULL );
    Fraig_NodeVecFree( vNodes );
    free( pSimInfo );
    free( pVarsToCheck );
    return nSuppStr - nSuppVac;
}

/**Function*************************************************************

  Synopsis    [Moves the PI nodes forward.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_NodeVecMovePisForward( Fraig_Man_t * p, Fraig_Node_t * pRoot, Fraig_NodeVec_t * vNodes )
{
    Fraig_Node_t ** pArray;
    int i, nNodes, nSuppVars;
    // allocate the array
    pArray = ALLOC( Fraig_Node_t *, vNodes->nCap );
    // copy the PIs in the right order
    for ( i = nNodes = 0; i < p->vInputs->nSize; i++ )
        if ( Fraig_BitStringHasBit( pRoot->pSuppStr, i ) )
            pArray[nNodes++] = p->vNodes->pArray[i];
    nSuppVars = nNodes;
    // copy the remaining nodes
    for ( i = 0; i < vNodes->nSize; i++ )
        if ( Fraig_NodeIsAnd(vNodes->pArray[i]) )
            pArray[nNodes++] = vNodes->pArray[i];
    assert( nNodes == vNodes->nSize );
    free( vNodes->pArray );
    vNodes->pArray = pArray;
    return nSuppVars;
}

/**Function*************************************************************

  Synopsis    [Simulate one round of simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_NodeSuppSimulate( Fraig_NodeVec_t * vNodes, int * pVarsToCheck, int nVarsToCheck )
{
    Fraig_PatSet_t * pPats;
    Fraig_Node_t * pRoot, * pNode;
    unsigned * pRootSimInfo, * pSims, * pSims0, * pSims1;
    int nWords  = FRAIG_NUM_WORDS(nVarsToCheck); // how many words we need for one round
    int nRounds = FRAIG_SIM_ROUNDS / nWords;     // how many rounds we can do at a time
    int nBits   = FRAIG_SIM_ROUNDS * 32;         // the total number of bits in the sim info 
    int fCompl0, fCompl1, nSuppStr, iNew, w, r, i;

    // get the number of vars in the structural support
    for ( nSuppStr = 0; nSuppStr < vNodes->nSize; nSuppStr++ )
        if ( Fraig_NodeIsAnd( vNodes->pArray[nSuppStr] ) )
            break;

    // collect the pattern numbers to use
    pPats = Fraig_PatternsCompute( vNodes, nRounds );
    if ( nRounds > pPats->nPats )
        nRounds = pPats->nPats;

    // go through the rounds
    for ( r = 0; r < nRounds; r++ )
    {
        // set the PI info of this round
        for ( i = 0; i < nSuppStr; i++ )
        {
            pNode = vNodes->pArray[i];
            pSims = FRAIG_SUPP_GET_SIMINFO( pNode );
            if ( Fraig_BitStringHasBit( pPats->pPats[r], i ) )
            {
                for ( w = 0; w < nWords; w++ )
                    pSims[r * nWords + w] = FRAIG_FULL;
            }
            else
            {
                for ( w = 0; w < nWords; w++ )
                    pSims[r * nWords + w] = 0;
            }
        }
        // flip the diagonal bit
        for ( i = 0; i < nVarsToCheck; i++ )
        {
            pNode = vNodes->pArray[ pVarsToCheck[i] ];
            pSims = FRAIG_SUPP_GET_SIMINFO( pNode );
            Fraig_BitStringXorBit( pSims + r * nWords, i );
        }
    }

for ( i = 0; i < nSuppStr; i++ )
{
    pNode = vNodes->pArray[i];
    pSims = FRAIG_SUPP_GET_SIMINFO( pNode );
    Extra_PrintBinary( stdout, pSims, 100 ); printf( "\n" );
}

    // simulate
    for ( i = nSuppStr; i < vNodes->nSize; i++ )
    {
        pNode = vNodes->pArray[i];
        assert( Fraig_NodeIsAnd(pNode) );

        fCompl0 = Fraig_IsComplement( pNode->p1 );
        fCompl1 = Fraig_IsComplement( pNode->p2 );
        pSims   = FRAIG_SUPP_GET_SIMINFO( pNode );
        pSims0  = FRAIG_SUPP_GET_SIMINFO( Fraig_Regular(pNode->p1) );
        pSims1  = FRAIG_SUPP_GET_SIMINFO( Fraig_Regular(pNode->p2) );
        if ( fCompl0 && fCompl1 )
            for ( w = 0; w < FRAIG_SIM_ROUNDS; w++ )
                pSims[w] = ~pSims0[w] & ~pSims1[w];
        else if ( !fCompl0 && fCompl1 )
            for ( w = 0; w < FRAIG_SIM_ROUNDS; w++ )
                pSims[w] = pSims0[w] & ~pSims1[w];
        else if ( fCompl0 && !fCompl1 )
            for ( w = 0; w < FRAIG_SIM_ROUNDS; w++ )
                pSims[w] = ~pSims0[w] & pSims1[w];
        else // if ( fCompl0 && fCompl1 )
            for ( w = 0; w < FRAIG_SIM_ROUNDS; w++ )
                pSims[w] = pSims0[w] & pSims1[w];
    }

    // collect the resulting info by considering every var
    pRoot = vNodes->pArray[vNodes->nSize - 1];
    pRootSimInfo = FRAIG_SUPP_GET_SIMINFO( pRoot );
    if ( pPats->fOne )
    {
        for ( i = 0; i < nVarsToCheck; i++ )
            for ( r = 0; r < nRounds; r++ )
                if ( !Fraig_BitStringHasBit( pRootSimInfo + r * nWords, i ) )
                {
                    pVarsToCheck[i] = -1;
                    break;
                }
    }
    else
    {
        for ( i = 0; i < nVarsToCheck; i++ )
            for ( r = 0; r < nRounds; r++ )
                if ( Fraig_BitStringHasBit( pRootSimInfo + r * nWords, i ) )
                {
                    pVarsToCheck[i] = -1;
                    break;
                }
    }
    Fraig_PatternsDelete( pPats );

Extra_PrintBinary( stdout, pRoot->pSims->uTests, 100 ); printf( "\n" );
Extra_PrintBinary( stdout, pRootSimInfo, 100 ); printf( "\n" );

    printf( "Original ratio = %7.5f.  Final ratio = %7.5f.\n", pRoot->aProb, 
        ((float)Fraig_BitStringCountOnes( pRootSimInfo, FRAIG_SIM_ROUNDS ))/FRAIG_SIM_ROUNDS/32 );

    // compact the array of variables
    for ( i = iNew = 0; i < nVarsToCheck; i++ )
        if ( pVarsToCheck[i] >= 0 )
            pVarsToCheck[iNew++] = pVarsToCheck[i];
    return iNew;
}


/**Function*************************************************************

  Synopsis    [Allocates a set of patterns.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_PatSet_t * Fraig_PatternsCompute( Fraig_NodeVec_t * vNodes, int nPatsMax )
{
    Fraig_PatSet_t * p;
    Fraig_Node_t * pRoot, * pNode;
    unsigned * pSimInfo, * pSims, * pSims0, * pSims1;
    int nSimRounds, fCompl0, fCompl1, iBit, i, w, b;

    // get the root of the cone
    pRoot = vNodes->pArray[ vNodes->nSize - 1 ];

    p = ALLOC( Fraig_PatSet_t, 1 );
    memset( p, 0, sizeof(Fraig_PatSet_t) );
    p->fOne = (pRoot->aProb < 0.5);
    p->aProb = p->fOne? pRoot->aProb : 1 - pRoot->aProb;
    p->nPatsAlloc = nPatsMax;
    for ( i = 0; i < vNodes->nSize; i++ )
        if ( Fraig_NodeIsAnd(vNodes->pArray[i]) )
            break;
    p->nVars = i;
    p->nWords = FRAIG_NUM_WORDS(p->nVars);
    p->pPats    = ALLOC( unsigned *, p->nPatsAlloc );
    p->pPats[0] = ALLOC( unsigned,   p->nPatsAlloc * p->nWords ); 
    memset( p->pPats[0], 0, sizeof(unsigned) * p->nPatsAlloc * p->nWords );
    for ( i = 1; i < p->nPatsAlloc; i++ )
        p->pPats[i] = p->pPats[i-1] + p->nWords;

    // allocate simulation info to solve the task
    nSimRounds = (int)((13.0 * nPatsMax / 12) / (p->aProb * 32));
    if ( nSimRounds > 10000 ) // if we are asking for more than 10,000 words per node
        nSimRounds = 10000;

    // allocate memory for the simulation info and attach it to the nodes
    pSimInfo = ALLOC( unsigned, vNodes->nSize * nSimRounds );
    for ( i = 0; i < vNodes->nSize; i++ )
        FRAIG_SUPP_SET_SIMINFO2( vNodes->pArray[i], pSimInfo + i * nSimRounds );

    // assign the elementary input variables
    for ( i = 0; i < p->nVars; i++ )
    {
        pNode = vNodes->pArray[i];
        pSims = FRAIG_SUPP_GET_SIMINFO2( pNode );
        for ( w = 0; w < nSimRounds; w++ )
            pSims[w] = FRAIG_RANDOM_UNSIGNED;
    }

    // simulate
    for ( i = p->nVars; i < vNodes->nSize; i++ )
    {
        pNode = vNodes->pArray[i];
        assert( Fraig_NodeIsAnd(pNode) );

        fCompl0 = Fraig_IsComplement( pNode->p1 );
        fCompl1 = Fraig_IsComplement( pNode->p2 );
        pSims   = FRAIG_SUPP_GET_SIMINFO2( pNode );
        pSims0  = FRAIG_SUPP_GET_SIMINFO2( Fraig_Regular(pNode->p1) );
        pSims1  = FRAIG_SUPP_GET_SIMINFO2( Fraig_Regular(pNode->p2) );
        if ( fCompl0 && fCompl1 )
            for ( w = 0; w < nSimRounds; w++ )
                pSims[w] = ~pSims0[w] & ~pSims1[w];
        else if ( !fCompl0 && fCompl1 )
            for ( w = 0; w < nSimRounds; w++ )
                pSims[w] = pSims0[w] & ~pSims1[w];
        else if ( fCompl0 && !fCompl1 )
            for ( w = 0; w < nSimRounds; w++ )
                pSims[w] = ~pSims0[w] & pSims1[w];
        else // if ( fCompl0 && fCompl1 )
            for ( w = 0; w < nSimRounds; w++ )
                pSims[w] = pSims0[w] & pSims1[w];
    }

    // collect the patterns
    pSims = FRAIG_SUPP_GET_SIMINFO2( pRoot );
    for ( w = 0; w < nSimRounds; w++ )
    {
        if ( !p->fOne )
            pSims[w] = ~pSims[w];
        if ( pSims[w] == 0 )
            continue;
        for ( b = 0; b < 32; b++ )
            if ( pSims[w] & (1 << b) )
            {
                iBit = ((w << 5) | b);
                for ( i = 0; i < p->nVars; i++ )
                    if ( Fraig_BitStringHasBit( FRAIG_SUPP_GET_SIMINFO2(vNodes->pArray[i]), iBit ) )
                        Fraig_BitStringSetBit( p->pPats[p->nPats], i );
                p->nPats++;
                if ( p->nPats == p->nPatsAlloc )
                    break;
            }
        if ( p->nPats == p->nPatsAlloc )
            break;
    }

    // free the temporary info
    for ( i = 0; i < vNodes->nSize; i++ )
        FRAIG_SUPP_SET_SIMINFO2( vNodes->pArray[i], NULL );
    free( pSimInfo );
    return p;
}

/**Function*************************************************************

  Synopsis    [Delete the patterns.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_PatternsDelete( Fraig_PatSet_t * pPats )
{
    free( pPats->pPats[0] );
    free( pPats->pPats );
    free( pPats );
}

/**Function*************************************************************

  Synopsis    [Test pattern computation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_PatternsTest( Fraig_Man_t * pMan )
{
    Fraig_PatSet_t * pPats;
    Fraig_NodeVec_t * vNodes;
    int i, clk;

    vNodes = Fraig_DfsOne( pMan, Fraig_Regular(pMan->vOutputs->pArray[0]), 0 );
    Fraig_NodeVecMovePisForward( pMan, Fraig_Regular(pMan->vOutputs->pArray[0]), vNodes );

clk = clock();
    pPats = Fraig_PatternsCompute( vNodes, 500 );
PRT( "Patterns", clock() - clk );

    for ( i = 0; i < pPats->nPats; i++ )
    {
        printf( "%3d ", i );
        Extra_PrintBinary( stdout, pPats->pPats[i], pPats->nVars );
        printf( "\n" );
    }

    Fraig_NodeVecFree( vNodes );
    free( pPats->pPats[0] );
    free( pPats->pPats );
    free( pPats );
}
#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



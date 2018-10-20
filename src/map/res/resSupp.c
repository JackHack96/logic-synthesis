/**CFile****************************************************************

  FileName    [resSupp.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic resynthesis engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: resSupp.c,v 1.1 2005/01/23 06:59:49 alanmi Exp $]

***********************************************************************/

#include "resInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Res_NodeCollectTfo_rec( Fpga_Node_t * pNode, int i, unsigned ** pSuppS, Fpga_NodeVec_t * vNodes );
static int  Res_NodeTestSuppInfo( Res_Man_t * p );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes the true support of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_ManGetTrueSupports( Res_Man_t * p )
{
    Fpga_NodeVec_t * vNodes;
    Fpga_Node_t * pNode, * pNode1, * pNode2;
    unsigned ** pSimm0, ** pSimm1, ** pSuppS, ** pSuppF;
    unsigned * pSimmNode1, * pSimmNode2;
    unsigned * pSuppNode1, * pSuppNode2;
    int fComp1, fComp2, i, j, k;

    // sort the nodes by level
    Fpga_NodeVecSortByLevel( p->pMan->vAnds );
    // make sure the first nodes are PIs
    for ( i = 0; i < p->pMan->nInputs; i++ )
        assert( p->pMan->vAnds->pArray[i]->Num == i );
    // reassign the numbers
    for ( i = 0; i < p->pMan->vAnds->nSize; i++ )
        p->pMan->vAnds->pArray[i]->NumA = i;

    // allocate memory for the simulation info
    pSimm0    = ALLOC( unsigned *, p->pMan->vAnds->nSize );
    pSimm0[0] = ALLOC( unsigned, p->pMan->vAnds->nSize * p->pMan->nSimRounds );
    for ( i = 1; i < p->pMan->vAnds->nSize; i++ )
        pSimm0[i] = pSimm0[i-1] + p->pMan->nSimRounds;
    // allocate memory for the simulation info
    pSimm1    = ALLOC( unsigned *, p->pMan->vAnds->nSize );
    pSimm1[0] = ALLOC( unsigned, p->pMan->vAnds->nSize * p->pMan->nSimRounds );
    for ( i = 1; i < p->pMan->vAnds->nSize; i++ )
        pSimm1[i] = pSimm1[i-1] + p->pMan->nSimRounds;

    // assign the random/systematic simulation info to the PIs
    for ( i = 0; i < p->pMan->nInputs; i++ )
        for ( k = 0; k < p->pMan->nSimRounds; k++ )
            pSimm0[i][k] = p->pMan->pSimInfo[i][k];
    // simulate the internal nodes
    for ( i = 0; i < p->pMan->vAnds->nSize; i++ )
    {
        pNode = p->pMan->vAnds->pArray[i];
        if ( !Fpga_NodeIsAnd(pNode) )
            continue;
        pSimmNode1 = pSimm0[Fpga_Regular(pNode->p1)->NumA];
        pSimmNode2 = pSimm0[Fpga_Regular(pNode->p2)->NumA];
        fComp1 = Fpga_IsComplement(pNode->p1);
        fComp2 = Fpga_IsComplement(pNode->p2);
        if ( fComp1 && fComp2 )
            for ( k = 0; k < p->pMan->nSimRounds; k++ )
                pSimm0[pNode->NumA][k] = ~pSimmNode1[k] & ~pSimmNode2[k];
        else if ( fComp1 && !fComp2 )
            for ( k = 0; k < p->pMan->nSimRounds; k++ )
                pSimm0[pNode->NumA][k] = ~pSimmNode1[k] &  pSimmNode2[k];
        else if ( !fComp1 && fComp2 )
            for ( k = 0; k < p->pMan->nSimRounds; k++ )
                pSimm0[pNode->NumA][k] =  pSimmNode1[k] & ~pSimmNode2[k];
        else // if ( fComp1 && fComp2 )
            for ( k = 0; k < p->pMan->nSimRounds; k++ )
                pSimm0[pNode->NumA][k] =  pSimmNode1[k] &  pSimmNode2[k];
    }

    // allocate memory for supports
    p->nSuppWords = (p->pMan->nInputs + sizeof(unsigned) * 8) / sizeof(unsigned) / 8;
    // structural support
    pSuppS     = ALLOC( unsigned *, p->pMan->vAnds->nSize );
    pSuppS[0]  = ALLOC( unsigned, p->pMan->vAnds->nSize * p->nSuppWords );
    memset( pSuppS[0], 0, sizeof(unsigned) * p->pMan->vAnds->nSize * p->nSuppWords );
    for ( i = 1; i < p->pMan->vAnds->nSize; i++ )
        pSuppS[i] = pSuppS[i-1] + p->nSuppWords;
    // functional support
    pSuppF     = ALLOC( unsigned *, p->pMan->vAnds->nSize );
    pSuppF[0]  = ALLOC( unsigned, p->pMan->vAnds->nSize * p->nSuppWords );
    memset( pSuppF[0], 0, sizeof(unsigned) * p->pMan->vAnds->nSize * p->nSuppWords );
    for ( i = 1; i < p->pMan->vAnds->nSize; i++ )
        pSuppF[i] = pSuppF[i-1] + p->nSuppWords;

    // assign the structural support to the PIs
    for ( i = 0; i < p->pMan->nInputs; i++ )
        Fpga_NodeSetSuppVar( pSuppS[i], i );
    // derive the structural supports
    for ( i = 0; i < p->pMan->vAnds->nSize; i++ )
    {
        pNode = p->pMan->vAnds->pArray[i];
        if ( !Fpga_NodeIsAnd(pNode) )
            continue;
        pSuppNode1 = pSuppS[Fpga_Regular(pNode->p1)->NumA];
        pSuppNode2 = pSuppS[Fpga_Regular(pNode->p2)->NumA];
        for ( k = 0; k < p->nSuppWords; k++ )
            pSuppS[pNode->NumA][k] = pSuppNode1[k] | pSuppNode2[k];
    }

    // compute the functional supports
    vNodes = Fpga_NodeVecAlloc( 1000 );
    for ( i = 0; i < p->pMan->nInputs; i++ )
    {
        Fpga_NodeSetSuppVar( pSuppF[i], i );
        // set the new simulation info
        for ( k = 0; k < p->pMan->nSimRounds; k++ )
            pSimm1[i][k] = ~pSimm0[i][k];

        // collect the nodes in the TFO cone of this PI
        Fpga_NodeVecClear( vNodes );
        for ( k = 0; k < p->pMan->nOutputs; k++ )
            if ( !Fpga_NodeIsConst(p->pMan->pOutputs[k]) )
                Res_NodeCollectTfo_rec( Fpga_Regular(p->pMan->pOutputs[k]), i, pSuppS, vNodes );
        if ( vNodes->nSize == 0 )
            continue;
        assert( vNodes->pArray[0] == p->pMan->pInputs[i] );

        // derive the true supports of the nodes
        for ( j = 1; j < vNodes->nSize; j++ )
        {
            pNode  = vNodes->pArray[j];
            pNode1 = Fpga_Regular(pNode->p1);
            pNode2 = Fpga_Regular(pNode->p2);

            fComp1 = Fpga_IsComplement(pNode->p1);
            fComp2 = Fpga_IsComplement(pNode->p2);

            pSimmNode1 = pNode1->fMark0? pSimm1[pNode1->NumA] : pSimm0[pNode1->NumA];
            pSimmNode2 = pNode2->fMark0? pSimm1[pNode2->NumA] : pSimm0[pNode2->NumA];

            if ( fComp1 && fComp2 )
                for ( k = 0; k < p->pMan->nSimRounds; k++ )
                    pSimm1[pNode->NumA][k] = ~pSimmNode1[k] & ~pSimmNode2[k];
            else if ( fComp1 && !fComp2 )
                for ( k = 0; k < p->pMan->nSimRounds; k++ )
                    pSimm1[pNode->NumA][k] = ~pSimmNode1[k] &  pSimmNode2[k];
            else if ( !fComp1 && fComp2 )
                for ( k = 0; k < p->pMan->nSimRounds; k++ )
                    pSimm1[pNode->NumA][k] =  pSimmNode1[k] & ~pSimmNode2[k];
            else // if ( fComp1 && fComp2 )
                for ( k = 0; k < p->pMan->nSimRounds; k++ )
                    pSimm1[pNode->NumA][k] =  pSimmNode1[k] &  pSimmNode2[k];

            // compare the nodes
            for ( k = 0; k < p->pMan->nSimRounds; k++ )
                if ( pSimm0[pNode->NumA][k] != pSimm1[pNode->NumA][k] )
                {
                    Fpga_NodeSetSuppVar( pSuppF[pNode->NumA], i );
                    break;
                }
        }
        // clean the collected nodes
        for ( j = 0; j < vNodes->nSize; j++ )
            vNodes->pArray[j]->fMark0 = 0;
    }
    Fpga_NodeVecFree( vNodes );

    // free the memory
//    FREE( pSimm0[0] );
//    FREE( pSimm0 );
    FREE( pSimm1[0] );
    FREE( pSimm1 );
//    FREE( pSuppS[0] );
//    FREE( pSuppS );
//    FREE( pSuppF[0] );
//    FREE( pSuppF );
    // assign the simulation and support info
    p->pSimms = pSimm0;
    p->pSuppF = pSuppF;
    p->pSuppS = pSuppS;
//    printf( "The number of support mismatches = %d.\n", Res_NodeTestSuppInfo(p) );
}

/**Function*************************************************************

  Synopsis    [Recursively computes the functional support of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_NodeCollectTfo_rec( Fpga_Node_t * pNode, int i, unsigned ** pSuppS, Fpga_NodeVec_t * vNodes )
{
    assert( !Fpga_IsComplement(pNode) );
    // if the node has been visited, quit
    if ( pNode->fMark0 )
        return;
    // if the node is not in the TFO of the PI, quit
    if ( !Fpga_NodeHasSuppVar( pSuppS[pNode->NumA], i ) )
        return;
    // call recursively
    if ( Fpga_NodeIsAnd(pNode) )
    {
        Res_NodeCollectTfo_rec( Fpga_Regular(pNode->p1), i, pSuppS, vNodes );
        Res_NodeCollectTfo_rec( Fpga_Regular(pNode->p2), i, pSuppS, vNodes );
    }
    // add the node
    Fpga_NodeVecPush( vNodes, pNode );
    // mark the node
    assert( pNode->fMark0 == 0 ); // check for no loops
    pNode->fMark0 = 1;
}

/**Function*************************************************************

  Synopsis    [Test support info for mismatches.]

  Description [This procedure is supposed to detect the case when the node
  depends on a variable, while both of its children do not depend on it.
  Clearly, this happens because of the weakness of random simulation.
  So this procedure can be used to measure the strength of simulation.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Res_NodeTestSuppInfo( Res_Man_t * p )
{
    Fpga_Node_t * pNode;
    unsigned * pSuppNode1, * pSuppNode2;
    unsigned Supp1, Supp2;
    int i, k, Counter;
    // get the number of words in the support info
    assert( p->nSuppWords == (p->pMan->nInputs + (int)sizeof(unsigned) * 8) / (int)sizeof(unsigned) / 8 );
    // go through the nodes and count the mismatches
    Counter = 0;
    for ( i = 0; i < p->pMan->vAnds->nSize; i++ )
    {
        pNode = p->pMan->vAnds->pArray[i];
        if ( !Fpga_NodeIsAnd(pNode) )
            continue;
        pSuppNode1 = p->pSuppF[Fpga_Regular(pNode->p1)->NumA];
        pSuppNode2 = p->pSuppF[Fpga_Regular(pNode->p2)->NumA];

        // only one child has it (Supp1) => the node has it (Supp2)
        // Supp1 => Supp2    or    Supp1 & !Supp2 == 0
        for ( k = 0; k < p->nSuppWords; k++ )
        {
            Supp1 = pSuppNode1[k] ^ pSuppNode2[k]; // only one of the children has it
            Supp2 = p->pSuppF[pNode->NumA][k];      // the node has it
            if ( Supp1 & ~Supp2 )
            {
                Counter++;
/*
        printf( "Node %3d : \n", pNode->NumA );
        Extra_PrintBinary( stdout, pSuppNode1, p->pMan->nInputs ); printf( "\n" );
        Extra_PrintBinary( stdout, pSuppNode2, p->pMan->nInputs ); printf( "\n" );
        Extra_PrintBinary( stdout, p->pSuppF[pNode->NumA], p->pMan->nInputs ); printf( "\n" );
        printf( "structure : \n", pNode->NumA );
        Extra_PrintBinary( stdout, p->pSuppS[Fpga_Regular(pNode->p1)->NumA], p->pMan->nInputs ); printf( "\n" );
        Extra_PrintBinary( stdout, p->pSuppS[Fpga_Regular(pNode->p2)->NumA], p->pMan->nInputs ); printf( "\n" );
        Extra_PrintBinary( stdout, p->pSuppS[pNode->NumA], p->pMan->nInputs ); printf( "\n" );
*/
                break;
            }
        }


        // the node has it (Supp1) => at least one of the children has it (Supp2)
        // Supp1 => Supp2    or    Supp1 & !Supp2 == 0
        for ( k = 0; k < p->nSuppWords; k++ )
        {
            Supp1 = p->pSuppF[pNode->NumA][k];      // the node has it
            Supp2 = pSuppNode1[k] | pSuppNode2[k]; // at least one of the children has it
            if ( Supp1 & ~Supp2 )
            {
                Counter++;
/*
        printf( "Node %3d : \n", pNode->NumA );
        Extra_PrintBinary( stdout, pSuppNode1, p->pMan->nInputs ); printf( "\n" );
        Extra_PrintBinary( stdout, pSuppNode2, p->pMan->nInputs ); printf( "\n" );
        Extra_PrintBinary( stdout, p->pSuppF[pNode->NumA], p->pMan->nInputs ); printf( "\n" );
*/
                break;
            }
        }
    }
/*
    // print the supports for the primary outputs
    for ( k = 0; k < p->pMan->nOutputs; k++ )
    {
        pNode = Fpga_Regular(p->pMan->pOutputs[k]);
        if ( !Fpga_NodeIsConst(pNode) )
        {
            printf( "Node %3d : \n", pNode->NumA );
            Extra_PrintBinary( stdout, p->pSuppF[pNode->NumA], p->pMan->nInputs ); printf( "\n" );
        }
    }
*/
    return Counter;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



/**CFile****************************************************************

  FileName    [dualNet.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Interprets the network as the formula representing the aumaton.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 18, 2003.]

  Revision    [$Id: dualNet.c,v 1.2 2004/02/19 03:06:53 alanmi Exp $]

***********************************************************************/

#include "dualInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Dual_Spec_t * Dual_DeriveSpecFromMvc( Ntk_Network_t * pNet, Mvc_Cover_t * pMvc1, Mvc_Cover_t * pMvc2, int Rank );
static DdNode * Dual_DeriveSpecFromMvcOne( DdManager * dd, Mvc_Cover_t * pMvc, int Rank, int nVars );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Derives the spec from the current network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dual_Spec_t * Dual_DeriveSpecFromNetwork( Ntk_Network_t * pNet, int Rank, bool fOrder )
{
    Dual_Spec_t * pSpec;
    FILE * pOut, * pErr;
    Ntk_Node_t * pNodePo, * pNode1, * pNode2, * pNodeCi, * pFanin;
    Ntk_Pin_t * pPin;
    Cvr_Cover_t * pCvr1, * pCvrNew1;
    Cvr_Cover_t * pCvr2, * pCvrNew2;
    Mvc_Cover_t * pMvc1, * pMvc2;
    int * pPermuteInv;
    int nInputs, i, k;

    pOut = Mv_FrameReadOut(Ntk_NetworkReadMvsis(pNet));
    pErr = Mv_FrameReadErr(Ntk_NetworkReadMvsis(pNet));

    if ( Ntk_NetworkReadNodeIntNum(pNet) > 2 )
    {
        fprintf( pErr, "The current network has more than one internal node (%d).\n", Ntk_NetworkReadNodeIntNum(pNet) );
        fprintf( pErr, "Cannot process such networks.\n" );
        return NULL;
    }

    nInputs = Ntk_NetworkReadCiNum(pNet);
    if ( nInputs < Rank + 1 )
    {
        fprintf( pErr, "The number of PIs (%d) is too small compared to the rank (%d+1).\n", nInputs, Rank );
        fprintf( pErr, "Cannot process such networks.\n" );
        return NULL;
    }
    if ( nInputs % (Rank + 1) != 0 )
    {
        fprintf( pErr, "The number of PIs (%d) does not divide evenly by the rank (%d+1).\n", nInputs, Rank );
        fprintf( pErr, "Cannot process such networks.\n" );
        return NULL;
    }

    // get the first internal node
    pNodePo = Ntk_NetworkFindNodeByName( pNet, "Out" );
    if ( pNodePo == NULL )
    {
        fprintf( pErr, "The network does not contain the node \"Out\".\n" );
        fprintf( pErr, "Cannot process such networks.\n" );
        return NULL;
    }

    pNode1 = Ntk_NodeReadFaninNode( pNodePo, 0 );
    assert( nInputs == Ntk_NodeReadFaninNum(pNode1) );

    // get the second internal node
    pNodePo = Ntk_NetworkFindNodeByName( pNet, "Init" );
    if ( pNodePo == NULL )
        pNode2 = NULL;
    else
    {
        pNode2 = Ntk_NodeReadFaninNode( pNodePo, 0 );
        assert( nInputs == Ntk_NodeReadFaninNum(pNode2) );
    }


    if ( fOrder )
    {
        // order the node's fanin's according to their order in the PI list
        // pPermuteInv contains for each new variable its place in the old order
        pPermuteInv = ALLOC( int, nInputs + 1 );
        i = 0;
        Ntk_NetworkForEachCi( pNet, pNodeCi )
        {
            Ntk_NodeForEachFaninWithIndex( pNode1, pPin, pFanin, k )
                if ( pFanin == pNodeCi )
                    break;
            assert( k < nInputs );
            pPermuteInv[i++] = k;
        }
        pPermuteInv[nInputs] = nInputs;

        // find the new Cvr
        pCvr1 = Ntk_NodeGetFuncCvr( pNode1 );
        pCvrNew1 = Cvr_CoverCreatePermuted( pCvr1, pPermuteInv );
        if ( pNode2 )
        {
            pCvr2 = Ntk_NodeGetFuncCvr( pNode2 );
            pCvrNew2 = Cvr_CoverCreatePermuted( pCvr2, pPermuteInv );
        }
        FREE( pPermuteInv );
    }
    else
    {
        pCvr1 = Ntk_NodeGetFuncCvr( pNode1 );
        pCvrNew1 = Cvr_CoverDup( pCvr1 );
        if ( pNode2 ) 
        {
            pCvr2 = Ntk_NodeGetFuncCvr( pNode2 );
            pCvrNew2 = Cvr_CoverDup( pCvr2 );
        }
    }

    // get the new cover
    pMvc1 = Cvr_CoverReadIsets( pCvrNew1 )[1];
    pMvc2 = pNode2? Cvr_CoverReadIsets( pCvrNew2 )[1] : NULL;

    // build the spec from the cover
    pSpec = Dual_DeriveSpecFromMvc( pNet, pMvc1, pMvc2, Rank );
    Cvr_CoverFree( pCvrNew1 );
    if ( pNode2 ) 
        Cvr_CoverFree( pCvrNew2 );

    return pSpec;
}

/**Function*************************************************************

  Synopsis    [Derives the spec from the given cover.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dual_Spec_t * Dual_DeriveSpecFromMvc( Ntk_Network_t * pNet, Mvc_Cover_t * pMvc1, Mvc_Cover_t * pMvc2, int Rank )
{
    Dual_Spec_t * p;
    DdManager * dd;
    Ntk_Node_t * pNode;
    int nInputs, i;

    nInputs = Ntk_NetworkReadCiNum(pNet);

    p = Dual_SpecAlloc();
    p->pMan       = Ntk_NetworkReadMan(pNet);
    p->pName      = util_strsav( pNet->pName );
    p->dd = dd    = Cudd_Init( nInputs, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    p->bSpec      = NULL;
    p->nRank      = Rank;
    p->nVars      = nInputs / (Rank + 1);
    p->nVarsIn    = nInputs / (Rank + 1);
    p->nVarsPar   = 0;
    p->nVarsOut   = 0;
    p->nVarsAlloc = p->nVars;

    i = 0;
    p->ppVarNames = ALLOC( char *, p->nVars );
    for ( i = 0; i < p->nVars; i++ )
    {
        pNode = Ntk_NetworkReadCiNode( pNet, p->nVars * Rank + i );
        p->ppVarNames[i] = util_strsav( pNode->pName );
    }

    p->pbVars  = ALLOC( DdNode *, p->nVars );
    for ( i = 0; i < p->nVars; i++ )
        p->pbVars[i] = dd->vars[ p->nVars * p->nRank + i ];

    // create the tree
    assert( dd->size == (p->nRank + 1) * p->nVars );
    Cudd_MakeTreeNode( dd, 0, dd->size, MTR_FIXED );
    Cudd_MakeTreeNode( dd, 0, p->nRank * p->nVars, MTR_DEFAULT );
    Cudd_MakeTreeNode( dd, p->nRank * p->nVars, p->nVars, MTR_DEFAULT );

    // derive the spec
    Cudd_AutodynEnable( dd, CUDD_REORDER_SYMM_SIFT );

    p->bSpec = Dual_DeriveSpecFromMvcOne( dd, pMvc1, p->nRank, p->nVars );  Cudd_Ref( p->bSpec );
    if ( pMvc2 )
        p->bCondInit = Dual_DeriveSpecFromMvcOne( dd, pMvc2, p->nRank, p->nVars );  
    else
        p->bCondInit = b1;
    Cudd_Ref( p->bCondInit );

printf( "BDD size before reordering = %d. ", Cudd_DagSize(p->bSpec) );
    Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 100 );
    Cudd_AutodynDisable( dd );
printf( "BDD size after reordering = %d.\n", Cudd_DagSize(p->bSpec) );

    return p;
}

/**Function*************************************************************

  Synopsis    [Reads one cover.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Dual_DeriveSpecFromMvcOne( DdManager * dd, Mvc_Cover_t * pMvc, int Rank, int nVars )
{
    Mvc_Cube_t * pCube;
    DdNode * bCube, * bCover, * bTemp, * bVar;
    int v, d, iVar, Value;

    bCover = b0;  Cudd_Ref( bCover );
    Mvc_CoverForEachCube( pMvc, pCube )
    {
        // write the first product
        iVar = 0;
        bCube = b1;   Cudd_Ref( bCube );
        for ( d = Rank; d >= 0; d-- )
        {
            for ( v = 0; v < nVars; v++ )
            {
                Value = Mvc_CubeVarValue( pCube, (Rank - d) * nVars + v );
                if ( Value == 1 )
                {
                    bVar = Cudd_Not( dd->vars[iVar] );
                    bCube = Cudd_bddAnd( dd, bTemp = bCube, bVar );  Cudd_Ref( bCube );
                    Cudd_RecursiveDeref( dd, bTemp );
                }
                else if ( Value == 2 )
                {
                    bVar = dd->vars[iVar];
                    bCube = Cudd_bddAnd( dd, bTemp = bCube, bVar );  Cudd_Ref( bCube );
                    Cudd_RecursiveDeref( dd, bTemp );
                }
                else if ( Value == 3 )
                {
                }
                else
                {
                    assert( 0 );
                }
                iVar++;
            }
        }
        assert( iVar == dd->size );
        bCover = Cudd_bddOr( dd, bTemp = bCover, bCube );  Cudd_Ref( bCover );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bCube );
    }
    Cudd_Deref( bCover );
    return bCover;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



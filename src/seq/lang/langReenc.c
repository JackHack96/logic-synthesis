/**CFile****************************************************************

  FileName    [langReenc.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Implicit language solving flow.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: langReenc.c,v 1.3 2004/02/19 03:06:54 alanmi Exp $]

***********************************************************************/

#include "langInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

struct Lang_Reenc_t_
{
    DdManager *    dd;             // the DD manager used for reencoding
    DdNode *       bReenc;         // the reencoding relation
    DdNode *       bCubeCs;        // the cube of CS vars
    int            nInputs;        // the number of IO vars in the original map
    int            nLatches;       // the number of latches in the original map
    int            nStatesNew;     // the new number of states
    int            nStBitsNew;     // the new number of state bits
    int            iVarLast;       // the index of the additional variable
    Vmx_VarMap_t * pVmxOld;        // the old variable map
    Vmx_VarMap_t * pVmxExt;        // the map, which contains additional state bits at the bottom
    Vmx_VarMap_t * pVmxNew;        // the map, in which old CS is mapped into new, similarly for NS
};

static DdNode * Lang_AutoReencodeDeriveRelation( Lang_Reenc_t * p, DdNode * bStates );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////
/**Function*************************************************************

  Synopsis    [Prepares the data structures for reencoding.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lang_AutoReencode( Lang_Auto_t * pAut )
{
    DdManager * dd = pAut->dd;
    Lang_Reenc_t * p;
    DdNode * bTemp;

    // perform reencoding of state bits
    p = Lang_AutoReencodePrepare( pAut );
    // remap stuff
    pAut->bInit = Lang_AutoReencodeOne( p, bTemp = pAut->bInit );            Cudd_Ref( pAut->bInit );
    Cudd_RecursiveDeref( dd, bTemp );
    pAut->bStates = Lang_AutoReencodeOne( p, bTemp = pAut->bStates );        Cudd_Ref( pAut->bStates );
    Cudd_RecursiveDeref( dd, bTemp );
    pAut->bAccepting = Lang_AutoReencodeOne( p, bTemp = pAut->bAccepting );  Cudd_Ref( pAut->bAccepting );
    Cudd_RecursiveDeref( dd, bTemp );

Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 100 );
Cudd_AutodynEnable( dd, CUDD_REORDER_SYMM_SIFT );
    // remap the relation
    pAut->bRel = Lang_AutoReencodeTwo( p, bTemp = pAut->bRel );              Cudd_Ref( pAut->bRel );
    Cudd_RecursiveDeref( dd, bTemp );

    // update the automaton after reencoding
    pAut->nLatches = 1;
    pAut->nStateBits = Extra_Base2Log( pAut->nStates + 1 );
    pAut->pVmx = Lang_AutoReencodeReadVmxNew( p );

    // remove the structures
    Lang_AutoReencodeCleanup( p );
Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 100 );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Prepares the data structures for reencoding.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Lang_Reenc_t * Lang_AutoReencodePrepare( Lang_Auto_t * pAut )
{
    Lang_Reenc_t * p;
    DdManager * dd = pAut->dd;
    DdNode ** pbVarsCs, ** pbVarsAs;
    int v;

    // start the remapping data structure
    p = ALLOC( Lang_Reenc_t, 1 );
    memset( p, 0, sizeof(Lang_Reenc_t) );
    p->dd         = pAut->dd;
    p->nInputs    = pAut->nInputs;
    p->nLatches   = pAut->nLatches;
    p->nStatesNew = pAut->nStates + 1;  // reserve for completion
    p->nStBitsNew = Extra_Base2Log( p->nStatesNew );
    p->iVarLast   = pAut->nInputs + 2 * pAut->nLatches;
    p->pVmxOld    = pAut->pVmx;
    p->pVmxExt    = Lang_VarMapReencodeExt( pAut, p->nStatesNew );
    p->pVmxNew    = Lang_VarMapReencodeNew( pAut, p->nStatesNew );
    p->bCubeCs    = Vmx_VarMapCharCubeRange( dd, p->pVmxExt, pAut->nInputs, pAut->nLatches ); Cudd_Ref( p->bCubeCs );
    // add new variables at the bottom
    for ( v = 0; v < p->nStBitsNew; v++ )
        Cudd_bddNewVar( dd );

    p->bReenc     = Lang_AutoReencodeDeriveRelation( p, pAut->bStates );   Cudd_Ref( p->bReenc );
/*
    {
        DdNode * bCubeIO, * bCubeCs, * bCubeNs, * bCubeNew;
        bCubeIO = Vmx_VarMapCharCubeRange( dd, p->pVmxExt, 0, pAut->nInputs ); Cudd_Ref( bCubeIO );
        bCubeCs = Vmx_VarMapCharCubeRange( dd, p->pVmxExt, pAut->nInputs, pAut->nLatches ); Cudd_Ref( bCubeCs );
        bCubeNs = Vmx_VarMapCharCubeRange( dd, p->pVmxExt, pAut->nInputs + pAut->nLatches, pAut->nLatches ); Cudd_Ref( bCubeNs );
        bCubeNew= Vmx_VarMapCharCubeRange( dd, p->pVmxExt, pAut->nInputs + 2 * pAut->nLatches, 1 ); Cudd_Ref( bCubeNew );
    PRB( dd, bCubeIO );
    PRB( dd, bCubeCs );
    PRB( dd, bCubeNs );
    PRB( dd, bCubeNew );
        Cudd_RecursiveDeref( dd, bCubeIO );
        Cudd_RecursiveDeref( dd, bCubeCs );
        Cudd_RecursiveDeref( dd, bCubeNs );
        Cudd_RecursiveDeref( dd, bCubeNew );
    }
*/
printf( "Reencoding relation before reordering = %d. ", Cudd_DagSize(p->bReenc) );
// reorder the global BDDs one more time
Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 100 );
printf( "After reordering = %d.\n", Cudd_DagSize(p->bReenc) );

    // setup the mapping in such a way that it will replace CS by AS
    pbVarsCs = Vmx_VarMapBinVarsRange( pAut->dd, p->pVmxExt, pAut->nInputs, pAut->nLatches );
    pbVarsAs = Vmx_VarMapBinVarsVar( pAut->dd, p->pVmxExt, p->iVarLast );
    Cudd_SetVarMap( pAut->dd, pbVarsCs, pbVarsAs, p->nStBitsNew );
    FREE( pbVarsCs );
    FREE( pbVarsAs );
    return p;
}

/**Function*************************************************************

  Synopsis    [Frees the data structures for reencoding.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vmx_VarMap_t * Lang_AutoReencodeReadVmxNew( Lang_Reenc_t * p )
{
    return p->pVmxNew;
}

/**Function*************************************************************

  Synopsis    [Frees the data structures for reencoding.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lang_AutoReencodeCleanup( Lang_Reenc_t * p )
{
    Cudd_RecursiveDeref( p->dd, p->bReenc );
    Cudd_RecursiveDeref( p->dd, p->bCubeCs );
    FREE( p );
}

/**Function*************************************************************

  Synopsis    [Remaps function, which depends on CS variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Lang_AutoReencodeOne( Lang_Reenc_t * p, DdNode * bFunc )
{
    DdManager * dd = p->dd;
    DdNode * bRemap, * bTemp;
    bRemap = Cudd_bddAndAbstract( dd, bFunc, p->bReenc, p->bCubeCs );  Cudd_Ref( bRemap );
    bRemap = Cudd_bddVarMap( dd, bTemp = bRemap );                     Cudd_Ref( bRemap );
    Cudd_RecursiveDeref( dd, bTemp );
    Cudd_Deref( bRemap );
    return bRemap;
}

/**Function*************************************************************

  Synopsis    [Remaps function, which depends on both CS and NS variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Lang_AutoReencodeTwo( Lang_Reenc_t * p, DdNode * bFunc )
{
    DdManager * dd = p->dd;
    DdNode * bRemap, * bRemap1, * bRemap2, * bTemp, * bReencS, * bCubeS;
    DdNode ** pbVarsNs, ** pbVarsAs;

//PRS( dd, bFunc );
    // reencode the CS vars
    bRemap1 = Cudd_bddAndAbstract( dd, bFunc, p->bReenc, p->bCubeCs );    Cudd_Ref( bRemap1 );
    bRemap1 = Cudd_bddVarMap( dd, bTemp = bRemap1 );                      Cudd_Ref( bRemap1 );
    Cudd_RecursiveDeref( dd, bTemp );
//PRS( dd, bRemap1 );
//Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 100 );

    // remap the reencoding relation into NS vars
    bReencS = Vmx_VarMapRemapRange( dd, p->bReenc, p->pVmxExt, 
        p->nInputs, p->nLatches, p->nInputs + p->nLatches, p->nLatches ); Cudd_Ref( bReencS );
    bCubeS = Vmx_VarMapRemapRange( dd, p->bCubeCs, p->pVmxExt, 
        p->nInputs, p->nLatches, p->nInputs + p->nLatches, p->nLatches ); Cudd_Ref( bCubeS );
    // reencode the NS vars
    bRemap2 = Cudd_bddAndAbstract( dd, bRemap1, bReencS, bCubeS );        Cudd_Ref( bRemap2 );
    Cudd_RecursiveDeref( dd, bRemap1 );
    Cudd_RecursiveDeref( dd, bReencS );
    Cudd_RecursiveDeref( dd, bCubeS );
//PRS( dd, bRemap2 );
//Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 100 );

    // remap AS into NS
    pbVarsNs = Vmx_VarMapBinVarsRange( dd, p->pVmxExt, p->nInputs + p->nLatches, p->nLatches );
    pbVarsAs = Vmx_VarMapBinVarsVar( dd, p->pVmxExt, p->iVarLast );
    bRemap   = Cudd_bddSwapVariables( dd, bRemap2, pbVarsNs, pbVarsAs, p->nStBitsNew ); Cudd_Ref( bRemap );
    Cudd_RecursiveDeref( dd, bRemap2 );
    FREE( pbVarsNs );
    FREE( pbVarsAs );
//PRS( dd, bRemap );

    Cudd_Deref( bRemap );
    return bRemap;
}



/**Function*************************************************************

  Synopsis    [Computes the reencoding relation in the new manager.]

  Description [The reachable states are expressed using current state vars.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Lang_AutoReencodeDeriveRelation( Lang_Reenc_t * p, DdNode * bStates )
{
    DdNode ** pbCodes;
    DdManager * dd = p->dd;
    DdNode * bFunc, * bMint, * bRel, * bTemp, * bProd;
    int iMint;

    // get the codes of the additional var to be used in the relation
    pbCodes = Vmx_VarMapEncodeVarMinterm( dd, p->pVmxExt, p->iVarLast );
    // iterate through mintems (states) of the reachable set and create the relation
    bFunc = bStates;     Cudd_Ref( bFunc );
    bRel  = b0;          Cudd_Ref( bRel );
    for ( iMint = 0; bFunc != b0; iMint++ )
    {
        bMint    = Extra_bddGetOneMinterm( dd, bFunc, p->bCubeCs );    Cudd_Ref( bMint );
        // subtract the minterm from the reachable states
        bFunc    = Cudd_bddAnd( dd, bTemp = bFunc, Cudd_Not(bMint) );  Cudd_Ref( bFunc );
        Cudd_RecursiveDeref( dd, bTemp );
        // create the product of this minterm by its code
        bProd    = Cudd_bddAnd( dd, bMint, pbCodes[iMint] );           Cudd_Ref( bProd );
        Cudd_RecursiveDeref( dd, bMint );
        // add the product to the relation
        bRel = Cudd_bddOr( dd, bTemp = bRel, bProd );                  Cudd_Ref( bRel );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bProd );
    }
    assert( iMint == p->nStatesNew - 1 );
    Cudd_RecursiveDeref( dd, bFunc );
    Vmx_VarMapEncodeDeref( dd, p->pVmxExt, pbCodes );
    Cudd_Deref( bRel );
    return bRel;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



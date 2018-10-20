/**CFile****************************************************************

  FileName    [langUtils.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Implicit language solving flow.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: langStg.c,v 1.5 2004/02/19 03:06:55 alanmi Exp $]

***********************************************************************/

#include "langInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static DdNode *  Lang_ExtractStgInitValue( DdManager * dd, Ntk_Network_t * pNet );
static DdNode *  Lang_ExtractStgCodeDomain( DdManager * dd, Ntk_Network_t * pNet );
static void      Lang_ExtractStgLongNames( Lang_Auto_t * pAut, Ntk_Network_t * pNet );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Extracts STG from the multi-level multi-valued network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Lang_Auto_t * Lang_ExtractStg( Ntk_Network_t * pNet, int fLongNames )
{
//    Lang_Reenc_t * p;
    Lang_Auto_t * pAut;
    Ntk_Node_t * pNode;
    Vmx_VarMap_t * pVmxGlo, * pVmxNew;
    DdNode * bDomain, * bStatesNs, * bTemp;
    DdNode * bRelT, * bRelO;
    DdManager * dd;
    char Buffer[1000];
    int nBitsOut, nPIs, nPOs, i;
    
    // compute the global functions
    Ntk_NetworkGlobalComputeCo( pNet, 1, 0, 0 );
    dd = Ntk_NetworkReadDdGlo( pNet );

    // get hold of the global manager and the global variable map
    dd       = Ntk_NetworkReadDdGlo( pNet );
    pVmxGlo  = Ntk_NetworkReadVmxGlo( pNet );
    nPIs     = Ntk_NetworkReadCiNum(pNet);
    nPOs     = Ntk_NetworkReadCoNum(pNet);

    // reorder the global BDDs one more time
    Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 100 );
    Cudd_AutodynDisable( dd );

    // create the global variable map: (mixed(PI,CS), mixed(PO,NS))
    pVmxGlo = Ntk_NetworkGlobalCreateVmxCo( pNet, 0 );
    Ntk_NetworkSetVmxGlo( pNet, pVmxGlo );
    // permute this map to have: (PI, PO, CS, NS)
    pVmxNew = Lang_VarMapStg( pNet );

//Vmx_VarMapPrint( pVmxGlo );
//Vmx_VarMapPrint( pVmxNew );
    
    // start the automaton
    pAut = ALLOC( Lang_Auto_t, 1 );
    memset( pAut, 0, sizeof(Lang_Auto_t) );
    // set the parameters 
    sprintf( Buffer, "%s_stg", pNet->pName );
    pAut->pName      = util_strsav( Buffer );
    pAut->pMan       = pNet->pMan;
    pAut->dd         = dd;
    pAut->nLatches   = Ntk_NetworkReadLatchNum( pNet );
    pAut->nInputs    = nPIs + nPOs - 2 * pAut->nLatches;
//    pAut->nOutputs   = nPOs - pAut->nLatches;
    pAut->nOutputs   = 0;
    pAut->pVmx       = pVmxNew; 
    pAut->nStateBits = Vmx_VarMapBitsNumRange( pAut->pVmx, pAut->nInputs, pAut->nLatches );
    pAut->nStates    = -1;

    // assign the input/output names in their natural order
    // this is compatible with Lang_VarMapStg()
    i = 0;
    pAut->ppNamesInput = ALLOC( char *, pAut->nInputs );
    Ntk_NetworkForEachCi( pNet, pNode )
        if ( pNode->Subtype != MV_BELONGS_TO_LATCH )
            pAut->ppNamesInput[i++] = util_strsav( pNode->pName );
    Ntk_NetworkForEachCo( pNet, pNode )
        if ( pNode->Subtype != MV_BELONGS_TO_LATCH )
            pAut->ppNamesInput[i++] = util_strsav( pNode->pName );
//    pAut->ppNamesOutput = ALLOC( char *, pAut->nOutputs );

    // add the new variables for the outputs at the botton and interleave NS vars with CS vars
    nBitsOut = Vmx_VarMapBitsNumRange( pVmxNew, nPIs, nPOs );
    Lang_AutoExtendManager( pAut, nBitsOut );

    // get the initial value
    pAut->bInit = Lang_ExtractStgInitValue( dd, pNet );       Cudd_Ref( pAut->bInit );

    // compute the transition relation with reordering
    bRelT = Ntk_NetworkGlobalRelation( pNet, 1, 1 );          Cudd_Ref( bRelT );
    // compute the output relation with reordering
    bRelO = Ntk_NetworkGlobalRelation( pNet, 0, 1 );          Cudd_Ref( bRelO );
/*
    {
        DdNode * bRel;
        bRel = Cudd_bddAnd( dd, bRelT, bRelO );  Cudd_Ref( bRel );
        Cudd_RecursiveDeref( dd, bRelT );
        Cudd_RecursiveDeref( dd, bRelO );
        Cudd_RecursiveDeref( dd, bRel );
    }
*/
    // prevent manager from being freed
    Ntk_NetworkWriteDdGlo( pNet, NULL );
    // undo the global functions
    Ntk_NetworkGlobalDeref( pNet );

    // perform reachability
    pAut->bRel = bRelT; 
    Lang_Reachability( pAut, 1, 0 );
    pAut->bRel = NULL;

    // reduce the reachable states to only one minterm per state
    bDomain = Lang_ExtractStgCodeDomain( dd, pNet );                    Cudd_Ref( bDomain );
    pAut->bStates = Cudd_bddAnd( dd, bTemp = pAut->bStates, bDomain );  Cudd_Ref( pAut->bStates );
    Cudd_RecursiveDeref( dd, bTemp );
    Cudd_RecursiveDeref( dd, bDomain );

    bStatesNs = Vmx_VarMapRemapRange( dd, pAut->bStates, pAut->pVmx, 
        pAut->nInputs, pAut->nLatches, pAut->nInputs + pAut->nLatches, pAut->nLatches ); Cudd_Ref( bStatesNs );

    bRelT = Cudd_bddAnd( dd, bTemp = bRelT, pAut->bStates );           Cudd_Ref( bRelT );
    Cudd_RecursiveDeref( dd, bTemp );
    bRelT = Cudd_bddAnd( dd, bTemp = bRelT, bStatesNs );                Cudd_Ref( bRelT );
    Cudd_RecursiveDeref( dd, bTemp );

    bRelO = Cudd_bddAnd( dd, bTemp = bRelO, pAut->bStates );           Cudd_Ref( bRelO );
    Cudd_RecursiveDeref( dd, bTemp );
    bRelO = Cudd_bddAnd( dd, bTemp = bRelO, bStatesNs );                Cudd_Ref( bRelO );
    Cudd_RecursiveDeref( dd, bTemp );

    Cudd_RecursiveDeref( dd, bStatesNs );

    // set the number of reachable states
    pAut->nStates = (int)Cudd_CountMinterm( dd, pAut->bStates, pAut->nStateBits );
//printf( "The number of reachable states is %d.\n", pAut->nStates );

    // set the accepting states
    pAut->bAccepting = pAut->bStates;  Cudd_Ref( pAut->bAccepting );
    pAut->nAccepting = pAut->nStates;

    // if long names are requested, assign them for reachable states
    if ( fLongNames )
        Lang_ExtractStgLongNames( pAut, pNet );
/*
    ///////////////////////////////////////////
    // perform reencoding of state bits
    p = Lang_AutoReencodePrepare( pAut );
    // remap stuff
    pAut->bInit = Lang_AutoReencodeOne( p, bTemp = pAut->bInit );            Cudd_Ref( pAut->bInit );
    Cudd_RecursiveDeref( dd, bTemp );
    pAut->bStates = Lang_AutoReencodeOne( p, bTemp = pAut->bStates );        Cudd_Ref( pAut->bStates );
    Cudd_RecursiveDeref( dd, bTemp );
    pAut->bAccepting = Lang_AutoReencodeOne( p, bTemp = pAut->bAccepting );  Cudd_Ref( pAut->bAccepting );
    Cudd_RecursiveDeref( dd, bTemp );
    // remap the relations
    bRelO = Lang_AutoReencodeOne( p, bTemp = bRelO );                        Cudd_Ref( bRelO );
    Cudd_RecursiveDeref( dd, bTemp );
Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 100 );
Cudd_AutodynEnable( dd, CUDD_REORDER_SYMM_SIFT );
    bRelT = Lang_AutoReencodeTwo( p, bTemp = bRelT );                        Cudd_Ref( bRelT );
    Cudd_RecursiveDeref( dd, bTemp );

    // update the automaton after reencoding
    pAut->nLatches = 1;
    pAut->nStateBits = Extra_Base2Log( pAut->nStates + 1 );
    pAut->pVmx = Lang_AutoReencodeReadVmxNew( p );
    // remove the structures
    Lang_AutoReencodeCleanup( p );

    // get the transition relation of the automaton
    pAut->bRel = Cudd_bddAnd( dd, bRelT, bRelO );                        Cudd_Ref( pAut->bRel );
    Cudd_RecursiveDeref( dd, bRelT );
    Cudd_RecursiveDeref( dd, bRelO );
// reorder the global BDDs one more time
Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 100 );
    ///////////////////////////////////////////
*/
    // compute the relation
    pAut->bRel = Cudd_bddAnd( dd, bRelT, bRelO );                        Cudd_Ref( pAut->bRel );
    Cudd_RecursiveDeref( dd, bRelT );
    Cudd_RecursiveDeref( dd, bRelO );
// reorder the global BDDs one more time
Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 100 );


    fprintf( stdout, "The extracted STG has %d states and %d transitions.\n", 
        pAut->nStates, Lang_AutoCountTransitions(pAut) );


    Lang_AutoVerifySupport( pAut );
/*
    // complete the automaton
    if ( Lang_AutoComplete( pAut ) )
        pAut->nStates++;
*/
    // reorder the global BDDs one more time
    Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 100 );
    Cudd_AutodynDisable( dd );
    return pAut;
}

/**Function*************************************************************

  Synopsis    [Creates the cube of latch reset values.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Lang_ExtractStgInitValue( DdManager * dd, Ntk_Network_t * pNet )
{
    Vmx_VarMap_t * pVmxExt;
    DdNode * bInit, * bTemp, * bValue;
    Ntk_Latch_t * pLatch;
    Ntk_Node_t * pNode;
    DdNode ** pbCodes;
    int Value, i;

    // get the global variable map
    pVmxExt = Ntk_NetworkReadVmxGlo( pNet );

    // start the cube of init values
    bInit = b1;   Cudd_Ref( bInit );
    // go through the current state variables
    i = 0;
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        if ( pNode->Subtype != MV_BELONGS_TO_LATCH )
        {
            i++;
            continue;
        }
        // find the corresponding latch
        pLatch = NULL;
        Ntk_NetworkForEachLatch( pNet, pLatch )
            if ( pNode == Ntk_LatchReadOutput( pLatch ) )
                break;
        assert( pLatch );
        // get the reset value of this latch
        Value = Ntk_LatchReadReset( pLatch );
        // get the code of the corresponding variable of the map
        pbCodes = Vmx_VarMapEncodeVar( dd, pVmxExt, i );
        bValue = pbCodes[Value];    Cudd_Ref( bValue );
        Vmx_VarMapEncodeDeref( dd, pVmxExt, pbCodes );

        // add the code to the cube
        bInit = Cudd_bddAnd( dd, bTemp = bInit, bValue );  Cudd_Ref( bInit );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bValue );
        i++;
    }
    Cudd_Deref( bInit );
    return bInit;
}

/**Function*************************************************************

  Synopsis    [Computes the domain of one minterm per state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Lang_ExtractStgCodeDomain( DdManager * dd, Ntk_Network_t * pNet )
{
    Vmx_VarMap_t * pVmxExt;
    Ntk_Node_t * pNode;
    DdNode * bDomain, * bSubdomain, * bTemp;
    int i;

    // get the global variable map
    pVmxExt = Ntk_NetworkReadVmxGlo( pNet );

    // reduce the reachable states to only one minterm per state
    bDomain = b1;  Cudd_Ref( bDomain );
    i = 0;
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        if ( pNode->Subtype != MV_BELONGS_TO_LATCH )
        {
            i++;
            continue;
        }
        bSubdomain = Vmx_VarMapCharDomain( dd, pVmxExt, i );        Cudd_Ref( bSubdomain );
        bDomain = Cudd_bddAnd( dd, bTemp = bDomain, bSubdomain );   Cudd_Ref( bDomain );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bSubdomain );
        i++;
    }
    Cudd_Deref( bDomain );
    return bDomain;
}

/**Function*************************************************************

  Synopsis    [Creates the cube of the values of the given vars.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lang_ExtractStgLongNames( Lang_Auto_t * pAut, Ntk_Network_t * pNet )
{
    DdManager * dd = pAut->dd;
    DdNode * bTemp, * bFunc, * bMint, * bCubeCs;
    char Buffer[100], Digit[10];
    Vmx_VarMap_t * pVmxExt;
    Ntk_Node_t * pNode;
    int * pVarsCs, * pCodes;
    int iVar, i, iState;

    // get the global variable map
    pVmxExt = Ntk_NetworkReadVmxGlo( pNet );

    // collect variables as they appear in the global map
    pVarsCs = ALLOC( int, pAut->nLatches );
    pCodes  = ALLOC( int, pAut->nLatches );
    iVar = 0;
    i = 0;
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        if ( pNode->Subtype == MV_BELONGS_TO_LATCH )
            pVarsCs[iVar++] = i;
        i++;
    }

    // allocate room for state names
//    pAut->ppNamesState = ALLOC( char *, pAut->nStates + 1 );
    pAut->tCode2Name = st_init_table( st_ptrcmp, st_ptrhash );

    // go through the minterms in the natural order
    iState = 0;
    bFunc    = pAut->bStates;     Cudd_Ref( bFunc );
    bCubeCs  = Vmx_VarMapCharCubeRange( dd, pAut->pVmx, pAut->nInputs, pAut->nLatches ); Cudd_Ref( bCubeCs );
    for ( iState = 0; bFunc != b0; iState++ )
    {
        bMint = Extra_bddGetOneMinterm( dd, bFunc, bCubeCs ); Cudd_Ref( bMint );
        // decode the cube
        Vmx_VarMapDecodeCube( dd, bMint, pVmxExt, pVarsCs, pCodes, pAut->nLatches );
        // print symbolic state name
        Buffer[0] = 0;
        for ( i = 0; i < pAut->nLatches; i++ )
        {
            sprintf( Digit, "%d", pCodes[i] );
            strcat( Buffer, Digit );
        }
//        pAut->ppNamesState[iState] = util_strsav( Buffer );
        st_insert( pAut->tCode2Name, (char *)bMint, util_strsav(Buffer) ); // takes ref of bMint
        // subtract the minterm from the reachable states
        bFunc = Cudd_bddAnd( dd, bTemp = bFunc, Cudd_Not(bMint) );  Cudd_Ref( bFunc );
        Cudd_RecursiveDeref( dd, bTemp );
       // Cudd_RecursiveDeref( dd, bMint ); // the ref is taken!
    }
    assert( iState == pAut->nStates );
    Cudd_RecursiveDeref( dd, bFunc );
    Cudd_RecursiveDeref( dd, bCubeCs );
    FREE( pVarsCs );
    FREE( pCodes );
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



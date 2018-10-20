/**CFile****************************************************************

  FileName    [langProd.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Implicit language solving flow.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: langProd.c,v 1.4 2004/02/19 03:06:54 alanmi Exp $]

***********************************************************************/

#include "langInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Lang_ProductLongNames( Lang_Auto_t * pAut, Lang_Auto_t * pAut1, Lang_Auto_t * pAut2, int * pPermute, int fSwapped );
static int * Lang_ProductCreatePermutationArray( Lang_Auto_t * pAut, Lang_Auto_t * pAut2 );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes the product of two automata.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Lang_Auto_t * Lang_AutoProduct( Lang_Auto_t * pAut1, Lang_Auto_t * pAut2, bool fLongNames, bool fSimple )
{
    DdManager * dd;
    Lang_Auto_t * pAut, * pAutTemp;
    DdNode * bRel2, * bAcc2, * bInit2, * bStat2, * bTemp, * bStatesNs;
    int * pPermute;
    int iValues1, iValues2, i;
    int fSwapped, nBits;

    // if one of the automata is empty, return an empty automaton
    if ( pAut1->nStates == 0 )
    {
        printf( "Trying to make the product with an empty automaton.\n" );
        return Lang_AutoDup( pAut1 );
    }
    if ( pAut2->nStates == 0 )
    {
        printf( "Trying to make the product with an empty automaton.\n" );
        return Lang_AutoDup( pAut2 );
    }

    // make sure that the automata have the same names spaces
    if ( pAut1->nInputs != pAut2->nInputs )
    {
        printf( "To compute the product, the automata should have the same IO.\n" );
        printf( "The automata have different number of inputs.\n" );
        return NULL;
    }
    for ( i = 0; i < pAut1->nInputs; i++ )
    {
        iValues1 = Vm_VarMapReadValues( Vmx_VarMapReadVm(pAut1->pVmx), i );
        iValues2 = Vm_VarMapReadValues( Vmx_VarMapReadVm(pAut2->pVmx), i );
        if ( strcmp( pAut1->ppNamesInput[i], pAut2->ppNamesInput[i] ) )
        {
            printf( "To compute the product, the automata should have the same IO.\n" );
            printf( "The input #%d is different in the two automata: \"%s\" and \"%s\".\n", 
                i, pAut1->ppNamesInput[i], pAut2->ppNamesInput[i] );
            return NULL;
        }
        if ( iValues1 != iValues2 )
        {
            printf( "To compute the product, the automata should have the same IO.\n" );
            printf( "The numbers of values of input #%d, \"%s\", is different: %d and %d.\n", 
                i, pAut1->ppNamesInput[i], iValues1, iValues2 );
            return NULL;
        }
    }


    // the second automaton will be added to the first
    // swap the automata to make sure that the smaller one will be moved
    fSwapped = 0;
    if ( pAut1->nStateBits < pAut2->nStateBits )
    {
        fSwapped = 1;
        pAutTemp = pAut1;
        pAut1    = pAut2;
        pAut2    = pAutTemp;
    }

    // start the new automaton by duplicating the first one
    pAut = Lang_AutoDup( pAut1 );
    FREE( pAut->pName ); 
    pAut->pName = util_strsav( "prod" );

    // free the old state names if given
    Lang_AutoFreeStateNames( pAut->dd, pAut->tCode2Name );
    pAut->tCode2Name = NULL;

    // add new variables to the manager
    dd = pAut->dd;
//    Lang_AutoExtendManager( pAut, 0 );

    // set the new parameters
    pAut->nLatches   = pAut1->nLatches + pAut2->nLatches;
//    pAut->pVmx       = Lang_VarMapDupLatch( pAut1 );
    pAut->pVmx       = Lang_VarMapProduct( pAut, pAut1, pAut2, &pPermute );    
    pAut->nStateBits = pAut1->nStateBits + pAut2->nStateBits;
    pAut->nStates    = -1;

    // extend the manager (should be after Lang_VarMapProduct)
//    for ( i = 0; i < 2 * pAut2->nStateBits; i++ )
//        Cudd_bddNewVar( pAut->dd );
    nBits = Vmx_VarMapReadBitsNum( pAut->pVmx );
    for ( i = pAut->dd->size; i < nBits; i++ )
        Cudd_bddNewVar( pAut->dd );

    // create the permutation array
//    pPermute = Lang_ProductCreatePermutationArray( pAut, pAut2 );

    // move the BDDs from Aut2 into the new manager
    bInit2 = Extra_TransferPermute( pAut2->dd, dd, pAut2->bInit,      pPermute );   Cudd_Ref( bInit2 );
    bAcc2  = Extra_TransferPermute( pAut2->dd, dd, pAut2->bAccepting, pPermute );   Cudd_Ref( bAcc2 );
    bStat2 = Extra_TransferPermute( pAut2->dd, dd, pAut2->bStates,    pPermute );   Cudd_Ref( bStat2 );
    bRel2  = Extra_TransferPermute( pAut2->dd, dd, pAut2->bRel,       pPermute );   Cudd_Ref( bRel2 );

    // create Init, Accepting, Relation, States
//PRB( dd, pAut->bInit );
//PRB( dd, bInit2 );

    // create the products
    pAut->bInit = Cudd_bddAnd( dd, bTemp = pAut->bInit, bInit2 );          Cudd_Ref( pAut->bInit );
    Cudd_RecursiveDeref( dd, bTemp );
    Cudd_RecursiveDeref( dd, bInit2 );

    pAut->bAccepting = Cudd_bddAnd( dd, bTemp = pAut->bAccepting, bAcc2 ); Cudd_Ref( pAut->bAccepting );
    Cudd_RecursiveDeref( dd, bTemp );
    Cudd_RecursiveDeref( dd, bAcc2 );

    pAut->bStates = Cudd_bddAnd( dd, bTemp = pAut->bStates, bStat2 );      Cudd_Ref( pAut->bStates );
    Cudd_RecursiveDeref( dd, bTemp );
    Cudd_RecursiveDeref( dd, bStat2 );

    Cudd_AutodynEnable( dd, CUDD_REORDER_SYMM_SIFT );

    pAut->bRel = Cudd_bddAnd( dd, bTemp = pAut->bRel, bRel2 );             Cudd_Ref( pAut->bRel );
    Cudd_RecursiveDeref( dd, bTemp );
    Cudd_RecursiveDeref( dd, bRel2 );

    Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 100 );
    Cudd_AutodynDisable( dd );

    if ( fSimple )
    {
       FREE( pPermute );
       return pAut;
    }
    Lang_AutoVerifySupport( pAut );

    // perform the reachability the reachable states
    Lang_Reachability( pAut, 1, 0 );

    // update the number of reachable states
    pAut->nStates = (int)Cudd_CountMinterm( dd, pAut->bStates, pAut->nStateBits );

    // reduce everything to only reachable states
    bStatesNs = Vmx_VarMapRemapRange( dd, pAut->bStates, pAut->pVmx, 
        pAut->nInputs, pAut->nLatches, pAut->nInputs + pAut->nLatches, pAut->nLatches ); Cudd_Ref( bStatesNs );

    pAut->bRel = Cudd_bddAnd( dd, bTemp = pAut->bRel, pAut->bStates );  Cudd_Ref( pAut->bRel );
    Cudd_RecursiveDeref( dd, bTemp );
    pAut->bRel = Cudd_bddAnd( dd, bTemp = pAut->bRel, bStatesNs );      Cudd_Ref( pAut->bRel );
    Cudd_RecursiveDeref( dd, bTemp );

    Cudd_RecursiveDeref( dd, bStatesNs );

    // reduce the accepting states
    pAut->bAccepting = Cudd_bddAnd( dd, bTemp = pAut->bAccepting, pAut->bStates );  Cudd_Ref( pAut->bAccepting );
    Cudd_RecursiveDeref( dd, bTemp );

    // update the number of accepting states
    pAut->nAccepting = (int)Cudd_CountMinterm( dd, pAut->bAccepting, pAut->nStateBits );

//Vmx_VarMapPrint( pAut1->pVmx );
//Vmx_VarMapPrint( pAut2->pVmx );
//Vmx_VarMapPrint( pAut->pVmx );


    // if long names are requested, assign them for reachable states
    if ( fLongNames )
    {
        if ( pAut1->tCode2Name && pAut2->tCode2Name )
            Lang_ProductLongNames( pAut, pAut1, pAut2, pPermute, fSwapped );
        else if ( pAut1->tCode2Name == NULL )
            printf( "Warning: Cannot create long product names because Automaton 1 has no long names.\n" );
        else if ( pAut2->tCode2Name == NULL )
            printf( "Warning: Cannot create long product names because Automaton 2 has no long names.\n" );
    }
    FREE( pPermute );

/*
    ///////////////////////////////////////////
    // decide whether reencoding is necessary
    if ( 1 )
    {
        // perform reencoding of state bits
        p = Lang_AutoReencodePrepare( pAut );
        // remap stuff
        pAut->bInit = Lang_AutoReencodeOne( p, bTemp = pAut->bInit );            Cudd_Ref( pAut->bInit );
        Cudd_RecursiveDeref( dd, bTemp );
        pAut->bStates = Lang_AutoReencodeOne( p, bTemp = pAut->bStates );        Cudd_Ref( pAut->bStates );
        Cudd_RecursiveDeref( dd, bTemp );
        pAut->bAccepting = Lang_AutoReencodeOne( p, bTemp = pAut->bAccepting );  Cudd_Ref( pAut->bAccepting );
        Cudd_RecursiveDeref( dd, bTemp );
        // remap the relation
//    Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 100 );
//    Cudd_AutodynEnable( dd, CUDD_REORDER_SYMM_SIFT );
        pAut->bRel = Lang_AutoReencodeTwo( p, bTemp = pAut->bRel );              Cudd_Ref( pAut->bRel );
        Cudd_RecursiveDeref( dd, bTemp );

        // update the automaton after reencoding
        pAut->nLatches = 1;
        pAut->nStateBits = Extra_Base2Log( pAut->nStates + 1 );
        pAut->pVmx = Lang_AutoReencodeReadVmxNew( p );
        // remove the structures
        Lang_AutoReencodeCleanup( p );
    // reorder the global BDDs one more time
//    Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 100 );
    }
    ///////////////////////////////////////////
*/
    Lang_AutoVerifySupport( pAut );

    // because the input automata are complete, completion is not necessary
    return pAut;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lang_ProductLongNames( Lang_Auto_t * pAut, Lang_Auto_t * pAut1, Lang_Auto_t * pAut2, int * pPermute, int fSwapped )
{
    DdManager * dd = pAut->dd;
    DdNode * bTemp, * bFunc, * bMint, * bMint1, * bMint2;
    DdNode * bCubeCs, * bCubeCs1, * bCubeCs2;
    st_table * tCode2Name1, * tCode2Name2;
    char Buffer[1000];
    char * pName1, * pName2;
    int iState;

    // get the new tables
    tCode2Name1 = Lang_AutoTransferNamesTable( pAut1, dd, NULL );
    tCode2Name2 = Lang_AutoTransferNamesTable( pAut2, dd, pPermute );

//pAut->tCode2Name = tCode2Name1;
//Lang_AutoPrintNameTable( pAut );
//pAut->tCode2Name = tCode2Name2;
//Lang_AutoPrintNameTable( pAut );

    // allocate room for state names
    pAut->tCode2Name = st_init_table( st_ptrcmp, st_ptrhash );

    // go through the minterms in the natural order
    iState = 0;
    bFunc    = pAut->bStates;     Cudd_Ref( bFunc );
    bCubeCs  = Vmx_VarMapCharCubeRange( dd, pAut->pVmx, pAut->nInputs, pAut->nLatches );  Cudd_Ref( bCubeCs );
    bCubeCs1 = Vmx_VarMapCharCubeRange( dd, pAut->pVmx, pAut->nInputs, pAut1->nLatches ); Cudd_Ref( bCubeCs1 );
    bCubeCs2 = Vmx_VarMapCharCubeRange( dd, pAut->pVmx, pAut->nInputs + pAut1->nLatches, pAut2->nLatches ); Cudd_Ref( bCubeCs2 );
    for ( iState = 0; bFunc != b0; iState++ )
    {
        // extract one minterm from the encoded state
        bMint  = Extra_bddGetOneMinterm( dd, bFunc, bCubeCs );  Cudd_Ref( bMint );
        bMint1 = Cudd_bddExistAbstract( dd, bMint, bCubeCs2 );  Cudd_Ref( bMint1 );
        bMint2 = Cudd_bddExistAbstract( dd, bMint, bCubeCs1 );  Cudd_Ref( bMint2 );
        // get the current state code
        if ( !st_lookup( tCode2Name1, (char *)bMint1, (char **)&pName1 ) )
        {
            Lang_AutoFreeStateNames( dd, pAut->tCode2Name );
            pAut->tCode2Name = NULL;
            // deref the functions
            Cudd_RecursiveDeref( dd, bMint );
            Cudd_RecursiveDeref( dd, bMint1 );
            Cudd_RecursiveDeref( dd, bMint2 );
            printf( "Cannot assign long names to the product automaton because of bug #135.\n" );
            goto QUITS;
        }

        // get the current state code
        if ( !st_lookup( tCode2Name2, (char *)bMint2, (char **)&pName2 ) )
        {
            Lang_AutoFreeStateNames( dd, pAut->tCode2Name );
            pAut->tCode2Name = NULL;
            // deref the functions
            Cudd_RecursiveDeref( dd, bMint );
            Cudd_RecursiveDeref( dd, bMint1 );
            Cudd_RecursiveDeref( dd, bMint2 );
            printf( "Cannot assign long names to the product automaton because of bug #275.\n" );
            goto QUITS;
        }
        Cudd_RecursiveDeref( dd, bMint2 );
        Cudd_RecursiveDeref( dd, bMint1 );

        // print symbolic state name
        if ( !fSwapped )
            sprintf( Buffer, "%s%s", pName1, pName2 );
        else
            sprintf( Buffer, "%s%s", pName2, pName1 );
//        pAut->ppNamesState[iState] = util_strsav( Buffer );
        st_insert( pAut->tCode2Name, (char *)bMint, util_strsav(Buffer) ); // takes ref of bMint
//printf( "Producing name %s.\n", Buffer );

        // subtract the minterm from the reachable states
        bFunc = Cudd_bddAnd( dd, bTemp = bFunc, Cudd_Not(bMint) );  Cudd_Ref( bFunc );
        Cudd_RecursiveDeref( dd, bTemp );
        // Cudd_RecursiveDeref( dd, bMint ); // the ref is taken
    }
    assert( iState == pAut->nStates );
    Lang_AutoFreeStateNames( dd, tCode2Name1 );
    Lang_AutoFreeStateNames( dd, tCode2Name2 );

QUITS:
    Cudd_RecursiveDeref( dd, bFunc );
    Cudd_RecursiveDeref( dd, bCubeCs );
    Cudd_RecursiveDeref( dd, bCubeCs1 );
    Cudd_RecursiveDeref( dd, bCubeCs2 );
}

/**Function*************************************************************

  Synopsis    [Permutes the binary bits from the second automaton to the new automaton.]

  Description [ONLY WORKS FOR 1 LATCH!!!]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int * Lang_ProductCreatePermutationArray( Lang_Auto_t * pAut, Lang_Auto_t * pAut2 )
{
    int * pBits2, * pBitsFirst2, * pBitsOrder2;
    int * pBitsN, * pBitsFirstN, * pBitsOrderN;
    int * pTransfer, * pPermute;
    int i, v, b, index;

    // find the variable map mapping the second automaton into the product automaton
    pTransfer = ALLOC( int, pAut->nInputs + pAut->nLatches + pAut2->nLatches );
    for ( i = 0; i < pAut2->nInputs; i++ )
        pTransfer[i] = i;
    // map the last two vars with a shift
    pTransfer[pAut2->nInputs + 0] = pAut->nInputs + 1;
    pTransfer[pAut2->nInputs + 1] = pAut->nInputs + 3;
    
    // get the bits arrays for the second map
    pBits2      = Vmx_VarMapReadBits( pAut2->pVmx );
    pBitsFirst2 = Vmx_VarMapReadBitsFirst( pAut2->pVmx );
    pBitsOrder2 = Vmx_VarMapReadBitsOrder( pAut2->pVmx );
    // get the bits arrays for the new map
    pBitsN      = Vmx_VarMapReadBits( pAut->pVmx );
    pBitsFirstN = Vmx_VarMapReadBitsFirst( pAut->pVmx );
    pBitsOrderN = Vmx_VarMapReadBitsOrder( pAut->pVmx );

    // create the permutation
    pPermute = ALLOC( int, pAut->dd->size );
    for ( v = 0; v < pAut2->nInputs + 2; v++ )
    {
        assert( pBits2[v] <= pBitsN[v] );
        for ( b = 0; b < pBits2[v]; b++ )
        {
            index = pBitsOrder2[ pBitsFirst2[v] + b ];
            pPermute[index] = pBitsOrderN[ pBitsFirstN[pTransfer[v]] + b ];
        }
    }
    FREE( pTransfer );
    return pPermute;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



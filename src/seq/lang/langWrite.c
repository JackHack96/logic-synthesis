/**CFile****************************************************************

  FileName    [langWrite.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Implicit language solving flow.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: langWrite.c,v 1.5 2004/02/19 03:06:55 alanmi Exp $]

***********************************************************************/

#include "langInt.h"
#include "auInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Au_Auto_t * Lang_AutomatonDerive( Lang_Auto_t * pLang );

extern int Extra_bddMintermNum( DdManager * dd, DdNode * bMint );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lang_AutoWrite( Lang_Auto_t * pLang, char * FileName )
{
    Au_Auto_t * pAut;
    if ( pLang->nStates == 0 )
        printf( "Warning: The automaton written into file \"%s\" is empty.\n", FileName );
    pAut = Lang_AutomatonDerive( pLang );
    Au_AutoWriteKiss( pAut, FileName );
    Au_AutoFree( pAut );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Au_Auto_t * Lang_AutomatonDerive( Lang_Auto_t * pLang )
{
    ProgressBar * pProgress;
    Au_Auto_t * pAut;
    Au_Trans_t * pTrans;
    DdManager * dd = pLang->dd;
    DdNode * bRel, * bRelSt, * bTemp, * bCond, * bMintNs, * bMintCs;
    DdNode * bCubeCs, * bCubeNs, * bCode, * bFunc;
    st_table * tCode2Num;
    char ** ppStateNames;
    DdNode ** pbStateCodes;
    char StateName[100];
    int i, k, s, iInit, nDigits, iNext;
    int fBinary, nInputBits, * pBits, iBit;
    Vm_VarMap_t * pVmNew;
    Vmx_VarMap_t * pVmxIO;

    if ( pLang->nStates > 1000 )
        printf( "The number of states is %d. Writing .aut file may be slow.\n", pLang->nStates );

    // check whether the problem is binary
    nInputBits = Vmx_VarMapBitsNumRange( pLang->pVmx, 0, pLang->nInputs );
    fBinary = (int)( pLang->nInputs == nInputBits );
    
    // create ZDD variables for writing the covers
    Cudd_zddVarsFromBddVars( dd, 2 );

    // start the automaton
    pAut = ALLOC( Au_Auto_t, 1 );
    memset( pAut, 0, sizeof(Au_Auto_t) );
    pAut->pName        = util_strsav( pLang->pName );
    pAut->pMan         = pLang->pMan;
    pAut->nInputs      = nInputBits;
    pAut->nOutputs     = pLang->nOutputs;
    pAut->nStates      = pLang->nStates;
    pAut->nStatesAlloc = pLang->nStates;

    // copy the variable and state names
    if ( fBinary )
    {
        pAut->ppNamesInput = ALLOC( char *, nInputBits );
        for ( i = 0; i < nInputBits; i++ )
            pAut->ppNamesInput[i] = util_strsav( pLang->ppNamesInput[i] );
        // assign the map to be used for the computation of SOP covers
        pVmxIO = pLang->pVmx;
    }
    else
    {
        pBits = Vmx_VarMapReadBits( pLang->pVmx );
        pAut->ppNamesInput = ALLOC( char *, nInputBits );
        iBit = 0;
        for ( i = 0; i < pLang->nInputs; i++ )
            for ( k = 0; k < pBits[i]; k++ )
            {
                sprintf( StateName, "%s_%d", pLang->ppNamesInput[i], k );
                pAut->ppNamesInput[iBit++] = util_strsav( StateName );
            }
        assert( iBit == nInputBits );
        // create the map to be used for the computation of SOP covers
        pVmNew = Vm_VarMapCreateBinary( Fnc_ManagerReadManVm(pLang->pMan), nInputBits, 0 );
        pVmxIO = Vmx_VarMapLookup( Fnc_ManagerReadManVmx(pLang->pMan), 
            pVmNew, nInputBits, Vmx_VarMapReadBitsOrder(pLang->pVmx) );
    }
//    pAut->ppNamesOutput = ALLOC( char *, pLang->nOutputs );
//    for ( i = 0; i < pLang->nOutputs; i++ )
//        pAut->ppNamesOutput[i] = util_strsav( pLang->ppNamesOutput[i] );
    // state names will be added later

    if ( pLang->nStates == 0 )
        return pAut;

    // get the char cubes
    bCubeCs  = Lang_AutoCharCubeCs( dd, pLang );          Cudd_Ref( bCubeCs );
    bCubeNs  = Lang_AutoCharCubeNs( dd, pLang );          Cudd_Ref( bCubeNs );
//PRB( dd, bCubeCs );
//PRB( dd, bCubeNs );

    // collect the state names into one array
    ppStateNames = ALLOC( char *, pLang->nStates );
    pbStateCodes = ALLOC( DdNode *, pLang->nStates );
    if ( pLang->tCode2Name )
    {
        st_generator * gen;
        char * pName;
        assert( pLang->nStates == st_count( pLang->tCode2Name ) );
        i = 0;
        st_foreach_item( pLang->tCode2Name, gen, (char **)&bCode, &pName )
        {
            pbStateCodes[i] = bCode;   Cudd_Ref( pbStateCodes[i] );
            ppStateNames[i] = pName;
            i++;
        }
    }
    else
    {
        // create the table of codes
        bFunc = pLang->bStates;  Cudd_Ref( bFunc );
        for ( i = 0; bFunc != b0; i++ )
        {
            // extract one minterm
            bMintCs = Extra_bddGetOneMinterm( dd, bFunc, bCubeCs );        Cudd_Ref( bMintCs );
//PRB( dd, bMintCs );
            // save the minterm
            pbStateCodes[i] = bMintCs;   Cudd_Ref( pbStateCodes[i] );
            // subtract this minterm from the relation
            bFunc = Cudd_bddAnd( dd, bTemp = bFunc, Cudd_Not(bMintCs) );   Cudd_Ref( bFunc );
            Cudd_RecursiveDeref( dd, bTemp );
            Cudd_RecursiveDeref( dd, bMintCs );
        }
        Cudd_RecursiveDeref( dd, bFunc );
    }
    assert( i == pLang->nStates );

    // set up the mapping of codes into their numbers
    tCode2Num = st_init_table( st_ptrcmp, st_ptrhash );
    for ( i = 0; i < pLang->nStates; i++ )
        st_insert( tCode2Num, (char *)pbStateCodes[i], (char *)i );


    // find the initial state
    for ( iInit = 0; iInit < pLang->nStates; iInit++ )
        if ( !Cudd_bddLeq( dd, pbStateCodes[iInit], Cudd_Not(pLang->bInit) ) )
            break;
    assert( iInit < pLang->nStates );

    // get the number of digits in the state number
    nDigits = Extra_Base10Log( pLang->nStates );

    // set up the internal variable replacement map: CS <-> NS
    Vmx_VarMapRemapRangeSetup( dd, pLang->pVmx, 
        pLang->nInputs + 0, pLang->nLatches, pLang->nInputs + pLang->nLatches, pLang->nLatches ); 

    // start the progress bar
    pProgress = Extra_ProgressBarStart( stdout, pLang->nStates );

    // create the states
    bRel = pLang->bRel;   Cudd_Ref( bRel );
    pAut->pStates = ALLOC( Au_State_t *, pAut->nStatesAlloc );
    for ( s = 0; s < pLang->nStates; s++ )
    {
        if ( s == 0 )
            i = iInit;
        else if ( s == iInit )
            i = 0;
        else
            i = s;

        // get the new state
        pAut->pStates[s] = Au_AutoStateAlloc();
        // set the state's name
        if ( pLang->tCode2Name )
            pAut->pStates[s]->pName = util_strsav( ppStateNames[i] );
        else
        {
            sprintf( StateName, "s%0*d", nDigits, s );
            pAut->pStates[s]->pName = util_strsav( StateName );
        }
        // set the state's acc attribute
        pAut->pStates[s]->fAcc = !Cudd_bddLeq( dd, pbStateCodes[i], Cudd_Not(pLang->bAccepting) );

        // get the transitions for this state
        bRelSt = Cudd_bddAndAbstract( dd, bRel, pbStateCodes[i], bCubeCs );   Cudd_Ref( bRelSt );

        // reduce the total transition relation
//        bRel   = Cudd_bddAnd( dd, bTemp = bRel, Cudd_Not(pbStateCodes[i]) );  Cudd_Ref( bRel );
//        Cudd_RecursiveDeref( dd, bTemp );
        // when these lines are commented out, the computation seems to be faster...
        // iterate through the transitions of this state
        while ( bRelSt != b0 )
        {
            // extract one minterm
            bMintNs = Extra_bddGetOneMinterm( dd, bRelSt, bCubeNs );        Cudd_Ref( bMintNs );
//PRB( dd, bMintNs );
            // tranfer this minterm to the CS var
            bMintCs = Cudd_bddVarMap( dd, bMintNs );                        Cudd_Ref( bMintCs );
//printf( "\n" );
//PRB( dd, bMintCs );
            // get the number of this next state
            if ( !st_lookup( tCode2Num, (char *)bMintCs, (char **)&iNext ) )
            {
                assert( 0 );
            }
            Cudd_RecursiveDeref( dd, bMintCs );

            if ( iNext == 0 )
                k = iInit;
            else if ( iNext == iInit )
                k = 0;
            else
                k = iNext;

            // get the condition
            bCond = Cudd_bddAndAbstract( dd, bRelSt, bMintNs, bCubeNs ); Cudd_Ref( bCond );  

            // create the transition
            pTrans = Au_AutoTransAlloc();
            pTrans->pCond = Fnc_FunctionDeriveSopFromMddSpecial( 
                Fnc_ManagerReadManMvc(pLang->pMan), dd, bCond, bCond, pVmxIO, nInputBits );
            pTrans->StateCur  = s;
            pTrans->StateNext = k;
            // add the transition
            Au_AutoTransAdd( pAut->pStates[s], pTrans );
            Cudd_RecursiveDeref( dd, bCond );

            // subtract this minterm from the relation
            bRelSt = Cudd_bddAnd( dd, bTemp = bRelSt, Cudd_Not(bMintNs) );    Cudd_Ref( bRelSt );
            Cudd_RecursiveDeref( dd, bTemp );
            Cudd_RecursiveDeref( dd, bMintNs );
        }
        Cudd_RecursiveDeref( dd, bRelSt );

        if ( s && (s % 30 == 0) )
        {
            char Buffer[100];
            sprintf( Buffer, "%d states", s );
            Extra_ProgressBarUpdate( pProgress, s, Buffer );
        }
    }
    Extra_ProgressBarStop( pProgress );

    Cudd_RecursiveDeref( dd, bRel );
    Cudd_RecursiveDeref( dd, bCubeCs );
    Cudd_RecursiveDeref( dd, bCubeNs );

    for ( i = 0; i < pLang->nStates; i++ )
        Cudd_RecursiveDeref( dd, pbStateCodes[i] );
    FREE( pbStateCodes );
    FREE( ppStateNames );
    st_free_table( tCode2Num );

    return pAut;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



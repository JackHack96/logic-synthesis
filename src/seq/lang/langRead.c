/**CFile****************************************************************

  FileName    [langRead.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Implicit language solving flow.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: langRead.c,v 1.4 2004/02/19 03:06:54 alanmi Exp $]

***********************************************************************/

#include "langInt.h"
#include "auInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Procedures to read the automaton KISS file.]

  Description [Completes the automaton if initially it was not complete.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Lang_Auto_t * Lang_AutoRead( Mv_Frame_t * pMvsis, char * FileName )
{
    FILE * pFile, * pErr;
    Au_Auto_t * pAut;
    Lang_Auto_t * pLang;

    // check that the input file is okay
    pErr = Mv_FrameReadErr(pMvsis);
    if ( (pFile = fopen( FileName, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\".\n", FileName );
        return NULL;
    }
    fclose( pFile );

    pAut = Au_AutoReadKiss( pMvsis, FileName );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileName );
        return NULL;
    }

    pLang = Lang_AutomatonConvert( pAut, NULL );
    Au_AutoFree( pAut );
    return pLang;
}

/**Function*************************************************************

  Synopsis    [Convert the automaton from the two-level representation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Lang_Auto_t * Lang_AutomatonConvert( Au_Auto_t * pAut, DdManager * ddUser )
{
    ProgressBar * pProgress;
    Au_Trans_t * pTrans;
    Lang_Auto_t * pLang;
    Vm_VarMap_t * pVm;
    Vmx_VarMap_t * pVmx;
    DdManager * dd;
    DdNode * bProd, * bTemp;
    DdNode ** pbCodes, ** pbCodesCs, ** pbCodesNs;
    int * pValues;
    int i, v, nBitsIO, nBits;
    int fComplete = 1;

    // create the variable map
    pValues = ALLOC( int, pAut->nInputs + 2 );
    for ( v = 0; v < pAut->nInputs; v++ )
        pValues[v] = 2;
    pValues[pAut->nInputs  ] = pAut->nStates + 1; // reserve one state for possible completion
    pValues[pAut->nInputs+1] = pAut->nStates + 1;
    pVmx = Vmx_VarMapCreateSimple( Fnc_ManagerReadManVm(pAut->pMan), 
        Fnc_ManagerReadManVmx(pAut->pMan), pAut->nInputs, 2, pValues );
    pVm = Vmx_VarMapReadVm( pVmx );
    FREE( pValues );

    // start the automaton
    pLang = Lang_AutoAlloc();
    pLang->pName      = util_strsav( pAut->pName );
    pLang->nInputs    = pAut->nInputs;
    pLang->nOutputs   = pAut->nOutputs;
    pLang->nLatches   = 1;
    pLang->nStates    = pAut->nStates;
    pLang->nStateBits = Extra_Base2Log( pLang->nStates + 1 );

    // variable names
    if ( pAut->ppNamesInput )
    {
        pLang->ppNamesInput = ALLOC( char *, pAut->nInputs );
        for ( i = 0; i < pAut->nInputs; i++ )
            pLang->ppNamesInput[i] = util_strsav( pAut->ppNamesInput[i] );
    }
//    if ( pAut->ppNamesOutput )
//    {
//        pLang->ppNamesOutput = ALLOC( char *, pAut->nOutputs );
//        for ( i = 0; i < pAut->nOutputs; i++ )
//            pLang->ppNamesOutput[i] = util_strsav( pAut->ppNamesOutput[i] );
//    }

    // functionality
    pLang->pMan = pAut->pMan;
    pLang->pVmx = pVmx;
    nBitsIO = Vmx_VarMapBitsNumRange( pLang->pVmx, 0, pLang->nInputs );
    nBits   = Vmx_VarMapReadBitsNum( pLang->pVmx );
    if ( ddUser )
        pLang->dd = dd = ddUser;
    else  // create the new manager with the IO bits
        pLang->dd = dd = Cudd_Init( nBitsIO, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    if ( dd->size == nBitsIO )
    {
        // add the current and next states, interleaves
        for ( i = 0; i < pLang->nStateBits; i++ )
            Cudd_bddNewVar( dd );    
        for ( i = 0; i < pLang->nStateBits; i++ )
            Cudd_bddNewVarAtLevel( dd, dd->size - (pLang->nStateBits - 1 - i) );    
//    for ( i = 0; i < ddNew->size; i++ )
//        printf( "Level = %2d. Var = %2d.\n", i, ddNew->invperm[i] );
    }
    else if ( dd->size < nBits )
    { // simply add the needed bits at the end
        for ( i = dd->size; i < nBits; i++ )
            Cudd_bddNewVar( dd );    
    }

    // derive the codes
    pbCodes   = Vmx_VarMapEncodeMapMinterm( dd, pLang->pVmx );
    pbCodesCs = pbCodes + Vm_VarMapReadValuesFirst( pVm, pAut->nInputs + 0 );
    pbCodesNs = pbCodes + Vm_VarMapReadValuesFirst( pVm, pAut->nInputs + 1 );

    // add the state names
    if ( pAut->pStates[0]->pName )
    {
        DdNode * bCode;
        char * pName;
        pLang->tCode2Name = st_init_table( st_ptrcmp, st_ptrhash );
        for ( i = 0; i < pAut->nStates; i++ )
        {
            bCode = pbCodesCs[i];  Cudd_Ref( bCode );
            pName = util_strsav( pAut->pStates[i]->pName );
            st_insert( pLang->tCode2Name, (char *)bCode, pName );
        }
    }

    // set the code of the initial state
    pLang->bInit = pbCodesCs[0];   Cudd_Ref( pLang->bInit );

    // set the accepting states
    pLang->nAccepting = 0;
    pLang->bAccepting = b0;  Cudd_Ref( pLang->bAccepting );
    pLang->bStates    = b0;  Cudd_Ref( pLang->bStates );
    for ( i = 0; i < pAut->nStates; i++ )
        if ( pAut->pStates[i]->fAcc )
        {
            // add this product
            pLang->bAccepting = Cudd_bddOr( dd, bTemp = pLang->bAccepting, pbCodesCs[i] );  Cudd_Ref( pLang->bAccepting );
            Cudd_RecursiveDeref( dd, bTemp );
            pLang->nAccepting++;
        }
        else
        {
            pLang->bStates = Cudd_bddOr( dd, bTemp = pLang->bStates, pbCodesCs[i] );  Cudd_Ref( pLang->bStates );
            Cudd_RecursiveDeref( dd, bTemp );
        }
    // add the accepting to the set of all states
    pLang->bStates = Cudd_bddOr( dd, bTemp = pLang->bStates, pLang->bAccepting );    Cudd_Ref( pLang->bStates );
    Cudd_RecursiveDeref( dd, bTemp );

    // reorder
    Cudd_AutodynEnable( dd, CUDD_REORDER_SYMM_SIFT );

    // start the progress bar
    pProgress = Extra_ProgressBarStart( stdout, pAut->nStates );
    // derive the transition relation
    pLang->bRel = b0;  Cudd_Ref( pLang->bRel );
    for ( i = 0; i < pAut->nStates; i++ )
    {
        Au_StateForEachTransition( pAut->pStates[i], pTrans )
        {
            if ( pTrans->pCond == NULL ) // the condition is given as a BDD
            {
                bProd = pTrans->bCond;  // takes ref
                pTrans->bCond = NULL;
            }
            else
            {
                bProd = Fnc_FunctionDeriveMddFromSop( dd, pVm, pTrans->pCond, pbCodes );   
                Cudd_Ref( bProd );
            }
            bProd = Cudd_bddAnd( dd, bTemp = bProd, pbCodesCs[pTrans->StateCur] );     Cudd_Ref( bProd );
            Cudd_RecursiveDeref( dd, bTemp );
            bProd = Cudd_bddAnd( dd, bTemp = bProd, pbCodesNs[pTrans->StateNext] );    Cudd_Ref( bProd );
            Cudd_RecursiveDeref( dd, bTemp );
            // add this product
            pLang->bRel = Cudd_bddOr( dd, bTemp = pLang->bRel, bProd );                Cudd_Ref( pLang->bRel );
            Cudd_RecursiveDeref( dd, bTemp );
            Cudd_RecursiveDeref( dd, bProd );
        }
        if ( i && (i % 30 == 0) )
        {
            char Buffer[100];
            sprintf( Buffer, "%d states", i );
            Extra_ProgressBarUpdate( pProgress, i, Buffer );
        }
    }
    Extra_ProgressBarStop( pProgress );
/*
    // complete the automaton if necessary
    if ( fComplete && Lang_AutoComplete( pLang ) )
    { // completion happened
      // increment the number of states
        pLang->nStates++;
        assert( pLang->nStates == pAut->nStates + 1 );
    }
*/

    Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 100 );
    Cudd_AutodynDisable( dd );

    Vmx_VarMapEncodeDeref( dd, pLang->pVmx, pbCodes );
    return pLang;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



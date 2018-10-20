/**CFile****************************************************************

  FileName    [langUtils.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Implicit language solving flow.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: langUtils.c,v 1.4 2004/02/19 03:06:55 alanmi Exp $]

***********************************************************************/

#include "langInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates the automaton structure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Lang_Auto_t * Lang_AutoAlloc()
{
    Lang_Auto_t * pLang;
    pLang = ALLOC( Lang_Auto_t, 1 );
    memset( pLang, 0, sizeof(Lang_Auto_t) );
    return pLang;
}

/**Function*************************************************************

  Synopsis    [Creates the automaton with the identical IO names.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Lang_Auto_t * Lang_AutoClone( Lang_Auto_t * pAut )
{
    Lang_Auto_t * pLang;
    int i;
    pLang = Lang_AutoAlloc();
    pLang->nInputs    = pAut->nInputs;
    pLang->nOutputs   = pAut->nOutputs;
    pLang->nLatches   = pAut->nLatches;
//    pLang->nStates    = pAut->nStates;
//    pLang->nAccepting = pAut->nAccepting;
//    pLang->nStateBits = pAut->nStateBits;
    pLang->pMan       = pAut->pMan;
    pLang->pName      = util_strsav( pAut->pName );
    // copy the variable names
    pLang->ppNamesInput = ALLOC( char *, pAut->nInputs );
    for ( i = 0; i < pAut->nInputs; i++ )
        pLang->ppNamesInput[i] = util_strsav( pAut->ppNamesInput[i] );
//    pLang->ppNamesOutput = ALLOC( char *, pAut->nOutputs );
//    for ( i = 0; i < pAut->nOutputs; i++ )
//        pLang->ppNamesOutput[i] = util_strsav( pAut->ppNamesOutput[i] );
    return pLang;
}

/**Function*************************************************************

  Synopsis    [Duplicates the automaton.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Lang_Auto_t * Lang_AutoDup( Lang_Auto_t * pAut )
{
    DdManager * dd;
    Lang_Auto_t * pLang;

    // copy parameters and IO names
    pLang = Lang_AutoClone( pAut );
    // copy the genesis info
    if ( pAut->pHistory )
        pLang->pHistory = util_strsav( pAut->pHistory );
    // copy the information related to the states
    pLang->nStates    = pAut->nStates;
    pLang->nAccepting = pAut->nAccepting;
    pLang->nStateBits = pAut->nStateBits;
    // copy other stuff
    pLang->pMan = pAut->pMan;
    pLang->pVmx = pAut->pVmx;
    // create the new manager with the same ordering of variables
    pLang->dd = dd = Cudd_Init( pAut->dd->size, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    Cudd_ShuffleHeap( dd, pAut->dd->invperm );
    // transfer the BDDs
    if ( pAut->bRel )
    {
        pLang->bRel = Cudd_bddTransfer( pAut->dd, dd, pAut->bRel );  
        Cudd_Ref( pLang->bRel );
    }
    if ( pAut->bInit )
    {
        pLang->bInit = Cudd_bddTransfer( pAut->dd, dd, pAut->bInit );  
        Cudd_Ref( pLang->bInit );
    }
    if ( pAut->bStates )
    {
        pLang->bStates = Cudd_bddTransfer( pAut->dd, dd, pAut->bStates );  
        Cudd_Ref( pLang->bStates );
    }
    if ( pAut->bAccepting )
    {
        pLang->bAccepting = Cudd_bddTransfer( pAut->dd, dd, pAut->bAccepting );  
        Cudd_Ref( pLang->bAccepting );
    }
    // copy state names if available
    if ( pAut->tCode2Name )
    {
        st_generator * gen;
        DdNode * bCodeOld, * bCodeNew;
        char * pNameOld, * pNameNew;
        pLang->tCode2Name = st_init_table( st_ptrcmp, st_ptrhash );
        // go through all the entries in the old table and create the new table
        st_foreach_item( pAut->tCode2Name, gen, (char **)&bCodeOld, &pNameOld )
        {
            bCodeNew = Cudd_bddTransfer( pAut->dd, dd, bCodeOld );  Cudd_Ref( bCodeNew );
            pNameNew = util_strsav( pNameOld );
            st_insert( pLang->tCode2Name, (char *)bCodeNew, pNameNew );
        }
    }
    return pLang;
}

/**Function*************************************************************

  Synopsis    [Deallocates the automaton.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lang_AutoFree( Lang_Auto_t * pAut )
{
    DdManager * dd = pAut->dd;
    int i;
    // dereference the BDDs
    if ( pAut->bRel )
        Cudd_RecursiveDeref( dd, pAut->bRel );
    if ( pAut->bInit )
        Cudd_RecursiveDeref( dd, pAut->bInit );
    if ( pAut->bStates )
        Cudd_RecursiveDeref( dd, pAut->bStates );
    if ( pAut->bAccepting )
        Cudd_RecursiveDeref( dd, pAut->bAccepting );
    // free the state names if given
    Lang_AutoFreeStateNames( dd, pAut->tCode2Name );
    // quite the manager associated with the automaton
    Extra_StopManager( dd );
    // free the input/output and state names
    for ( i = 0; i < pAut->nInputs; i++ )
        FREE( pAut->ppNamesInput[i] );
//    for ( i = 0; i < pAut->nOutputs; i++ )
//        FREE( pAut->ppNamesOutput[i] );
    FREE( pAut->ppNamesInput );
    FREE( pAut->ppNamesOutput );
    FREE( pAut->pName );
    FREE( pAut->pHistory );
    FREE( pAut );
}

/**Function*************************************************************

  Synopsis    [Free state names.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lang_AutoFreeStateNames( DdManager * dd, st_table * tCode2Name )
{
    st_generator * gen;
    DdNode * bCode;
    char * pName;
    if ( tCode2Name == NULL )
        return;
    // remove all state names produced so far
    st_foreach_item( tCode2Name, gen, (char **)&bCode, &pName )
    {
        Cudd_RecursiveDeref( dd, bCode );
        FREE( pName );
    }
    st_free_table( tCode2Name );
}

/**Function*************************************************************

  Synopsis    [Counts transitions in the automaton.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lang_AutoCountTransitions( Lang_Auto_t * pAut )
{
    DdManager * dd = pAut->dd;
    DdNode * bQuant, * bCubeIO;
    double Res;
    // quantify the input/output variables
    bCubeIO = Vmx_VarMapCharCubeRange( dd, pAut->pVmx, 0, pAut->nInputs );  Cudd_Ref( bCubeIO );
    bQuant  = Cudd_bddExistAbstract( dd, pAut->bRel, bCubeIO );             Cudd_Ref( bQuant ); 
    Cudd_RecursiveDeref( dd, bCubeIO );
    // count the number of minterms depending on CS/NS vars
    Res = Cudd_CountMinterm( dd, bQuant, 2 * pAut->nStateBits );
    Cudd_RecursiveDeref( dd, bQuant );
    return (int)Res;
}


/**Function*************************************************************

  Synopsis    [Extends the BDD manager to contain additional variables.]

  Description [The additional variables are composed of the additional
  IO variables (nAddInputs) and the additional CS2/NS2 variables.
  The former variables are added at the bottom. The latter are interleaves 
  with the current CS/NS variables.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lang_AutoExtendManager( Lang_Auto_t * pAut, int nAddInputBits )
{
    DdManager * dd = pAut->dd;
    DdNode ** pbVarsCs, ** pbVarsNs;
    int nBits1, nBits2, v;

    // first add the additional IO variables at the bottom
    assert( nAddInputBits >= 0 );
    if ( nAddInputBits )
        for ( v = 0; v < nAddInputBits; v++ )
            Cudd_bddNewVar( dd );
    // prepare to interleave the new CS/NS variables with the old ones
    // get the arrays of the current CS and NS variables
    pbVarsCs = Vmx_VarMapBinVarsRange( dd, pAut->pVmx, pAut->nInputs +              0, pAut->nLatches );
    pbVarsNs = Vmx_VarMapBinVarsRange( dd, pAut->pVmx, pAut->nInputs + pAut->nLatches, pAut->nLatches );
    // get the number of bits in these variables
    nBits1 = Vmx_VarMapBitsNumRange( pAut->pVmx, pAut->nInputs +              0, pAut->nLatches );
    nBits2 = Vmx_VarMapBitsNumRange( pAut->pVmx, pAut->nInputs + pAut->nLatches, pAut->nLatches );
    assert( nBits1 == nBits2 );
    // first, interleave new CS variables
    for ( v = 0; v < nBits1; v++ )
        Cudd_bddNewVarAtLevel( dd, pAut->dd->perm[pbVarsCs[v]->index] );
    // next, interleave new NS variables
    for ( v = 0; v < nBits2; v++ )
        Cudd_bddNewVarAtLevel( dd, pAut->dd->perm[pbVarsNs[v]->index] );
    FREE( pbVarsCs );
    FREE( pbVarsNs );
}

/**Function*************************************************************

  Synopsis    [Returns the code of the given state using CS or NS vars.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Lang_AutoGetStateCode( Lang_Auto_t * pAut, int iState, int fUseNsVars )
{
    DdManager * dd = pAut->dd;
    DdNode ** pbVars;
    DdNode * bCube;

    // get the varaibles to encode the cube
    pbVars = Vmx_VarMapBinVarsVar( dd, pAut->pVmx, pAut->nInputs + fUseNsVars );
    // compute the cube
    bCube = Extra_bddBitsToCube2( dd, iState, pAut->nStateBits, pbVars );  Cudd_Ref( bCube );
    FREE( pbVars );
    Cudd_Deref( bCube );
    return bCube;
}

/**Function*************************************************************

  Synopsis    [Get unused code minterm.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Lang_AutoGetUnusedCodeMinterm( Lang_Auto_t * pAut )
{
    DdManager * dd = pAut->dd;
    DdNode * bCubeCs, * bMint;

    bCubeCs = Lang_AutoCharCubeCs( dd, pAut );                                Cudd_Ref( bCubeCs );
    bMint   = Extra_bddGetOneMinterm( dd, Cudd_Not(pAut->bStates), bCubeCs ); Cudd_Ref( bMint );
    Cudd_RecursiveDeref( dd, bCubeCs );
    Cudd_Deref( bMint );
    return bMint;
}

/**Function*************************************************************

  Synopsis    [Makes sure that the relation and other BDDs depend on the current vars.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lang_AutoVerifySupport( Lang_Auto_t * pAut )
{
    DdManager * dd = pAut->dd;
    DdNode * bSuppTot, * bSuppCs, * bSuppIOCs, * bSuppCur, * bDiff;
    int RetValue = 1;

    bSuppIOCs = Vmx_VarMapCharCubeRange( dd, pAut->pVmx, 0, pAut->nInputs + pAut->nLatches ); Cudd_Ref( bSuppIOCs );
    bSuppCs   = Vmx_VarMapCharCubeRange( dd, pAut->pVmx, pAut->nInputs,     pAut->nLatches ); Cudd_Ref( bSuppCs );
    bSuppTot  = Vmx_VarMapCharCubeRange( dd, pAut->pVmx, 0, pAut->nInputs + 2 * pAut->nLatches ); Cudd_Ref( bSuppTot );

    // check the initial states
    bSuppCur  = Cudd_Support( dd, pAut->bInit );                   Cudd_Ref( bSuppCur );
    bDiff     = Cudd_bddExistAbstract( dd, bSuppCur, bSuppCs );    Cudd_Ref( bDiff );
    if ( bDiff != b1 )
    {
        printf( "AutoVerify: Initial state BDD depends on extra variables " );
        PRB( dd, bDiff );
        RetValue = 0;
    }
    Cudd_RecursiveDeref( dd, bDiff );
    Cudd_RecursiveDeref( dd, bSuppCur );

    // check the states
    bSuppCur  = Cudd_Support( dd, pAut->bStates );                 Cudd_Ref( bSuppCur );
    bDiff     = Cudd_bddExistAbstract( dd, bSuppCur, bSuppCs );    Cudd_Ref( bDiff );
    if ( bDiff != b1 )
    {
        printf( "AutoVerify: Total states BDD depends on extra variables " );
        PRB( dd, bDiff );
        RetValue = 0;
    }
    Cudd_RecursiveDeref( dd, bDiff );
    Cudd_RecursiveDeref( dd, bSuppCur );

    // check the accepting states
    bSuppCur  = Cudd_Support( dd, pAut->bAccepting );              Cudd_Ref( bSuppCur );
    bDiff     = Cudd_bddExistAbstract( dd, bSuppCur, bSuppCs );    Cudd_Ref( bDiff );
    if ( bDiff != b1 )
    {
        printf( "AutoVerify: Accepting states BDD depends on extra variables " );
        PRB( dd, bDiff );
        RetValue = 0;
    }
    Cudd_RecursiveDeref( dd, bDiff );
    Cudd_RecursiveDeref( dd, bSuppCur );

    // check the relation
    bSuppCur  = Cudd_Support( dd, pAut->bRel );                    Cudd_Ref( bSuppCur );
    bDiff     = Cudd_bddExistAbstract( dd, bSuppCur, bSuppTot );   Cudd_Ref( bDiff );
    if ( bDiff != b1 )
    {
        printf( "AutoVerify: Relation BDD depends on extra variables " );
        PRB( dd, bDiff );
        RetValue = 0;
    }
    Cudd_RecursiveDeref( dd, bDiff );
    Cudd_RecursiveDeref( dd, bSuppCur );

    // dereference the remaining variables
    Cudd_RecursiveDeref( dd, bSuppIOCs );
    Cudd_RecursiveDeref( dd, bSuppCs );
    Cudd_RecursiveDeref( dd, bSuppTot );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Assigns the history of the automaton.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lang_AutoAssignHistory( Lang_Auto_t * pLang, int argc, char * argv[] )
{
    char Command[1000];
    char History[1000];
    int i;

    // get the command
    Command[0] = 0;
    for ( i = 0; i < argc; i++ )
    {
        strcat( Command, " " );
        strcat( Command, argv[i] );
    }

    // get the history
    sprintf( History, "%2d in  %5d st : %s", pLang->nInputs, pLang->nStates, Command );

    // assign the history
    FREE( pLang->pHistory );
    pLang->pHistory = util_strsav( History );
}


/**Function*************************************************************

  Synopsis    [Finds the state name for the DC state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Lang_AutoGetDcStateName( Lang_Auto_t * pLang )
{
    st_generator * gen;
    DdNode * bCode;
    char * pName;
    char pDcStateName[10];
    int fBreak;
    int Counter;
    assert( pLang->tCode2Name );
    // start with "DC" and create the name that does not exist among the state names
    sprintf( pDcStateName, "%s", "DC" );
    Counter = 1;
    while ( 1 )
    {
        fBreak = 0;
        st_foreach_item( pLang->tCode2Name, gen, (char **)&bCode, &pName )
        {
            if ( strcmp( pDcStateName, pName ) == 0 )
            {
                fBreak = 1;
                st_free_gen(gen);
                break;
            }
        }
        if ( !fBreak ) // it does not exist
            break;
        // if it exists, create a new name and check again
        sprintf( pDcStateName, "%s%d", "DC", Counter++ );
    }
    return util_strsav( pDcStateName );
}

/**Function*************************************************************

  Synopsis    [Converts the name table using the permutation array.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
st_table * Lang_AutoTransferNamesTable( Lang_Auto_t * pLang, DdManager * dd, int * pPermute )
{
    st_table * tCode2Name;
    st_generator * gen;
    DdNode * bCodeOld, * bCodeNew;
    char * pNameOld, * pNameNew;

    tCode2Name = st_init_table( st_ptrcmp, st_ptrhash );
    // go through all the entries in the old table and create the new table
    st_foreach_item( pLang->tCode2Name, gen, (char **)&bCodeOld, &pNameOld )
    {
        if ( pPermute )
            bCodeNew = Extra_TransferPermute( pLang->dd, dd, bCodeOld, pPermute );  
        else
            bCodeNew = Cudd_bddTransfer( pLang->dd, dd, bCodeOld );  
        Cudd_Ref( bCodeNew );
        pNameNew = util_strsav( pNameOld );
        st_insert( tCode2Name, (char *)bCodeNew, pNameNew );
    }
    return tCode2Name;
}


/**Function*************************************************************

  Synopsis    [Prints the name table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lang_AutoPrintNameTable( Lang_Auto_t * pLang )
{
    st_generator * gen;
    DdNode * bCode;
    char * pName;
    if ( pLang->tCode2Name == NULL )
    {
        printf( "Automaton \"%s\": The names table is not specified.\n", pLang->pName );
        return;
    }
    printf( "Automaton \"%s\": There are %d entries the in the names table.\n", pLang->pName, st_count(pLang->tCode2Name) );
    st_foreach_item( pLang->tCode2Name, gen, (char **)&bCode, &pName )
    {
        printf( "%15s : ", pName );
        PRB( pLang->dd, bCode );
    }
}


/**Function*************************************************************

  Synopsis    [Returns the char IO cube.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Lang_AutoCharCubeIO( DdManager * dd, Lang_Auto_t * pAut )
{
    return Vmx_VarMapCharCubeRange( dd, pAut->pVmx, 0, pAut->nInputs );
}

/**Function*************************************************************

  Synopsis    [Returns the char IO cube.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Lang_AutoCharCubeCs( DdManager * dd, Lang_Auto_t * pAut )
{
    return Vmx_VarMapCharCubeRange( dd, pAut->pVmx, pAut->nInputs, pAut->nLatches );
}

/**Function*************************************************************

  Synopsis    [Returns the char IO cube.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Lang_AutoCharCubeNs( DdManager * dd, Lang_Auto_t * pAut )
{
    return Vmx_VarMapCharCubeRange( dd, pAut->pVmx, pAut->nInputs + pAut->nLatches, pAut->nLatches );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



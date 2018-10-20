/**CFile****************************************************************

  FileName    [langFilter.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Implicit language solving flow.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: langFilter.c,v 1.4 2004/02/19 03:06:54 alanmi Exp $]

***********************************************************************/

#include "langInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Lang_AutoCleanUpStateTable( Lang_Auto_t * pAut, DdNode * bStatesAll, DdNode * bStatesRem );
static int Lang_AutoFilterStates( Lang_Auto_t * pAut, DdNode * bStatesLeft );


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lang_AutoPrefixClose( Lang_Auto_t * pAut )
{
    DdManager * dd = pAut->dd;
    DdNode * bStatesLeft;

    if ( pAut->nStates == 0 )
    {
        printf( "Trying to prefix close an automaton with no states.\n" );
        return 1;
    }

    // if the initial state is not accepting, inform the user
    if ( Cudd_bddLeq( dd, pAut->bInit, Cudd_Not(pAut->bAccepting) ) ) // no overlap
    {
        printf( "The initial state is not accepting.\n" );
        printf( "Prefix closed created an automaton with no states.\n" );
        // remove all state names
        Lang_AutoCleanUpStateTable( pAut, pAut->bStates, b0 );
        // remove everything
        Lang_AutoFilterStates( pAut, b0 );
        return 1;
    }
    // update the state names if there is any change
    Lang_AutoCleanUpStateTable( pAut, pAut->bStates, pAut->bAccepting );

    // make the states only the accepting states
    bStatesLeft = pAut->bAccepting;   Cudd_Ref( bStatesLeft );
    Lang_AutoFilterStates( pAut, pAut->bAccepting );
    Cudd_RecursiveDeref( dd, bStatesLeft );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Filters the states to contain only the given ones.]

  Description [Returns the number of states after the transformation.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lang_AutoFilterStates( Lang_Auto_t * pAut, DdNode * bStatesLeft )
{
    DdManager * dd = pAut->dd;
    DdNode * bTemp, * bStatesNs;

    // transform the initial state
    if ( Cudd_bddLeq( dd, pAut->bInit, Cudd_Not(bStatesLeft) ) ) // no overlap
    {
        Cudd_RecursiveDeref( dd, pAut->bInit );
        pAut->bInit = b0;              Cudd_Ref( b0 );
        Cudd_RecursiveDeref( dd, pAut->bStates );
        pAut->bStates = b0;            Cudd_Ref( b0 );
        Cudd_RecursiveDeref( dd, pAut->bAccepting );
        pAut->bAccepting = b0;         Cudd_Ref( b0 );
        Cudd_RecursiveDeref( dd, pAut->bRel );
        pAut->bRel = b0;               Cudd_Ref( b0 );
        pAut->nStates = 0;
        pAut->nAccepting = 0;
        return 0;
    }

    // transform the states
    Cudd_RecursiveDeref( pAut->dd, pAut->bStates );
    pAut->bStates = bStatesLeft;    Cudd_Ref( pAut->bStates );

    // transform the accepting states
    Cudd_RecursiveDeref( pAut->dd, pAut->bAccepting );
    pAut->bAccepting = bStatesLeft;    Cudd_Ref( pAut->bAccepting );

    // transform the transition relation
    bStatesNs = Vmx_VarMapRemapRange( dd, bStatesLeft, pAut->pVmx, 
        pAut->nInputs, pAut->nLatches, pAut->nInputs + pAut->nLatches, pAut->nLatches ); Cudd_Ref( bStatesNs );

    pAut->bRel = Cudd_bddAnd( dd, bTemp = pAut->bRel, bStatesLeft );   Cudd_Ref( pAut->bRel );
    Cudd_RecursiveDeref( dd, bTemp );
    pAut->bRel = Cudd_bddAnd( dd, bTemp = pAut->bRel, bStatesNs );     Cudd_Ref( pAut->bRel );
    Cudd_RecursiveDeref( dd, bTemp );

    Cudd_RecursiveDeref( dd, bStatesNs );

    // count the number of states
    pAut->nStates = (int)Cudd_CountMinterm( dd, pAut->bStates, pAut->nStateBits );
    pAut->nAccepting = pAut->nStates;
    return pAut->nStates;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lang_AutoProgressive( Lang_Auto_t * pAut, int nInputs, bool fVerbose )
{
    DdManager * dd = pAut->dd;
    st_table * tCode2Name = NULL;
    DdNode * bStatesAll, * bRelOrig, * bStatesLeft;
    DdNode * bCubeIO, * bTemp, * bIncomp;

    // remember the original states and state names
    if ( pAut->tCode2Name )
    {
        bStatesAll = pAut->bStates;    Cudd_Ref( bStatesAll );
        tCode2Name = pAut->tCode2Name;
        pAut->tCode2Name = NULL;
    }

    // make it prefix closed
    Lang_AutoPrefixClose( pAut );
    if ( pAut->nStates == 0 )
        return 1;
    
    // quantify the extra IO vars and save the original relation in bRelOrig
    bCubeIO = Vmx_VarMapCharCubeRange( dd, pAut->pVmx, nInputs, pAut->nInputs - nInputs );  Cudd_Ref( bCubeIO );
    pAut->bRel = Cudd_bddExistAbstract( dd, bRelOrig = pAut->bRel, bCubeIO );               Cudd_Ref( pAut->bRel );
    Cudd_RecursiveDeref( dd, bCubeIO );

    // iterate though the incomplete states
    while ( 1 )
    {
        // get the incomplete states
        bIncomp = Lang_AutoIncompleteStates( pAut, pAut->bRel, pAut->bStates );  Cudd_Ref( bIncomp );
        // if there is no incomplete states, quit
        if ( bIncomp == b0 )
        {
            Cudd_RecursiveDeref( dd, bIncomp );
            break;
        }
        // get the remaining states
        bStatesLeft = Cudd_bddAnd( dd, pAut->bStates, Cudd_Not( bIncomp ) );     Cudd_Ref( bStatesLeft );
        Cudd_RecursiveDeref( dd, bIncomp );
        // finter the remaining states
        Lang_AutoFilterStates( pAut, bStatesLeft );
        Cudd_RecursiveDeref( dd, bStatesLeft );
    }

    // transform back to the original relation
    pAut->bRel = Cudd_bddAnd( dd, bTemp = pAut->bRel, bRelOrig );                  Cudd_Ref( pAut->bRel );
    Cudd_RecursiveDeref( dd, bTemp );
    Cudd_RecursiveDeref( dd, bRelOrig );

    // remove the useless state names
    if ( tCode2Name )
    {
        pAut->tCode2Name = tCode2Name;
        Lang_AutoCleanUpStateTable( pAut, bStatesAll, pAut->bStates );
        Cudd_RecursiveDeref( dd, bStatesAll );
    }

    if ( pAut->nStates == 0 )
        printf( "The automaton is empty after applying progressive.\n" );

    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lang_AutoCleanUpStateTable( Lang_Auto_t * pAut, DdNode * bStatesAll, DdNode * bStatesRem )
{
    DdManager * dd = pAut->dd;
    DdNode * bFunc, * bCubeCs, * bCode, * bMintCs, * bTemp; 
    char * pName;
    int i;

    if ( pAut->tCode2Name == NULL )
        return 1;
    if ( bStatesAll == bStatesRem )
        return 1;
    if ( bStatesRem == b0 )
    {   // remove all 
        st_generator * gen;
        st_foreach_item( pAut->tCode2Name, gen, (char **)&bCode, &pName )
        {
            Cudd_RecursiveDeref( dd, bCode );
            FREE( pName );
        }
        st_free_table( pAut->tCode2Name );
        pAut->tCode2Name = NULL;
        return 1;
    }
//Lang_AutoPrintNameTable( pAut );

    // remove the state names corresponding to the state that do not remain
    bFunc = Cudd_bddAnd( dd, bStatesAll, Cudd_Not(bStatesRem) );       Cudd_Ref( bFunc );
    bCubeCs  = Lang_AutoCharCubeCs( dd, pAut );                        Cudd_Ref( bCubeCs );
//PRB( dd, bFunc );
//PRB( dd, bCubeCs );
    for ( i = 0; bFunc != b0; i++ )
    {
        // extract one minterm
        bMintCs = Extra_bddGetOneMinterm( dd, bFunc, bCubeCs );        Cudd_Ref( bMintCs );
//PRB( dd, bMintCs );
        // delete this state
        if ( !st_delete( pAut->tCode2Name, (char **)&bMintCs, &pName ) )
        {
            assert( 0 );
        }
        Cudd_RecursiveDeref( dd, bMintCs );
        FREE( pName );

        // subtract this minterm from the relation
        bFunc = Cudd_bddAnd( dd, bTemp = bFunc, Cudd_Not(bMintCs) );   Cudd_Ref( bFunc );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bMintCs );
    }
    Cudd_RecursiveDeref( dd, bFunc );
    Cudd_RecursiveDeref( dd, bCubeCs );
    return 1;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Lang_Auto_t * Lang_AutoReduce( Lang_Auto_t * pAut, int nInputs, bool fVerbose )
{
    return NULL;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



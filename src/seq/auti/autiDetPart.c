/**CFile****************************************************************

  FileName    [autiDeterm.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Automata determinization procedure.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: autiDetPart.c,v 1.1 2004/02/19 03:06:50 alanmi Exp $]

***********************************************************************/

#include "autiInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static st_table * Auti_AutoPrecomputePartitions( Aut_Auto_t * pAut, DdNode ** pbCubeCs );
static DdNode *   Auti_AutoStateSumAccepting( Aut_Auto_t * pAut );
static DdNode *   Auti_TestOr( DdManager * dd, DdNode * A, DdNode * B );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Another determinization procedure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aut_Auto_t * Auti_AutoDeterminizePart( Aut_Auto_t * pAut, int fAllAccepting, int fLongNames )
{
    ProgressBar * pProgress;
    Aut_Auto_t * pAutD;
    st_table * tSubset2Num;
    Aut_State_t * pS, * pState, * pStateD;
    Aut_State_t ** pEntry;
    Aut_Trans_t * pTrans;
    DdManager * dd;
    DdNode * bTrans, * bMint, * bSubset, * bCond, * bTemp;
    DdNode * bAccepting, * bCubeCs, * bCubeIn;
    st_table * tCode2State;
    int nDigits;
    char Buffer[100];
    char * pNamePrev, * pNameNew;
    int Counter;
    int s, i;

    if ( pAut->nStates == 0 )
    {
        printf( "Trying to determinize the automaton with no states.\n" );
        return pAut;
    }
    if ( pAut->nStates == 1 )
        return pAut;

    if ( Auti_AutoCheckIsNd( stdout, pAut, pAut->nVars, 0 ) == 0 )
    {
        printf( "The automaton is deterministic; determinization is not performed.\n" );
        return pAut;
    }

    // get the input cube
    dd = Aut_AutoReadDd(pAut);
    bCubeIn = Vmx_VarMapCharCubeRange( dd, pAut->pVmx, 0, pAut->nVars );  Cudd_Ref( bCubeIn );
    // get the partitioned transition relation
    tCode2State = Auti_AutoPrecomputePartitions( pAut, &bCubeCs );
    Cudd_Ref( bCubeCs );
    // after this call, pState->bCond stores the code 
    // while pState->bCondI stores the partitioned transition relation

    // start the new automaton
    pAutD = Aut_AutoClone( pAut, pAut->pMan );
    // set the determinized automaton name
    pAutD->pName = Extra_FileNameGeneric(pAut->pName);

    // create the initial state
    bSubset = pAut->pInit[0]->bCond;
    pState = Aut_StateAlloc( pAutD );
    Aut_StateSetData( pState, (char *)bSubset );   Cudd_Ref( bSubset );
    Aut_AutoStateAddLast( pState );
    // set the initial state
    Aut_AutoSetInitNum( pAutD, 1 );
    Aut_AutoSetInit( pAutD, pState );

    // start the table to collect the reachable states
    tSubset2Num = st_init_table(st_ptrcmp,st_ptrhash);
    st_insert( tSubset2Num, (char *)bSubset, (char *)pState );


    // iteratively process the subsets
    pProgress = Extra_ProgressBarStart( stdout, 1000 );
    i = 0;
    Aut_AutoForEachState( pAutD, pState )
    {
        // get the subset corresponding to this state of the ND automaton
        bSubset = (DdNode *)Aut_StateReadData(pState);    Cudd_Ref( bSubset );
        // get the transitions for this subset
        bTrans = b0;  Cudd_Ref( bTrans );
        Counter = 0;
        while ( bSubset != b0 )
        {
            // get one state 
            bMint = Extra_bddGetOneMinterm( dd, bSubset, bCubeCs );  Cudd_Ref( bMint );
//PRB( dd, bMint );
            // find the state's pointer
            if ( !st_lookup( tCode2State, (char *)bMint, (char **)&pS ) )
            {
                assert( 0 );
            }

            // add the partition of this state to the transitions
            bTrans = Cudd_bddOr( dd, bTemp = bTrans, pS->bCondI );  Cudd_Ref( bTrans );
//            bTrans = Auti_TestOr( dd, bTemp = bTrans, pS->bCondI );  Cudd_Ref( bTrans );
            Cudd_RecursiveDeref( dd, bTemp );
            // reduce the set of subsets
            bSubset = Cudd_bddAnd( dd, bTemp = bSubset, Cudd_Not(bMint) );  Cudd_Ref( bSubset );
            Cudd_RecursiveDeref( dd, bTemp );
            Cudd_RecursiveDeref( dd, bMint );
            Counter++;
        }
        Cudd_RecursiveDeref( dd, bSubset );
//PRB( dd, bTrans );
//        printf( " %d (%d)", Counter, Cudd_DagSize(bTrans) );

        // extract the subsets reachable by certain inputs
        while ( bTrans != b0 )
        {
            // get the minterm in terms of the input vars
            bMint = Extra_bddGetOneCube( dd, bTrans );                    Cudd_Ref( bMint );
            bMint = Cudd_bddExistAbstract( dd, bTemp = bMint, bCubeCs );  Cudd_Ref( bMint );
            Cudd_RecursiveDeref( dd, bTemp );
            // get the state subset
            bSubset = Cudd_bddAndAbstract( dd, bTrans, bMint, bCubeIn );  Cudd_Ref( bSubset ); // NS vars
            Cudd_RecursiveDeref( dd, bMint );
            // check if this subset was already visited
            if ( !st_find_or_add( tSubset2Num, (char *)bSubset, (char ***)&pEntry ) )
            { // does not exist - add it to storage
                pS = Aut_StateAlloc(pAutD);
                Aut_StateSetData( pS, (char *)bSubset );   Cudd_Ref( bSubset );
                Aut_AutoStateAddLast( pS );
                *pEntry = pS;
            }
            // get the condition
            bCond = Cudd_bddXorExistAbstract( dd, bTrans, bSubset, bCubeCs ); Cudd_Ref( bCond );
            bCond = Cudd_Not( bCond );
            Cudd_RecursiveDeref( dd, bSubset );

            // add the transition
            pTrans = Aut_TransAlloc(pAutD);
            Aut_TransSetStateFrom( pTrans, pState );
            Aut_TransSetStateTo  ( pTrans, *pEntry );
            Aut_TransSetCond     ( pTrans, bCond );    Cudd_Ref( bCond );
            // add the transition
            Aut_AutoAddTransition( pTrans );

            // reduce the transitions
            bTrans = Cudd_bddAnd( dd, bTemp = bTrans, Cudd_Not( bCond ) ); Cudd_Ref( bTrans );
            Cudd_RecursiveDeref( dd, bTemp );
            Cudd_RecursiveDeref( dd, bCond );
        }
        Cudd_RecursiveDeref( dd, bTrans );

        if ( i > 50 && (i % 20 == 0) )
        {
            char Buffer[100];
            sprintf( Buffer, "%d states", i );
            Extra_ProgressBarUpdate( pProgress, 1000 * i / pAutD->nStates, Buffer );
        }
        i++;
    }
    Extra_ProgressBarStop( pProgress );
 
    // get the number of digits in the state number
    nDigits = Extra_Base10Log(pAutD->nStates);

    // generate the state names and acceptable conditions
    s = 0;
    Aut_AutoForEachState( pAutD, pStateD )
    {
        // get the subset
        bSubset = (DdNode *)pStateD->pData;
//PRB( dd, bSubset );
        if ( fLongNames )
        { // print state names as subsets of the original state names
            int fFirst = 1;
            // alloc the name
            pNameNew = ALLOC( char, 2 );
            pNameNew[0] = 0;
            Aut_AutoForEachState( pAut, pState )
            {
//PRB( dd, pState->bCond );
                if ( Cudd_bddLeq( dd, pState->bCond, bSubset ) )
                {
                    if ( fFirst )
                        fFirst = 0;
                    else
                        strcat( pNameNew, "_" );
                    pNamePrev = pNameNew;
                    pNameNew = ALLOC( char, strlen(pNamePrev) + strlen(pState->pName) + 3 );
                    strcpy( pNameNew, pNamePrev );
                    strcat( pNameNew, pState->pName );
                    FREE( pNamePrev );
                }
            }
        }
        else
        { // print simple state names
            sprintf( Buffer, "s%0*d", nDigits, s );
            pNameNew = util_strsav( Buffer );
        }
        pStateD->pName = Aut_AutoRegisterName( pAutD, pNameNew );
        FREE( pNameNew );
        if ( st_insert( pAutD->tName2State, pStateD->pName, (char *)pStateD ) )
        {
            assert( 0 );
        }
        s++;
    }

    // set the accepting attributes of states
    bAccepting = Auti_AutoStateSumAccepting( pAut );   Cudd_Ref( bAccepting );
    Aut_AutoForEachState( pAutD, pState )
    {
        // get the subset
        bSubset = (DdNode *)pState->pData;
        // check whether this state is accepting
//        pState->fAcc = !Cudd_bddLeq( dd, pS->bSubset, Cudd_Not(bAccepting) );
        // check whether this state is accepting
        if ( fAllAccepting ) // requires all states to be accepting
            pState->fAcc =  Cudd_bddLeq( dd, bSubset, bAccepting );
        else // requires at least one state to be accepting
            pState->fAcc = !Cudd_bddLeq( dd, bSubset, Cudd_Not(bAccepting) );
        // deref the subset
        Cudd_RecursiveDeref( dd, bSubset );
    }
    Cudd_RecursiveDeref( dd, bAccepting );

    // free the table
    st_free_table( tSubset2Num );
    st_free_table( tCode2State );

    // clean the conditions in the states
    Aut_AutoDerefSumCond( pAut );
    Aut_AutoDerefSumCondI( pAut );
    Cudd_RecursiveDeref( dd, bCubeCs );
    Cudd_RecursiveDeref( dd, bCubeIn );

    printf( "Determinization:  (%d states, %d trans)  ->  (%d states, %d trans)\n", 
        pAut->nStates,  Aut_AutoCountTransitions( pAut ),
        pAutD->nStates, Aut_AutoCountTransitions( pAutD ) );
    return pAutD;
}


/**Function*************************************************************

  Synopsis    [Precomputes the partitions of the TR.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
st_table * Auti_AutoPrecomputePartitions( Aut_Auto_t * pAut, DdNode ** pbCubeCs )
{
    DdManager * dd;
    st_table * tCode2State;
    int nVarsOld, nVarsNew;
    Aut_State_t * pState;
    Aut_Trans_t * pTrans;
    DdNode ** pbVars;
    DdNode * bSum, * bProd, * bTemp;
    int i, s;

    // allocate extra vars
    dd = Aut_AutoReadDd( pAut );
    nVarsOld = dd->size;
    nVarsNew = Extra_Base2Log(pAut->nStates);
    for ( i = 0; i < nVarsNew; i++ )
        Cudd_bddNewVar( dd );

    // clean the conditions in the states
    Aut_AutoDerefSumCond( pAut );
    Aut_AutoDerefSumCondI( pAut );

    // create the mapping of codes into states
    pbVars = dd->vars + nVarsOld; 
    tCode2State = st_init_table( st_ptrcmp, st_ptrhash );
    s = 0; 
    Aut_AutoForEachState( pAut, pState )
    {
        pState->bCond = Extra_bddBitsToCube( dd, s, nVarsNew, pbVars ); 
        Cudd_Ref( pState->bCond );
//PRB( dd, pState->bCond );
        st_insert( tCode2State, (char *)pState->bCond, (char *)pState );
        s++;
    }

    // derive the state sums
    Aut_AutoForEachState( pAut, pState )
    {
        bSum = b0;      Cudd_Ref( bSum );
        Aut_StateForEachTransitionFrom_int( pState, pTrans )
        {
            bProd = Cudd_bddAnd( dd, pTrans->bCond, pTrans->pTo->bCond );  Cudd_Ref( bProd );
            bSum  = Cudd_bddOr( dd, bTemp = bSum, bProd );                 Cudd_Ref( bSum );
            Cudd_RecursiveDeref( dd, bTemp );
            Cudd_RecursiveDeref( dd, bProd );
        }
        pState->bCondI = bSum; // takes ref
    }
    *pbCubeCs = Extra_bddBitsToCube( dd, (1<<nVarsNew)-1, nVarsNew, pbVars );  
    return tCode2State;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Auti_AutoStateSumAccepting( Aut_Auto_t * pAut )
{
    Aut_State_t * pState;
    DdNode * bSum, * bTemp;
    DdManager * dd = Aut_AutoReadDd( pAut );
    bSum = b0;      Cudd_Ref( bSum );
    Aut_AutoForEachState( pAut, pState )
    {
        if ( !pState->fAcc )
            continue;
        bSum  = Cudd_bddOr( dd, bTemp = bSum, pState->bCond );   Cudd_Ref( bSum );
        Cudd_RecursiveDeref( dd, bTemp );
    }
    Cudd_Deref( bSum );
    return bSum;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Auti_TestOr( DdManager * dd, DdNode * A, DdNode * B )
{
    return Cudd_bddOr( dd, A, B );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



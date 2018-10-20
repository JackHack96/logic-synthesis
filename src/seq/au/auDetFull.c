/**CFile****************************************************************

  FileName    [auDeterm.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Automata determinization procedure.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: auDetFull.c,v 1.1 2004/02/19 03:06:45 alanmi Exp $]

***********************************************************************/

#include "auInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Determinize the ND automaton.]

  Description [This determinization procedure uses the transition
  relation. The performance is therefore limited to only those problems
  that allow for efficient TR representation.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Au_Auto_t * Au_AutoDeterminizeFull( Au_Auto_t * pAut, int fAllAccepting, int fLongNames )
{
    ProgressBar * pProgress;
    Au_Auto_t * pAutD;
    Au_Rel_t * pTR;
    st_table * tSubset2Num;
    Au_State_t * pS, * pState;
    Au_Trans_t * pTrans;
    DdManager * dd;
    DdNode * bTrans, * bMint, * bSubset, * bSubsetCs, * bCond, * bTemp;
    DdNode * bAccepting;
    int * pEntry, s, i;
//    int * pPermute;
    int nDigits;
    char Buffer[100];
    char * pNamePrev;
    int clk, clkTotal, clkPart;


    if ( pAut->nStates == 0 )
    {
        printf( "Trying to determinize the automaton with no states.\n" );
        return pAut;
    }

    if ( pAut->nStates == 1 )
        return pAut;

    if ( Au_AutoCheckNd( stdout, pAut, pAut->nInputs, 0 ) == 0 )
    {
        printf( "The automaton is deterministic; determinization is not performed.\n" );
        return pAut;
    }

    // get the transition relation
    dd = Cudd_Init( 50, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);
    pTR = Au_AutoGetTR( dd, pAut, 1 );
    Cudd_zddVarsFromBddVars( dd, 2 );
printf( "Automaton to be determinized has relation with %d BDD nodes.\n", Cudd_DagSize(pTR->bRel) );

    // set up the permutation array from NS to CS vars
//    pPermute = Au_UtilsPermuteNs2Cs( pTR );
    Au_UtilsSetVarMap( pTR );

    // start the new automaton
    pAutD = Au_AutoClone( pAut );
    pAutD->nStates      = 0;
    pAutD->nStatesAlloc = 1000;
    pAutD->pStates      = ALLOC( Au_State_t *, pAutD->nStatesAlloc );

    // create the initial state
    pState = Au_AutoStateAlloc();
    pState->bSubset = pTR->pbStatesCs[0];   Cudd_Ref( pTR->pbStatesCs[0] ); 
    pAutD->pStates[ pAutD->nStates++ ] = pState;

    // start the table to collect the reachable states
    tSubset2Num = st_init_table(st_ptrcmp,st_ptrhash);
    st_insert( tSubset2Num, (char *)pTR->pbStatesCs[0], (char *)0 );

    pProgress = Extra_ProgressBarStart( stdout, 1000 );

    clkTotal = clock();
    clkPart  = 0;

    // iteratively process the subsets
    for ( s = 0; s < pAutD->nStates; s++ )
    {
        // get the state
        pS = pAutD->pStates[s];
        // get the subset
        bSubset = pS->bSubset; // CS vars
        // get the transitions for this subset
        bTrans = Cudd_bddAndAbstract( dd, pTR->bRel, bSubset, pTR->bCubeCs ); Cudd_Ref( bTrans );
        // extract the subsets reachable by certain inputs
clk = clock();
        while ( bTrans != b0 )
        {
            // get the minterm in terms of the input vars
            bMint = Extra_bddGetOneCube( dd, bTrans );                         Cudd_Ref( bMint );
            bMint = Cudd_bddExistAbstract( dd, bTemp = bMint, pTR->bCubeNs );  Cudd_Ref( bMint );
            Cudd_RecursiveDeref( dd, bTemp );
            // add literals to the minterm
            for ( i = 0; i < pAut->nInputs; i++ )
            {
                if ( !Cudd_bddLeq( dd, bMint, Cudd_Not(dd->vars[i]) ) &&
                     !Cudd_bddLeq( dd, bMint, dd->vars[i] )
                    )
                {
                    bMint = Cudd_bddAnd( dd, bTemp = bMint, Cudd_Not(dd->vars[i]) );  Cudd_Ref( bMint );
                    Cudd_RecursiveDeref( dd, bTemp );
                }
            }
            assert( bMint != b0 );

            // get the state subset
            bSubset = Cudd_bddAndAbstract( dd, bTrans, bMint, pTR->bCubeIn );  Cudd_Ref( bSubset ); // NS vars
            Cudd_RecursiveDeref( dd, bMint );
//            bSubsetCs = Cudd_bddPermute( dd, bSubset, pPermute );              Cudd_Ref( bSubsetCs ); // CS vars
            bSubsetCs = Cudd_bddVarMap( dd, bSubset );              Cudd_Ref( bSubsetCs ); // CS vars
            // check if this subset was already visited
            if ( !st_find_or_add( tSubset2Num, (char *)bSubsetCs, (char ***)&pEntry ) )
            { // does not exist - add it to storage

                // realloc storage for states if necessary
                if ( pAutD->nStatesAlloc <= pAutD->nStates )
                {
                    pAutD->pStates  = REALLOC( Au_State_t *, pAutD->pStates,  2 * pAutD->nStatesAlloc );
                    pAutD->nStatesAlloc *= 2;
                }

                pState = Au_AutoStateAlloc();
                pState->bSubset = bSubsetCs;   Cudd_Ref( bSubsetCs );
                pAutD->pStates[pAutD->nStates] = pState;
                *pEntry = pAutD->nStates;
                pAutD->nStates++;
            }
//            bCond = Cudd_bddAndAbstract( dd, bTrans, bSubset, pTR->bCubeNs ); Cudd_Ref( bCond );
            // existential quantification cannot be used

            // get the condition
            bCond = Cudd_bddXorExistAbstract( dd, bTrans, bSubset, pTR->bCubeNs ); Cudd_Ref( bCond );
            bCond = Cudd_Not( bCond );
            Cudd_RecursiveDeref( dd, bSubset );
            Cudd_RecursiveDeref( dd, bSubsetCs );

            // add the transition
            pTrans = Au_AutoTransAlloc();
            pTrans->pCond = Fnc_FunctionDeriveSopFromMddSpecial( Fnc_ManagerReadManMvc(pAut->pMan), 
                dd, bCond, bCond, pTR->pVmx, pAut->nInputs );
            pTrans->StateCur  = s;
            pTrans->StateNext = *pEntry;

            // add the transition
            Au_AutoTransAdd( pS, pTrans );

            // reduce the transitions
            bTrans = Cudd_bddAnd( dd, bTemp = bTrans, Cudd_Not( bCond ) ); Cudd_Ref( bTrans );
            Cudd_RecursiveDeref( dd, bTemp );
            Cudd_RecursiveDeref( dd, bCond );
        }
        Cudd_RecursiveDeref( dd, bTrans );
clkPart += clock() - clk;

        if ( s > 30 && (s % 20 == 0) )
        {
            char Buffer[100];
            sprintf( Buffer, "%d states", s );
            Extra_ProgressBarUpdate( pProgress, 1000 * s / pAutD->nStates, Buffer );
        }
    }
    Extra_ProgressBarStop( pProgress );

//PRT( "Total", clock() - clkTotal );
//PRT( "Part",  clkPart );

    // get the number of digits in the state number
	for ( nDigits = 0, i = pAutD->nStates - 1;  i;  i /= 10,  nDigits++ );

    // generate the state names and acceptable conditions
    for ( s = 0; s < pAutD->nStates; s++ )
    {
        // get the state
        pS = pAutD->pStates[s];
        // set the subset
        bSubset = pS->bSubset;
        if ( fLongNames )
        { // print state names as subsets of the original state names
            int fFirst = 1;
            // alloc the name
            pS->pName = ALLOC( char, 2 );
            pS->pName[0] = 0;
            for ( i = 0; i < pAut->nStates; i++ )
            {
                if ( Cudd_bddLeq( dd, pTR->pbStatesCs[i], bSubset ) )
                {
                    if ( fFirst )
                        fFirst = 0;
                    else
                        strcat( pS->pName, "_" );
//                    strcat( pS->pName, pAut->pStates[i]->pName );

                    pNamePrev = pS->pName;
                    pS->pName = ALLOC( char, strlen(pNamePrev) + strlen(pAut->pStates[i]->pName) + 3 );
                    strcpy( pS->pName, pNamePrev );
                    strcat( pS->pName, pAut->pStates[i]->pName );
                    FREE( pNamePrev );
                }
            }
        }
        else
        { // print simple state names
            sprintf( Buffer, "s%0*d", nDigits, s );
            pS->pName = util_strsav( Buffer );
        }
/*
        else
        { // print state names as subsets in positional notation

            // alloc the name
            pS->pName = ALLOC( char, pAut->nStates + 1 );
            fAccepting = 0;
            for ( i = 0; i < pAut->nStates; i++ )
                if ( Cudd_bddLeq( dd, pTR->pbStatesCs[i], bSubset ) )
                {
                    pS->pName[i] = '1';
                    // check whether this state is accepting
                    if ( pAut->pStates[i]->fAcc )
                        fAccepting = 1;
                }
                else
                    pS->pName[i] = '0';
            pS->pName[pAut->nStates] = '\0';
        }
*/
    }


    // set the accepting attributes of states
    bAccepting = Au_AutoStateSumAccepting( pAut, dd, pTR->pbStatesCs ); Cudd_Ref( bAccepting );
    for ( s = 0; s < pAutD->nStates; s++ )
    {
        // get the state
        pS = pAutD->pStates[s];
        // check whether this state is accepting
//        pS->fAcc = !Cudd_bddLeq( dd, pS->bSubset, Cudd_Not(bAccepting) );
        // check whether this state is accepting
        if ( fAllAccepting ) // requires all states to be accepting
            pS->fAcc = Cudd_bddLeq( dd, pS->bSubset, bAccepting );
        else // requires at least one state to be accepting
            pS->fAcc = !Cudd_bddLeq( dd, pS->bSubset, Cudd_Not(bAccepting) );
        // deref the subset
        Cudd_RecursiveDeref( dd, pS->bSubset );
    }
    Cudd_RecursiveDeref( dd, bAccepting );

    // free the table
    st_free_table( tSubset2Num );
//    FREE( pPermute );

    Au_AutoRelFree( pTR );
    pAut->pTR = NULL;
    Extra_StopManager( dd );

    printf( "Determinization:  (%d states, %d trans)  ->  (%d states, %d trans)\n", 
        pAut->nStates,  Au_AutoCountTransitions( pAut ),
        pAutD->nStates, Au_AutoCountTransitions( pAutD ) );
    return pAutD;
}


/**Function*************************************************************

  Synopsis    [Determinize the ND automaton.]

  Description [This determinization procedure uses the transition
  relation. The performance is therefore limited to only those problems
  that allow for efficient TR representation.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Au_Auto_t * Au_AutoDeterminizeFullOld( Au_Auto_t * pAut, int fComplete, int fLongNames )
{
    ProgressBar * pProgress;
    Au_Auto_t * pAutD;
    Au_Rel_t * pTR;
    Mvr_Relation_t * pMvrTemp;
    st_table * tSubset2Num;
    Au_State_t * pS, * pState;
    Au_Trans_t * pTrans;
    DdManager * dd;
    DdNode * bTrans, * bMint, * bSubset, * bSubsetCs, * bCond, * bTemp;
    DdNode * bAccepting;
    int * pEntry, s, i;
    int * pPermute;
    int nDigits;
    char Buffer[100];
    char * pNamePrev;

    if ( pAut->nStates == 0 )
    {
        printf( "Trying to determinize the automaton with no states.\n" );
        return pAut;
    }

    if ( pAut->nStates == 1 )
        return pAut;

    if ( Au_AutoCheckNd( stdout, pAut, pAut->nInputs, 0 ) == 0 )
    {
        printf( "The automaton is deterministic; determinization is not performed.\n" );
        return pAut;
    }

    // get the transition relation
    dd = Mvr_ManagerReadDdLoc( Fnc_ManagerReadManMvr(pAut->pMan) );
    pTR = Au_AutoGetTR(dd, pAut, 0);
printf( "Automaton to be determinized has relation with %d BDD nodes.\n", Cudd_DagSize(pTR->bRel) );

    // get the temporary relation
    pMvrTemp = Mvr_RelationCreate( Fnc_ManagerReadManMvr(pAut->pMan), pTR->pVmx, b1 );

    // set up the permutatin array from NS to CS vars
    pPermute = Au_UtilsPermuteNs2Cs( pTR );

    // start the new automaton
    pAutD = Au_AutoClone( pAut );
    pAutD->nStates      = 0;
    pAutD->nStatesAlloc = 1000;
    pAutD->pStates      = ALLOC( Au_State_t *, pAutD->nStatesAlloc );

    // create the initial state
    pState = Au_AutoStateAlloc();
    pState->bSubset = pTR->pbStatesCs[0];   Cudd_Ref( pTR->pbStatesCs[0] ); 
    pAutD->pStates[ pAutD->nStates++ ] = pState;

    // start the table to collect the reachable states
    tSubset2Num = st_init_table(st_ptrcmp,st_ptrhash);
    st_insert( tSubset2Num, (char *)pTR->pbStatesCs[0], (char *)0 );

    pProgress = Extra_ProgressBarStart( stdout, 1000 );

    // iteratively process the subsets
    for ( s = 0; s < pAutD->nStates; s++ )
    {
        // get the state
        pS = pAutD->pStates[s];
        // get the subset
        bSubset = pS->bSubset; // CS vars
        // get the transitions for this subset
        bTrans = Cudd_bddAndAbstract( dd, pTR->bRel, bSubset, pTR->bCubeCs ); Cudd_Ref( bTrans );
        // extract the subsets reachable by certain inputs
        while ( bTrans != b0 )
        {
            // get the minterm in terms of the input vars
            bMint = Extra_bddGetOneCube( dd, bTrans );                         Cudd_Ref( bMint );
            bMint = Cudd_bddExistAbstract( dd, bTemp = bMint, pTR->bCubeNs );  Cudd_Ref( bMint );
            Cudd_RecursiveDeref( dd, bTemp );
            // get the state subset
            bSubset = Cudd_bddAndAbstract( dd, bTrans, bMint, pTR->bCubeIn );  Cudd_Ref( bSubset ); // NS vars
            Cudd_RecursiveDeref( dd, bMint );
            bSubsetCs = Cudd_bddPermute( dd, bSubset, pPermute );              Cudd_Ref( bSubsetCs ); // CS vars
            // check if this subset was already visited
            if ( !st_find_or_add( tSubset2Num, (char *)bSubsetCs, (char ***)&pEntry ) )
            { // does not exist - add it to storage

                // realloc storage for states if necessary
                if ( pAutD->nStatesAlloc <= pAutD->nStates )
                {
                    pAutD->pStates  = REALLOC( Au_State_t *, pAutD->pStates,  2 * pAutD->nStatesAlloc );
                    pAutD->nStatesAlloc *= 2;
                }

                pState = Au_AutoStateAlloc();
                pState->bSubset = bSubsetCs;   Cudd_Ref( bSubsetCs );
                pAutD->pStates[pAutD->nStates] = pState;
                *pEntry = pAutD->nStates;
                pAutD->nStates++;
            }
//            bCond = Cudd_bddAndAbstract( dd, bTrans, bSubset, pTR->bCubeNs ); Cudd_Ref( bCond );
            // existential quantification cannot be used

            // get the condition
            bCond = Cudd_bddXorExistAbstract( dd, bTrans, bSubset, pTR->bCubeNs ); Cudd_Ref( bCond );
            bCond = Cudd_Not( bCond );
            Cudd_RecursiveDeref( dd, bSubset );
            Cudd_RecursiveDeref( dd, bSubsetCs );

            // add the transition
            pTrans = Au_AutoTransAlloc();
            pTrans->pCond = Fnc_FunctionDeriveSopFromMdd( Fnc_ManagerReadManMvc(pAut->pMan), 
                pMvrTemp, bCond, bCond, pAut->nInputs );
            pTrans->StateCur  = s;
            pTrans->StateNext = *pEntry;
            // add the transition
            Au_AutoTransAdd( pS, pTrans );

            // reduce the transitions
            bTrans = Cudd_bddAnd( dd, bTemp = bTrans, Cudd_Not( bCond ) ); Cudd_Ref( bTrans );
            Cudd_RecursiveDeref( dd, bTemp );
            Cudd_RecursiveDeref( dd, bCond );
        }
        Cudd_RecursiveDeref( dd, bTrans );

        if ( s > 50 && (s % 20 == 0) )
        {
            char Buffer[100];
            sprintf( Buffer, "%d states", s );
            Extra_ProgressBarUpdate( pProgress, 1000 * s / pAutD->nStates, Buffer );
        }
    }
    Extra_ProgressBarStop( pProgress );

    // get the number of digits in the state number
	for ( nDigits = 0, i = pAutD->nStates - 1;  i;  i /= 10,  nDigits++ );

    // generate the state names and acceptable conditions
    for ( s = 0; s < pAutD->nStates; s++ )
    {
        // get the state
        pS = pAutD->pStates[s];
        // set the subset
        bSubset = pS->bSubset;
        if ( fLongNames )
        { // print state names as subsets of the original state names
            int fFirst = 1;
            // alloc the name
            pS->pName = ALLOC( char, 2 );
            pS->pName[0] = 0;
            for ( i = 0; i < pAut->nStates; i++ )
            {
                if ( Cudd_bddLeq( dd, pTR->pbStatesCs[i], bSubset ) )
                {
                    if ( fFirst )
                        fFirst = 0;
                    else
                        strcat( pS->pName, "_" );
//                    strcat( pS->pName, pAut->pStates[i]->pName );

                    pNamePrev = pS->pName;
                    pS->pName = ALLOC( char, strlen(pNamePrev) + strlen(pAut->pStates[i]->pName) + 3 );
                    strcpy( pS->pName, pNamePrev );
                    strcat( pS->pName, pAut->pStates[i]->pName );
                    FREE( pNamePrev );
                }
            }
        }
        else
        { // print simple state names
            sprintf( Buffer, "s%0*d", nDigits, s );
            pS->pName = util_strsav( Buffer );
        }
/*
        else
        { // print state names as subsets in positional notation

            // alloc the name
            pS->pName = ALLOC( char, pAut->nStates + 1 );
            fAccepting = 0;
            for ( i = 0; i < pAut->nStates; i++ )
                if ( Cudd_bddLeq( dd, pTR->pbStatesCs[i], bSubset ) )
                {
                    pS->pName[i] = '1';
                    // check whether this state is accepting
                    if ( pAut->pStates[i]->fAcc )
                        fAccepting = 1;
                }
                else
                    pS->pName[i] = '0';
            pS->pName[pAut->nStates] = '\0';
        }
*/
    }


    // set the accepting attributes of states
    bAccepting = Au_AutoStateSumAccepting( pAut, dd, pTR->pbStatesCs ); Cudd_Ref( bAccepting );
    for ( s = 0; s < pAutD->nStates; s++ )
    {
        // get the state
        pS = pAutD->pStates[s];
        // check whether this state is accepting
        pS->fAcc = !Cudd_bddLeq( dd, pS->bSubset, Cudd_Not(bAccepting) );
        // deref the subset
        Cudd_RecursiveDeref( dd, pS->bSubset );
    }
    Cudd_RecursiveDeref( dd, bAccepting );

    // free the table
    st_free_table( tSubset2Num );
    Mvr_RelationFree( pMvrTemp );
    FREE( pPermute );

    printf( "Determinization:  (%d states, %d trans)  ->  (%d states, %d trans)\n", 
        pAut->nStates,  Au_AutoCountTransitions( pAut ),
        pAutD->nStates, Au_AutoCountTransitions( pAutD ) );
    return pAutD;
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



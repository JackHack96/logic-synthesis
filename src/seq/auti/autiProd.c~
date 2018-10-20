/**CFile****************************************************************

  FileName    [autiProduct.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Computes the product of two automata.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: autiProd.c,v 1.2 2005/06/02 03:34:20 alanmi Exp $]

***********************************************************************/

#include "autiInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define PROD_STATE_HASH(s1,s2)   ((((unsigned)s1) << 16) | (((unsigned)s2) & 0xffff))
#define PROD_STATE_UNHASH1(s)     (((unsigned)s1) >> 16)
#define PROD_STATE_UNHASH2(s)     (((unsigned)s1) & 0xffff)

static Aut_State_t * Auti_AutoProductCreateState( Aut_Auto_t * pAutP, Aut_State_t * pS1, Aut_State_t * pS2 );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates the product of two automata.]

  Description [The resulting product automaton contains only the 
  reachable states.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aut_Auto_t * Auti_AutoProduct( Aut_Auto_t * pAut1, Aut_Auto_t * pAut2, int fLongNames )
{
    ProgressBar * pProgress;
    Aut_Auto_t * pAutP;
    Aut_State_t * pS, * pS1, * pS2, * pState, ** pEntry;
    Aut_Trans_t * pTrans, * pTrans1, * pTrans2;
    st_table * tState2Num;
    char Buffer[500];
    char * pName1, * pName2;
    int s, nDigits;
    unsigned Hash;


    // if one of the automata is empty return an empty automaton
    if ( pAut1->nStates == 0 )
    {
        printf( "Trying to make the product with an empty automaton.\n" );
        pAutP = Aut_AutoDup( pAut1, pAut1->pMan );
        // set the product automaton name
        pName1 = Extra_FileNameGeneric(pAut1->pName);
        pName2 = Extra_FileNameGeneric(pAut2->pName);
        sprintf( Buffer, "%s*%s", pName1, pName2 );
        FREE( pName1 );
        FREE( pName2 );
        pAutP->pName = util_strsav( Buffer );
        return pAutP;
    }
    if ( pAut2->nStates == 0 )
    {
        printf( "Trying to make the product with an empty automaton.\n" );
        pAutP = Aut_AutoDup( pAut2, pAut1->pMan );
        // set the product automaton name
        pName1 = Extra_FileNameGeneric(pAut1->pName);
        pName2 = Extra_FileNameGeneric(pAut2->pName);
        sprintf( Buffer, "%s*%s", pName1, pName2 );
        FREE( pName1 );
        FREE( pName2 );
        pAutP->pName = util_strsav( Buffer );
        return pAutP;
    }

    // make sure they have the same number of inputs
    // make sure that the ordering of input names are identical
    if ( !Auti_AutoProductCheckInputs( pAut1, pAut2 ) )
        return NULL;

    // assign the state numbers
    Aut_AutoAssignNumbers( pAut1 );
    Aut_AutoAssignNumbers( pAut2 );

    // start the product automaton
    pAutP = Aut_AutoClone( pAut1, pAut1->pMan );

    // set the product automaton name
    pName1 = Extra_FileNameGeneric(pAut1->pName);
    pName2 = Extra_FileNameGeneric(pAut2->pName);
    sprintf( Buffer, "%s*%s", pName1, pName2 );
    FREE( pName1 );
    FREE( pName2 );
    pAutP->pName = util_strsav( Buffer );

    // create the initial state
    pState = Auti_AutoProductCreateState( pAutP, pAut1->pInit[0], pAut2->pInit[0] );
    Aut_AutoStateAddLast( pState );
    pAutP->pInit[0] = pState;

    // start the table to remember the reached states of the product automaton
    tState2Num = st_init_table(st_numcmp,st_numhash);
    Hash = PROD_STATE_HASH( pAut1->pInit[0]->uData, pAut2->pInit[0]->uData );
    st_insert( tState2Num, (char *)Hash, (char *)pState );

    // perform explicit traversal of the product machine
    s = 0;
    pProgress = Extra_ProgressBarStart( stdout, 1000 );
    Aut_AutoForEachState_int( pAutP, pS )
    {
        // get the generating states of this product state
        pS1 = (Aut_State_t *)pS->pData;
        pS2 = (Aut_State_t *)pS->pData2;
        // check what states are reachable from this state in both automata
        Aut_StateForEachTransitionFrom_int( pS1, pTrans1 )
        {
            Aut_StateForEachTransitionFrom_int( pS2, pTrans2 )
            {
                // check if the conditions overlap
                if ( Aut_AutoTransOverlap( pTrans1, pTrans2 ) )
                {
                    // check if this product state is already visited
                    Hash = PROD_STATE_HASH(pTrans1->pTo->uData, pTrans2->pTo->uData);
                    if ( !st_find_or_add( tState2Num, (char *)Hash, (char ***)&pEntry ) )
                    { // does not exist - add it to storage
                        pState = Auti_AutoProductCreateState( pAutP, pTrans1->pTo, pTrans2->pTo );
                        *pEntry = pState;
                        Aut_AutoStateAddLast( pState );
                    }
                    // create a new transition
                    pTrans = Aut_TransAlloc( pAutP );
                    pTrans->pFrom = pS;
                    pTrans->pTo   = *pEntry;
                    pTrans->bCond = Cudd_bddAnd( pAutP->pMan->dd, pTrans1->bCond, pTrans2->bCond );
                    Cudd_Ref( pTrans->bCond );
                    // add the transition
                    Aut_AutoAddTransition( pTrans );
                }
            }
        }

        if ( ++s % 500 == 0 )
        {
            sprintf( Buffer, "%d states", s );
            Extra_ProgressBarUpdate( pProgress, 1000 * s / pAutP->nStates, Buffer );
        }
    }
    Extra_ProgressBarStop( pProgress );
    st_free_table( tState2Num );

    // detect the trivial case when the product automaton is empty
    if ( pAutP->nStates == 1 && pAutP->pHead->nTransFrom == 0 )
    {
        // there are no states and no transitions
        Aut_AutoStateDelete( pAutP->pHead );
        return pAutP;
    }

    // get the number of digits in the state number
    nDigits = Extra_Base10Log( pAutP->nStates );
    // create state names and accepting params
    s = 0;
    Aut_AutoForEachState_int( pAutP, pS )
    {
        // get the generating states of this product state
        pS1 = (Aut_State_t *)pS->pData;
        pS2 = (Aut_State_t *)pS->pData2;
        if ( fLongNames )
            sprintf( Buffer, "%s%s", pS1->pName, pS2->pName );
        else
            sprintf( Buffer, "s%0*d", nDigits, s );
        pS->pName = Aut_AutoRegisterName( pAutP, Buffer );
        s++;
        if ( st_insert( pAutP->tName2State, pS->pName, (char *)pState ) )
        {
            assert( 0 );
        }
    }

    printf( "Product: (%d st, %d trans) x (%d st, %d trans) -> (%d st, %d trans)\n", 
        pAut1->nStates, Aut_AutoCountTransitions( pAut1 ),
        pAut2->nStates, Aut_AutoCountTransitions( pAut2 ),
        pAutP->nStates, Aut_AutoCountTransitions( pAutP ) );

    return pAutP;
}

/**Function*************************************************************

  Synopsis    [Makes sure that two automata are comparable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Auti_AutoProductCommonSupp( Aut_Auto_t * pAut1, Aut_Auto_t * pAut2, char * pFileName1, char * pFileName2, Aut_Auto_t ** ppAut1, Aut_Auto_t ** ppAut2 )
{
    char pInputNames[2000], pStateName[500];
    char pCommand[1000];
    int nInputs;
    int i, k;

    // build the least common support
    pInputNames[0] = 0;
    nInputs = 0;
    for ( i = 0; i < pAut1->nVars; i++ )
    {
        sprintf( pStateName, "%s%s(%d)", (nInputs? ",":""), pAut1->pVars[i]->pName, pAut1->pVars[i]->nValues );
        strcat( pInputNames, pStateName );
        nInputs++;
    }
    for ( i = 0; i < pAut2->nVars; i++ )
    {
        for ( k = 0; k < pAut1->nVars; k++ )
            if ( strcmp( pAut1->pVars[k]->pName, pAut2->pVars[i]->pName ) == 0 )
                break;
        if ( k < pAut1->nVars )
            continue;
        // this is a new variable; add it to the list
        sprintf( pStateName, "%s%s(%d)", (nInputs? ",":""), pAut2->pVars[i]->pName, pAut2->pVars[i]->nValues );
        strcat( pInputNames, pStateName );
        nInputs++;
    }
    // bring the automata to the common support
    if ( pAut1->nVars != nInputs || pAut2->nVars != nInputs )
    {
        Mv_Frame_t * pMvsis = Mv_FrameGetGlobalFrame();

        printf( "Warning: The automata have different supports; using the least common support.\n" );

        // free the old automata
        Aut_AutoFree( pAut1 );
        Aut_AutoFree( pAut2 );

        // support the automata
        sprintf( pCommand,  "support %s %s %s", pInputNames, pFileName1, "tempAut1" ); 
        Cmd_CommandExecute( pMvsis, pCommand );
        sprintf( pCommand,  "support %s %s %s", pInputNames, pFileName2, "tempAut2" ); 
        Cmd_CommandExecute( pMvsis, pCommand );

        // derive the new automata

        pAut1 = Aio_AutoRead( pMvsis, "tempAut1", NULL );
        if ( pAut1 == NULL )
        {
            fprintf( stdout, "Reading the automaton from file \"%s\" has failed.\n", "tempAut1" );
            return 0;
        }
        pAut2 = Aio_AutoRead( pMvsis, "tempAut2", Aut_AutoReadMan(pAut1) );
        if ( pAut2 == NULL )
        {
            fprintf( stdout, "Reading the automaton from file \"%s\" has failed.\n", "tempAut2" );
            return 0;
        }
    }

    *ppAut1 = pAut1;
    *ppAut2 = pAut2;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Makes sure that two automata are comparable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Auti_AutoProductCheckInputs( Aut_Auto_t * pAut1, Aut_Auto_t * pAut2 )
{
    int i;
    // make sure they have the same number of inputs
    if ( pAut1->nVars != pAut2->nVars )
    {
        printf( "Cannot make the product of automata with different number of inputs.\n" );
        return 0;
    }

    // make sure that the ordering of input names are identical
    for ( i = 0; i < pAut1->nVars; i++ )
        if ( strcmp( pAut1->pVars[i]->pName, pAut2->pVars[i]->pName ) != 0 )
        {
            printf( "Cannot make the product of automata with different ordering of inputs.\n" );
            return 0;
        }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Makes sure that two automata are comparable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aut_State_t * Auti_AutoProductCreateState( Aut_Auto_t * pAutP, Aut_State_t * pS1, Aut_State_t * pS2 )
{
    Aut_State_t * pState;
    pState = Aut_StateAlloc( pAutP );
    pState->fAcc = (pS1->fAcc && pS2->fAcc);
    pState->pData = (char *)pS1;
    pState->pData2 = (char *)pS2;
    return pState;
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



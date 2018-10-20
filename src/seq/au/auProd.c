/**CFile****************************************************************

  FileName    [auProd.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [The product of two automata.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: auProd.c,v 1.1 2004/02/19 03:06:47 alanmi Exp $]

***********************************************************************/

#include "auInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define PROD_STATE_HASH(s1,s2)   ((((unsigned)s1) << 16) | (((unsigned)s2) & 0xffff))
#define PROD_STATE_UNHASH1(s)     (((unsigned)s1) >> 16)
#define PROD_STATE_UNHASH2(s)     (((unsigned)s1) & 0xffff)

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
Au_Auto_t * Au_AutoProduct( Au_Auto_t * pAut1, Au_Auto_t * pAut2, int fLongNames )
{
    ProgressBar * pProgress;
    Vm_VarMap_t * pVm;
    Mvc_Data_t * pData;
    Mvc_Cover_t * pCover;

    Au_Auto_t * pAutP;
    Au_State_t * pS, * pState;
    Au_Trans_t * pTrans, * pTrans1, * pTrans2;
    st_table * tState2Num;
    char Buffer[100];
    int * pEntry, s, nDigits;
    unsigned Hash;


    // make sure they have the same number of inputs
    // make sure that the ordering of input names are identical
    if ( !Au_AutoProductCheckInputs( pAut1, pAut2 ) )
        return NULL;

    // if one of the automata is empty return an empty automaton
    if ( pAut1->nStates == 0 )
    {
        printf( "Trying to make the product with an empty automaton.\n" );
        return Au_AutoDup( pAut1 );
    }
    if ( pAut2->nStates == 0 )
    {
        printf( "Trying to make the product with an empty automaton.\n" );
        return Au_AutoDup( pAut2 );
    }


    pAutP = Au_AutoClone( pAut1 );
    pAutP->nStates      = 0;
    pAutP->nStatesAlloc = 1000;
    pAutP->pStates      = ALLOC( Au_State_t *, pAutP->nStatesAlloc );

    // create the initial state
    pState = Au_AutoStateAlloc();
    pState->State1 = 0;
    pState->State2 = 0;
    pAutP->pStates[ pAutP->nStates++ ] = pState;

    // start the table to remember the reached states of the product automaton
    tState2Num = st_init_table(st_numcmp,st_numhash);
    st_insert( tState2Num, (char *)0, (char *)0 );

    // create the MVC data to check the intersection of conditions
    pCover = pAut1->pStates[0]->pHead->pCond;
    pVm    = Vm_VarMapCreateBinary( Fnc_ManagerReadManVm(pAutP->pMan), pAutP->nInputs, 0 );
    pData  = Mvc_CoverDataAlloc( pVm, pCover );

    pProgress = Extra_ProgressBarStart( stdout, 1000 );

    // perform explicit traversal of the product machine
    for ( s = 0; s < pAutP->nStates; s++ )
    {
        pS = pAutP->pStates[s];
        // check what states are reachable from this state in both automata
        Au_StateForEachTransition( pAut1->pStates[pS->State1], pTrans1 )
        {
            Au_StateForEachTransition( pAut2->pStates[pS->State2], pTrans2 )
            {
                // check if the conditions overlap
                if ( Mvc_CoverIsIntersecting( pData, pTrans1->pCond, pTrans2->pCond ) )
                {
                    // check if this state already exists
                    Hash = PROD_STATE_HASH(pTrans1->StateNext, pTrans2->StateNext);
                    if ( !st_find_or_add( tState2Num, (char *)Hash, (char ***)&pEntry ) )
                    { // does not exist - add it to storage

                        // realloc storage for states if necessary
                        if ( pAutP->nStatesAlloc <= pAutP->nStates )
                        {
                            pAutP->pStates  = REALLOC( Au_State_t *, pAutP->pStates,  2 * pAutP->nStatesAlloc );
                            pAutP->nStatesAlloc *= 2;
                        }

                        pState = Au_AutoStateAlloc();
                        pState->State1 = pTrans1->StateNext;
                        pState->State2 = pTrans2->StateNext;
                        pAutP->pStates[pAutP->nStates] = pState;
                        *pEntry = pAutP->nStates;
                        pAutP->nStates++;
                    }
                    // add the transition
                    pTrans = Au_AutoTransAlloc();
                    pTrans->pCond = Mvc_CoverBooleanAnd( pData, pTrans1->pCond, pTrans2->pCond );
                    Mvc_CoverContain( pTrans->pCond );
                    pTrans->StateCur  = s;
                    pTrans->StateNext = *pEntry;
                    // add the transition
                    Au_AutoTransAdd( pS, pTrans );
                }
            }
        }

        if ( s > 30 && (s % 20 == 0) )
        {
            char Buffer[100];
            sprintf( Buffer, "%d states", s );
            Extra_ProgressBarUpdate( pProgress, 1000 * s / pAutP->nStates, Buffer );
        }
    }
    Extra_ProgressBarStop( pProgress );

    st_free_table( tState2Num );
    Mvc_CoverDataFree( pData, pCover );

    // detect the trivial case when the product automaton is empty
    if ( pAutP->nStates == 1 && pAutP->pStates[0]->pHead == NULL )
    {
        // there are no state and no transitions
        FREE( pAutP->pStates[0] );
        pAutP->nStates = 0;
        return pAutP;
    }

    // get the number of digits in the state number
	for ( nDigits = 0, s = pAutP->nStates - 1;  s;  s /= 10,  nDigits++ );

    // create state names and accepting params
    for ( s = 0; s < pAutP->nStates; s++ )
    {
        pS        = pAutP->pStates[s];
        pS->fAcc  = (pAut1->pStates[pS->State1]->fAcc && pAut2->pStates[pS->State2]->fAcc);
        if ( fLongNames )
            pS->pName = Au_UtilsCombineNames( pAut1->pStates[pS->State1]->pName, pAut2->pStates[pS->State2]->pName );
        else
        {
            sprintf( Buffer, "s%0*d", nDigits, s );
            pS->pName = util_strsav( Buffer );
        }
    }

    printf( "Product: (%d st, %d trans) x (%d st, %d trans) -> (%d st, %d trans)\n", 
        pAut1->nStates, Au_AutoCountTransitions( pAut1 ),
        pAut2->nStates, Au_AutoCountTransitions( pAut2 ),
        pAutP->nStates, Au_AutoCountTransitions( pAutP ) );

    return pAutP;
}

/**Function*************************************************************

  Synopsis    [Makes sure that two automata are comparable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_AutoProductCheckInputs( Au_Auto_t * pAut1, Au_Auto_t * pAut2 )
{
    int i;

    // make sure they have the same number of inputs
    assert( pAut1->nStates < 64000 );
    assert( pAut2->nStates < 64000 );
    if ( pAut1->nInputs != pAut2->nInputs )
    {
        printf( "Cannot make the product of automata with different number of inputs.\n" );
        return 0;
    }

    // make sure that the ordering of input names are identical
    for ( i = 0; i < pAut1->nInputs; i++ )
        if ( strcmp( pAut1->ppNamesInput[i], pAut2->ppNamesInput[i] ) != 0 )
        {
            printf( "Cannot make the product of automata with different ordering of inputs.\n" );
            return 0;
        }
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



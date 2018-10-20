/**CFile****************************************************************

  FileName    [auState.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Procedure for state minimization of automata.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: auMinim.c,v 1.1 2004/02/19 03:06:46 alanmi Exp $]

***********************************************************************/

#include "auInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define PAIR_UNKNOWN 0   // the status of this pair is unknown 
#define PAIR_DISTING 1   // the pair of states is distinquishable
#define PAIR_EQUIVAL 2   // the pair of states is equivalent
#define PAIR_CHECKED 3   // the pair is currently being checked

#define STORE_LOOKUP(pStore,s,t)    ((s > t)? pStore[s][t]: pStore[t][s])
#define STORE_INSERT(pStore,s,t,v)  { if (s > t) pStore[s][t] = v; else pStore[t][s] = v; }

static int Au_AutoStateExplore_rec( Au_Auto_t * pAut, int s1, int s2, char ** pStore, Mvc_Data_t * pData );
static int Au_AutoStatesCompare( Au_Auto_t * pAut, int s1, int s2, char ** pStore, Mvc_Data_t * pData );

static int s_nStatesProcessed;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Au_Auto_t * Au_AutoStateMinimize( Au_Auto_t * pAut, int fUseTable )
{
    Vm_VarMap_t * pVm;
    Mvc_Data_t * pData;
    Mvc_Cover_t * pCover;

    ProgressBar * pProgress;
    Au_Auto_t * pAutR;
    Au_State_t * pState, * pStateR;
    Au_Trans_t * pTrans, * pTransR;
    char ** pStore;
    int * pStateMap;
    char * pStateUni;
    int s, t, i, nUnique;

    if ( pAut->nStates == 0 )
    {
        printf( "Trying to state minimize an empty automaton.\n" );
        return Au_AutoDup( pAut );
    }

    // check if the automaton is deterministic
    if ( Au_AutoCheckNd( stdout, pAut, pAut->nInputs, 0 ) > 0 )
    {
        printf( "The automaton is non-deterministic.\n" );
        return NULL;
    }

    // check if the automaton is complete
    if ( Au_AutoComplete( pAut, pAut->nInputs, 0, 0 ) )
        printf( "Warning: The automaton has been completed before state minimization.\n" );

    if ( pAut->nStates == 1 )
        return Au_AutoDup( pAut );

    // create the MVC data to check the intersection of conditions
    pCover = pAut->pStates[0]->pHead->pCond;
    pVm    = Vm_VarMapCreateBinary( Fnc_ManagerReadManVm(pAut->pMan), pAut->nInputs, 0 );
    pData  = Mvc_CoverDataAlloc( pVm, pCover );

    // alloc and clear storage to mark equivalent/non-equivalent state pairs
    pStore = ALLOC( char *, pAut->nStates );
    pStore[0] = ALLOC( char, pAut->nStates * pAut->nStates );
    memset( pStore[0], 0, sizeof(char) * pAut->nStates * pAut->nStates );
    for ( i = 1; i < pAut->nStates; i++ )
        pStore[i] = pStore[i-1] + pAut->nStates;


    // the method based on table is trivial and has cubic complexity
    if ( fUseTable )
    {
        int fChange;
        int nIters;
        pProgress = Extra_ProgressBarStart( stdout, pAut->nStates * pAut->nStates / 2 );
        s_nStatesProcessed = 0;
        // mark the distinquishable states due to accepting/rejecting
//        printf( "Initial iteration...\n" );
        for ( s = 0; s < pAut->nStates; s++ )
            for ( t = 0; t < s; t++ )
                if ( pAut->pStates[s]->fAcc ^ pAut->pStates[t]->fAcc )
                {
                    STORE_INSERT( pStore, s, t, PAIR_DISTING );
                    s_nStatesProcessed++;

                    if ( s_nStatesProcessed % 20 == 0 )
                        Extra_ProgressBarUpdate( pProgress, s_nStatesProcessed, NULL );
                }
        // refine the state pairs
        nIters = 0;
        do 
        {
//            printf( "Iteration %d...\n", ++nIters );
            fChange = 0;
            for ( s = 0; s < pAut->nStates; s++ )
                for ( t = 0; t < s; t++ )
                    if ( STORE_LOOKUP( pStore, s, t ) == PAIR_UNKNOWN )
                        if ( Au_AutoStatesCompare( pAut, s, t, pStore, pData ) )
                        {
                            fChange = 1;
                            if ( s_nStatesProcessed % 20 == 0 )
                                Extra_ProgressBarUpdate( pProgress, s_nStatesProcessed, NULL );
                        }
        }
        while ( fChange );
        Extra_ProgressBarStop( pProgress );

        // assume all other pairs to be equivalent
        for ( s = 0; s < pAut->nStates; s++ )
            for ( t = 0; t < s; t++ )
                if ( STORE_LOOKUP( pStore, s, t ) == PAIR_UNKNOWN )
                {
                    STORE_INSERT( pStore, s, t, PAIR_EQUIVAL );
                }
    }
    // this method is a little better than the above but it has a bug and it disabled now
    else
    {
        pProgress = Extra_ProgressBarStart( stdout, pAut->nStates * pAut->nStates / 2 );
        s_nStatesProcessed = 0;
        // explore every state pair
        for ( s = 0; s < pAut->nStates; s++ )
        {
            for ( t = 0; t < s; t++ )
                Au_AutoStateExplore_rec( pAut, s, t, pStore, pData );
            if ( s % 20 == 0 )
                Extra_ProgressBarUpdate( pProgress, s_nStatesProcessed, NULL );
        }
        Extra_ProgressBarStop( pProgress );
    }

    // remove the MVC data
    Mvc_CoverDataFree( pData, pCover );


    // create the mapping of states:
    // the number is the number of the new state, into which this one is mapped
    pStateMap = ALLOC( int,  pAut->nStates );
    pStateUni = ALLOC( char, pAut->nStates );
    memset( pStateUni, 0, sizeof(char) * pAut->nStates );
    nUnique = 0;
    for ( s = 0; s < pAut->nStates; s++ )
    {
        // for the given state (s), check if it is equivalent to
        // any other state, and find such a state with the smallest number
        for ( t = 0; t < s; t++ )
            if ( STORE_LOOKUP( pStore, s, t ) == PAIR_EQUIVAL )
            {
                pStateMap[s] = pStateMap[t];
//printf( "State \"%s\" is equivalent to state \"%s\"\n.", 
//       pAut->pStates[s]->pName,
//       pAut->pStates[t]->pName );
                break;
            }
        if ( t == s )
        { // could not find a matching state => unique state
           pStateMap[s] = nUnique++;
           // mark this state as unique
           pStateUni[s] = 1;
        }
    }
    FREE( pStore[0] );
    FREE( pStore );


    // start the state-minimized automaton
    pAutR = Au_AutoClone( pAut );
    pAutR->pName        = util_strsav( pAut->pName );
    pAutR->nStates      = nUnique;
    pAutR->nStatesAlloc = nUnique + 1;
    pAutR->pStates      = ALLOC( Au_State_t *, pAutR->nStatesAlloc );

    // combine the equivalent states
    // when two states are equivalent, one of them can be simply dropped
    // and all the transition, which go into it, are redirected to the other one
    for ( s = 0; s < pAut->nStates; s++ )
    {
        if ( pStateUni[s] ) // unique state
        {
            // get the old state
            pState = pAut->pStates[s];
            // create the new state
            pStateR = Au_AutoStateAlloc();
            pStateR->pName = util_strsav( pState->pName );
            pStateR->fAcc  = pState->fAcc;
            // copy the transitions
            Au_StateForEachTransition( pState, pTrans )
            {
                // add the transition
                pTransR = Au_AutoTransAlloc();
                pTransR->pCond = Mvc_CoverDup( pTrans->pCond );
                pTransR->StateCur  = pStateMap[pTrans->StateCur ];
                pTransR->StateNext = pStateMap[pTrans->StateNext];
                // add the transition
                Au_AutoTransAdd( pStateR, pTransR );
            }
            // add the new state
            pAutR->pStates[ pStateMap[s] ] = pStateR;
        }
    }
    FREE( pStateMap );
    FREE( pStateUni );

    printf( "State minimization:  (%d states, %d trans)  ->  (%d states, %d trans)\n", 
        pAut->nStates,  Au_AutoCountTransitions( pAut ),
        pAutR->nStates, Au_AutoCountTransitions( pAutR ) );

    return pAutR;
}

/**Function*************************************************************

  Synopsis    [Determines the status of the state pair.]

  Description [If the status of the pair is known, returns it. If the status
  is unknown, it is determined by this procedure, added to storage, and returned.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_AutoStateExplore_rec( Au_Auto_t * pAut, int s1, int s2, char ** pStore, Mvc_Data_t * pData )
{
    Au_State_t * pS1, * pS2;
    Au_Trans_t * pT1, * pT2;
    int Status;

    if ( (Status = STORE_LOOKUP( pStore, s1, s2 )) != PAIR_UNKNOWN )
        return Status;

    // check the trivial case, when the states are distinquishable 
    // because one of them is accepting, while the other is not
    pS1 = pAut->pStates[s1];
    pS2 = pAut->pStates[s2];
    if ( pS1->fAcc ^ pS2->fAcc )
    {
        s_nStatesProcessed++;
        STORE_INSERT( pStore, s1, s2, PAIR_DISTING );
        return PAIR_DISTING;
    }

    // set the current status of this pair as being checked
    STORE_INSERT( pStore, s1, s2, PAIR_CHECKED );

    // try all the matching transitions from the two states
    Au_StateForEachTransition( pS1, pT1 )
    {
        Au_StateForEachTransition( pS2, pT2 )
        {
            // check if the conditions overlap
            if ( Mvc_CoverIsIntersecting( pData, pT1->pCond, pT2->pCond ) )
            {
                // get the status of this pair
                Status = Au_AutoStateExplore_rec( pAut, pT1->StateNext, pT2->StateNext, pStore, pData );
                assert( Status != PAIR_UNKNOWN );
                if ( Status == PAIR_DISTING )
                {
                    s_nStatesProcessed++;
                    STORE_INSERT( pStore, s1, s2, PAIR_DISTING );
                    return PAIR_DISTING;
                }
            }
        }
    }
    // we explored all the branches and did not find the way 
    // to distinquish them; we conclude that the states are equivalent
    s_nStatesProcessed++;
    STORE_INSERT( pStore, s1, s2, PAIR_EQUIVAL );
    return PAIR_EQUIVAL;
}


/**Function*************************************************************

  Synopsis    [Compares the pair of states.]

  Description [Returns 1 if the states were found to be distinct;
  otherwise returns 0.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_AutoStatesCompare( Au_Auto_t * pAut, int s1, int s2, char ** pStore, Mvc_Data_t * pData )
{
    Au_Trans_t * pT1, * pT2;

    assert( STORE_LOOKUP( pStore, s1, s2 ) == PAIR_UNKNOWN );

    // try all the matching transitions from the two states
    Au_StateForEachTransition( pAut->pStates[s1], pT1 )
    {
        Au_StateForEachTransition( pAut->pStates[s2], pT2 )
        {
            // check if the conditions overlap
            if ( Mvc_CoverIsIntersecting( pData, pT1->pCond, pT2->pCond ) )
            {
                if ( STORE_LOOKUP( pStore, pT1->StateNext, pT2->StateNext ) == PAIR_DISTING )
                {
                    s_nStatesProcessed++;
                    STORE_INSERT( pStore, s1, s2, PAIR_DISTING );
                    return 1;
                }
            }
        }
    }
    // we explored the branches and could not find the way to distinquish these states
    return 0;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



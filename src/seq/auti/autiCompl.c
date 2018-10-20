/**CFile****************************************************************

  FileName    [autiCompl.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Procedures for completion and complemention.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: autiCompl.c,v 1.2 2005/06/02 03:34:20 alanmi Exp $]

***********************************************************************/

#include "autiInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Completes the automaton.]

  Description [Completes the automaton with the DC state, which is accepting
  iff the flag fAccepting is set to 1. The number of inputs (nInputs) can
  be used to specify the number of FSM inputs, w.r.t. which the completion
  is performed. This procedure returns 0 if the original automaton was 
  originally complete. Otherwise it returns the number of incomplete states,
  which were completed.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Auti_AutoComplete( Aut_Auto_t * pAut, int nInputs, bool fAccepting )
{
    ProgressBar * pProgress;
    DdManager * dd = pAut->pMan->dd;
    Aut_State_t * pState, * pStateDc;
    Aut_Trans_t * pTrans;
    int RetValue, s;

    // compute the I-conditions of states
    Aut_AutoDerefSumCond( pAut );
    Aut_AutoDerefSumCondI( pAut );
    Aut_AutoComputeSumCond( pAut );
    Aut_AutoComputeSumCondI( pAut, nInputs );

    // check for incomplete states
    RetValue = 0;
    Aut_AutoForEachState_int( pAut, pState )
        if ( pState->bCondI != b1 )
            RetValue++;

    if ( RetValue == 0 )
    {
        Aut_AutoDerefSumCond( pAut );
        Aut_AutoDerefSumCondI( pAut );
        return RetValue;
    }

    // complete by adding one state
    pStateDc = Aut_AutoCreateDcState( pAut );
    pStateDc->fAcc = fAccepting;

    // create transitions into this state
    pProgress = Extra_ProgressBarStart( stdout, pAut->nStates );
    s = 0;
    Aut_AutoForEachState_int( pAut, pState )
    {
        if ( pState->bCondI != b1 )
        {
            pTrans = Aut_TransAlloc( pAut );
            pTrans->pFrom = pState;
            pTrans->pTo = pStateDc;
            pTrans->bCond = Cudd_Not( pState->bCond );  Cudd_Ref( pTrans->bCond );
            Aut_AutoAddTransition( pTrans );
        }
        if ( ++s % 100 == 0 )
            Extra_ProgressBarUpdate( pProgress, s, NULL );
    }
    Extra_ProgressBarStop( pProgress );
    // add the DC state
    Aut_AutoStateAddLast( pStateDc );
    assert( pStateDc->nTransFrom == 1 );
    assert( pStateDc->nTransTo > 0 );

    Aut_AutoDerefSumCond( pAut );
    Aut_AutoDerefSumCondI( pAut );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Complements the automaton.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Auti_AutoComplement( Aut_Auto_t * pAut )
{
    Aut_State_t * pState;
    Aut_AutoForEachState( pAut, pState )
        Aut_StateSetAcc( pState, !Aut_StateReadAcc(pState) );
}


/**Function*************************************************************

  Synopsis    [Returns the don't-care state if it exists, or NULL if not.]

  Description [The don't-care state is defined as a state that has transitions
  into it and a self-loop into itself with the constant 1 condition on it. 
  The don't-care state exists only in a completed automaton. It can be either 
  accepting or not.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aut_State_t * Auti_AutoFindDcState( Aut_Auto_t * pAut )
{
    Aut_State_t * pState;
    // go through the states of the automaton in the reverse order
    // because the DC is likely to be encountered at the end
    for ( pState = pAut->pTail; pState; pState = pState->pPrev )
    {
        // the number of transitions from the state should be 1
        // the only transition should be a self-loop
        // the condition on the self-loop should be constant 1
        if ( Aut_StateReadNumTransFrom(pState) == 1 && 
             Aut_TransReadStateTo( Aut_StateReadHeadFrom(pState) ) == pState &&
             Aut_TransReadCond   ( Aut_StateReadHeadFrom(pState) ) == Aut_AutoReadDd(pAut)->one )
             return pState;
    }
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Returns the accepting don't-care state if it exists, or NULL if not.]

  Description [The don't-care state is defined as a state that has transitions
  into it and a self-loop into itself with the constant 1 condition on it. 
  The don't-care state exists only in a completed automaton. It can be either 
  accepting or not.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aut_State_t * Auti_AutoFindDcStateAccepting( Aut_Auto_t * pAut )
{
    Aut_State_t * pState;
    // go through the states of the automaton in the reverse order
    // because the DC is likely to be encountered at the end
    for ( pState = pAut->pTail; pState; pState = pState->pPrev )
    {
        if ( pState->fAcc == 0 )
            continue;
        // the number of transitions from the state should be 1
        // the only transition should be a self-loop
        // the condition on the self-loop should be constant 1
        if ( Aut_StateReadNumTransFrom(pState) == 1 && 
             Aut_TransReadStateTo( Aut_StateReadHeadFrom(pState) ) == pState &&
             Aut_TransReadCond   ( Aut_StateReadHeadFrom(pState) ) == Aut_AutoReadDd(pAut)->one )
             return pState;
    }
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Returns the don't-care state if it exists, or NULL if not.]

  Description [The don't-care state is defined as a state that has transitions
  into it and a self-loop into itself with the constant 1 condition on it. 
  The don't-care state exists only in a completed automaton. It can be either 
  accepting or not.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aut_State_t * Auti_AutoFindDcStateNonAccepting( Aut_Auto_t * pAut )
{
    Aut_State_t * pState;
    // go through the states of the automaton in the reverse order
    // because the DC is likely to be encountered at the end
    for ( pState = pAut->pTail; pState; pState = pState->pPrev )
    {
        if ( pState->fAcc == 1 )
            continue;
        // the number of transitions from the state should be 1
        // the only transition should be a self-loop
        // the condition on the self-loop should be constant 1
        if ( Aut_StateReadNumTransFrom(pState) == 1 && 
             Aut_TransReadStateTo( Aut_StateReadHeadFrom(pState) ) == pState &&
             Aut_TransReadCond   ( Aut_StateReadHeadFrom(pState) ) == Aut_AutoReadDd(pAut)->one )
             return pState;
    }
    return NULL;
}


/**Function*************************************************************

  Synopsis    [Prints stats about the MV automaton.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Auti_AutoPrintStats( FILE * pFile, Aut_Auto_t * pAut, int nInputs, int fVerbose )
{
    Aut_Var_t ** pVars;
    char Buffer1[20], Buffer2[20];
    char Buffer3[20], Buffer4[20];
    int nStatesIncomp, nStatesNonDet;
    int nStatesNonPro, nStatesNonMoore;
    int nVars, c;
  
    // the first line
    // completeness of the automaton as automaton
    nStatesIncomp   = Auti_AutoCheckIsComplete( pAut, pAut->nVars );
    // determinism of the FSM
    nStatesNonDet   = Auti_AutoCheckIsNd( pFile, pAut, nInputs, 0 );
    // progressiveness of the FSM
    nStatesNonPro   = Auti_AutoCheckIsComplete( pAut, nInputs );
    // Moore-ness of the FSM
    nStatesNonMoore = Auti_AutoCheckIsMoore( pAut, nInputs );
    // create the output
    sprintf( Buffer1, " (%d st)", nStatesIncomp );
    sprintf( Buffer2, " (%d st)", nStatesNonDet );
    sprintf( Buffer3, " (%d st)", nStatesNonPro );
    sprintf( Buffer4, " (%d st)", nStatesNonMoore );

	fprintf( pFile, "\"%s\": %s%s, %s%s, %s%s, and %s%s.\n", 
        pAut->pName,  
        (nStatesIncomp?   "incomplete" : "complete"),
        (nStatesIncomp?   Buffer1 : ""),
        (nStatesNonDet?   "non-deterministic": "deterministic"),
        (nStatesNonDet?   Buffer2 : ""),
        (nStatesNonPro?   "non-progressive": "progressive"),
        (nStatesNonPro?   Buffer3 : ""),
        (nStatesNonMoore? "non-Moore": "Moore"),
        (nStatesNonMoore? Buffer4 : "")     
        );

    // the third line
    fprintf( pFile, "%d inputs ",        Aut_AutoReadVarNum(pAut) );
    fprintf( pFile, "(%d FSM inputs)  ", nInputs );
	fprintf( pFile, "%d states ",        Aut_AutoReadStateNum(pAut) );
	fprintf( pFile, "(%d accepting)  ",  Aut_AutoCountAccepting(pAut) );
	fprintf( pFile, "%d trans\n",        Aut_AutoCountTransitions(pAut) );
//	fprintf( pFile, "%d products\n",  Aut_AutoCountProducts(pAut) );

    // the second line
    fprintf( pFile, "Inputs = { ",   Aut_AutoReadVarNum(pAut) );
    nVars = Aut_AutoReadVarNum(pAut);
    pVars = Aut_AutoReadVars(pAut);
    for ( c = 0; c < nVars; c++ )
    {
        fprintf( pFile, "%s%s", ((c==0)? "": ","), Aut_VarReadName(pVars[c]) );
        if ( Aut_VarReadValueNum(pVars[c]) > 2 )
            fprintf( pFile, "(%d)", Aut_VarReadValueNum(pVars[c]) );
    }
    fprintf( pFile, " }\n" );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



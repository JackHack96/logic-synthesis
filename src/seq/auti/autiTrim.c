/**CFile****************************************************************

  FileName    [autiTrim.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    []

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: autiTrim.c,v 1.1 2004/04/08 05:53:14 alanmi Exp $]

***********************************************************************/

#include "autiInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Auti_AutoAddTrimStateCond( Aut_State_t * pStateX, Aut_State_t * pStateP );
static void Auti_AutoTrimTransitions( Aut_Auto_t * pAutX );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Trims the MGS by removing some internal states.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aut_Auto_t * Auti_AutoTrim( Aut_Auto_t * pAutX, Aut_Auto_t * pAutA )
{
    Aut_Auto_t * pAutP;
    Aut_State_t * pStateP, * pStateX;

    if ( pAutX->nStates == 0 )
    {
        printf( "Trying to trim an empty automaton.\n" );
        return pAutX;
    }

    // make sure pAutA is complete
    if ( Auti_AutoComplete( pAutA, pAutA->nVars, 0 ) )
        printf( "Warning: The known solution has been completed.\n" );

    // create the product
    pAutP = Auti_AutoProduct( pAutX, pAutA, 1 );
    if ( pAutP == NULL )
    {
        Aut_AutoFree( pAutX );
        return NULL;
    }
//Aio_AutoWrite( pAutP, "prod.aut", 1, 0 );

    // go through the states of the product automaton and collect
    // information about the transitions from (a a) states into (a n) states
    // this information will be stored in pStateX->bCond of each state of pAutX
    Aut_AutoDerefSumCond( pAutX );
    Aut_AutoForEachState( pAutP, pStateP )
    {
        pStateX = (Aut_State_t *)pStateP->pData;
        Auti_AutoAddTrimStateCond( pStateX, pStateP );
    }
    Aut_AutoFree( pAutP );

    // trim the transitions
    Auti_AutoTrimTransitions( pAutX );
    Aut_AutoDerefSumCond( pAutX );

    // get the remaining states
    Auti_AutoPrefixClose( pAutX );

/*
    // trim X by preserving the DCA state
    pStateX = Auti_AutoFindDcStateAccepting( pAutX );
    if ( pStateX )
        pStateX->fMark = 0;
    // check if the initial state has to be removed
    if ( pAutX->pInit[0]->fMark )
    {
        Aut_AutoSetMark( pAutX );
        Aut_AutoRemoveMarkedStates( pAutX );
        pAutX->pInit[0] = NULL;
        printf( "Warning: Trimming has removed the initial state.\n" );
        return pAutX;
    }
    // remove the rest of the states
    Aut_AutoRemoveMarkedStates( pAutX );
*/

    FREE( pAutX->pName );
    Aut_AutoSetName( pAutX, util_strsav( "trim" ) );

    return pAutX;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Auti_AutoAddTrimStateCond( Aut_State_t * pStateX, Aut_State_t * pStateP )
{
    Aut_Trans_t * pTransP;
    DdManager * dd = pStateX->pAut->pMan->dd;
    DdNode * bTemp;

    if ( !pStateP->fAcc )
        return;
    Aut_StateForEachTransitionFrom( pStateP, pTransP )
    {
        if ( pTransP->pTo->fAcc )
            continue;
        if ( pStateX->bCond == NULL )
        {
            pStateX->bCond = pTransP->bCond;   Cudd_Ref( pStateX->bCond );
        }
        else
        {
            pStateX->bCond = Cudd_bddOr( dd, bTemp = pStateX->bCond, pTransP->bCond ); Cudd_Ref( pStateX->bCond );
            Cudd_RecursiveDeref( dd, bTemp );
        }
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Auti_AutoTrimTransitions( Aut_Auto_t * pAutX )
{
    Aut_State_t * pStateX, * pStateDCA;
    Aut_Trans_t * pTransX;
    DdManager * dd = pAutX->pMan->dd;
    DdNode * bTemp;

    pStateDCA = Auti_AutoFindDcStateAccepting( pAutX );
    Aut_AutoForEachState( pAutX, pStateX )
    {
        if ( pStateX->bCond == NULL )
            continue;
        if ( pStateX == pStateDCA )
            continue;
        Aut_StateForEachTransitionFrom( pStateX, pTransX )
        {
            if ( pTransX->pTo == pStateDCA )
                continue;
            pTransX->bCond = Cudd_bddAnd( dd, bTemp = pTransX->bCond, Cudd_Not(pStateX->bCond) ); 
            Cudd_Ref( pTransX->bCond );
            Cudd_RecursiveDeref( dd, bTemp );
        }
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



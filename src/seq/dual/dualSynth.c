/**CFile****************************************************************

  FileName    [dualSynth.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Automata synthesis from L language specification.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 18, 2003.]

  Revision    [$Id: dualSynth.c,v 1.2 2004/02/19 03:06:53 alanmi Exp $]

***********************************************************************/

#include "dualInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Derives the automaton from the L language formula.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Au_Auto_t * Dual_AutoSynthesize( Dual_Spec_t * p )
{
    Au_Auto_t * pAut;
    DdManager * dd = p->dd;
    DdNode * bFunc, * bTemp, * bCube;
    Vm_VarMap_t * pVm;
    Vmx_VarMap_t * pVmx;
    char Buffer[100];
    int * pStateIsUsed;
    int * pStateHasTrans;
    int * pStatesVisit;
    int nStatesVisit;
    Au_Trans_t * pTrans;
    Au_State_t ** ppStatesTemp;
    int * pMapping;
    int nCubes, nDigits;
    int iState, i, k, s;

    // create ZDD variables for writing the covers
    Cudd_zddVarsFromBddVars( dd, 2 );

    // start the automaton
    pAut = Au_AutoAlloc();
    pAut->nInputs = p->nVars;
    pAut->nOutputs = 0;
    pAut->nStates = p->nStates;
    pAut->nStatesAlloc = p->nStates;
    pAut->ppNamesInput = ALLOC( char *, pAut->nInputs );
    // copy the input names
    for ( i = 0; i < p->nVars; i++ )
        pAut->ppNamesInput[i] = util_strsav( p->ppVarNames[i] );
//    pAut->ppNamesOutput = ALLOC( char *, pAut->nOutputs );
    // allocate the states
    pAut->pStates = ALLOC( Au_State_t *, pAut->nStatesAlloc );
    memset( pAut->pStates, 0, sizeof(int) * pAut->nStatesAlloc );

    // get the temporary variable map
    pVm  = Vm_VarMapCreateBinary( Fnc_ManagerReadManVm(p->pMan), p->nVars, 0 );
    pVmx = Vmx_VarMapLookup( Fnc_ManagerReadManVmx(p->pMan), pVm, -1, NULL );

    // explore the states reachable from the initial one
    pStateIsUsed = ALLOC( int, p->nStates );    // marks the states that has been explored
    pStateHasTrans = ALLOC( int, p->nStates );  // marks the states that have incoming or outgoing transitions
    pStatesVisit = ALLOC( int, p->nStates );    // the list of visited states
    memset( pStateIsUsed, 0, sizeof(int) * p->nStates );
    memset( pStateHasTrans, 0, sizeof(int) * p->nStates );

//PRB( dd, p->bCondInit );
    // add the initial states to the array of starting states
    nStatesVisit = 0;
    for ( i = 0; i < p->nStates; i++ )
    {
//PRB( dd, p->pbMarks[i] );
        // add the i-th state if it intersects with the initial condition
        if ( Cudd_bddLeq( dd, p->bCondInit, Cudd_Not(p->pbMarks[i]) ) == 0 )
        {
//printf( "Sat\n" );
            // add the state to the set of visited states
            pStatesVisit[ nStatesVisit++ ] = i;
            // mark the state as used
            pStateIsUsed[ i ] = 1;
        }
    }

    // save the initial states
    p->nStatesInit = nStatesVisit;
    p->pStatesInit = ALLOC( int, p->nStatesInit );
    memcpy( p->pStatesInit, pStatesVisit, sizeof(int) * p->nStatesInit );


    // explore the states starting from the initial ones
    nCubes = 0;
    bCube = Extra_bddComputeCube( dd, dd->vars, p->nRank * p->nVars );    Cudd_Ref( bCube );

    for ( s = 0; s < nStatesVisit; s++ )
    {
        // set the number of the state to visit (i)
        i = pStatesVisit[s];
        // start a new state
        pAut->pStates[i] = Au_AutoStateAlloc(); 
        pAut->pStates[i]->fAcc = 1;
        // check transitions from this state into other states
        for ( k = 0; k < p->nStates; k++ )
        {
    //        bFunc = Cudd_bddAnd( dd, p->pbTrans[i], p->pbMarks[k] );          Cudd_Ref( bFunc );
            bFunc = Cudd_bddAnd( dd, p->pbMarksP[i], p->pbTrans[i] );         Cudd_Ref( bFunc );
            // multiply by the original spec
            bFunc = Cudd_bddAnd( dd, bTemp = bFunc, p->pbMarks[k] );          Cudd_Ref( bFunc );
            Cudd_RecursiveDeref( dd, bTemp );
            if ( bFunc == b0 )
            {
                Cudd_RecursiveDeref( dd, bFunc );
                continue;
            }

            // check whether this is a new reachable state
            if ( pStateIsUsed[k] == 0 ) // not yet used
            {
                pStatesVisit[ nStatesVisit++ ] = k;
                pStateIsUsed[k] = 1;
            }

            // mark both states as having transitions
            pStateHasTrans[i] = 1;
            pStateHasTrans[k] = 1;

            bFunc = Cudd_bddExistAbstract( dd, bTemp = bFunc, bCube );        Cudd_Ref( bFunc );
            Cudd_RecursiveDeref( dd, bTemp );

            bFunc = Extra_bddMove( dd, bTemp = bFunc, -p->nVars * p->nRank ); Cudd_Ref( bFunc );
            Cudd_RecursiveDeref( dd, bTemp );

            // add transition from i to k
            pTrans = Au_AutoTransAlloc();
            pTrans->pCond = Fnc_FunctionDeriveSopFromMddSpecial( 
                Fnc_ManagerReadManMvc(p->pMan), dd, bFunc, bFunc, pVmx, p->nVars );
            pTrans->StateCur  = i;
            pTrans->StateNext = k;
            // add the transition
            Au_AutoTransAdd( pAut->pStates[i], pTrans );
            Cudd_RecursiveDeref( dd, bFunc );
    //printf( "%d -> %d: ", i, k );
    //PRB( dd, bFunc );
        }
    }
    Cudd_RecursiveDeref( dd, bCube );

    // compact the rest of the states in such a way that 
    // first, there are initial states, which are visited
    // then all the rest reachable from initial
    ppStatesTemp = ALLOC( Au_State_t *, p->nStates );
    // mapping shows the number of the old state in the new order
    pMapping = ALLOC( int, p->nStates );
    for ( i = 0; i < p->nStates; i++ )
        pMapping[i] = -1;
    // set the initial states
    iState = 0;
    for ( s = 0; s < p->nStatesInit; s++ )
    {
        // get the real number of this initial state
        i = p->pStatesInit[s];
        // skip it, if the state has no transitions
        if ( pStateHasTrans[i] == 0 )
            continue;
        // map this state into "iState"
        ppStatesTemp[iState] = pAut->pStates[i];   pAut->pStates[i] = NULL;
        pMapping[i] = iState;
        // unmark the initial state
        pStateHasTrans[i] = 0;
        iState++;
    }
    p->nStatesInit = iState;
    pAut->nInitial = iState;
    // set the remaining states
    for ( s = 0; s < nStatesVisit; s++ )
    {
        i = pStatesVisit[s];
        if ( pStateHasTrans[i] == 0 )
            continue;
        ppStatesTemp[iState] = pAut->pStates[i];    pAut->pStates[i] = NULL;
        pMapping[i] = iState;
        // unmark the initial states
        pStateHasTrans[i] = 0;
        iState++;
    }

    // clean the remaining states
    for ( s = 0; s < nStatesVisit; s++ )
        if ( pAut->pStates[s] )
        {
            assert( pAut->pStates[s]->pHead == NULL );
            Au_AutoStateFree(pAut->pStates[s]);
        }

    // copy the states into the new place
    FREE( pAut->pStates );
    pAut->pStates = ppStatesTemp;
    pAut->nStates = iState;

    // determine the length of the longest state name
	for ( nDigits = 0,  i = pAut->nStates - 1;  i;  i /= 10,  nDigits++ );

    // use the map to update the state pointers
    for ( s = 0; s < pAut->nStates; s++ )
    {
        Au_StateForEachTransition( pAut->pStates[s], pTrans )
        {
            pTrans->StateCur = pMapping[pTrans->StateCur];
            assert( pTrans->StateCur >= 0 );
            assert( pTrans->StateCur < pAut->nStates );
            pTrans->StateNext = pMapping[pTrans->StateNext];
            assert( pTrans->StateNext >= 0 );
            assert( pTrans->StateNext < pAut->nStates );
        }
        // assign the state name
        sprintf( Buffer, "s%0*d", nDigits, s ); 
        pAut->pStates[s]->pName = util_strsav(Buffer);
    }
    FREE( pMapping );

printf( "Synthesis: States = %d. Reachable = %d. Initial = %d. Transitions = %d.\n", 
       p->nStates, pAut->nStates, pAut->nInitial, Au_AutoCountTransitions(pAut) );

    FREE( pStateHasTrans );
    FREE( pStateIsUsed );
    FREE( pStatesVisit );
    return pAut;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



/**CFile****************************************************************

  FileName    [auBench.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [The benchmark generator for language solving.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: auBench.c,v 1.1 2004/02/19 03:06:43 alanmi Exp $]

***********************************************************************/

#include "auInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Au_AutoRemoveRandomTransition( Au_Auto_t * pAut );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Generated the specification FSM A for the given FSM C.]

  Description [The spec FSM A is derived by reducing some of the
  inputs and outputs of C and removing the transitions from A until 
  it is no longer true that "C is contained in A". The removed inputs
  become V (the signals going from X into C) and the removed outputs 
  become U (the signals going from C into X.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Au_Auto_t * Auto_AutoBenchmarkGenerate( Au_Auto_t * pAutIn, int nInsRemove, int nOutsRemove )
{
    Au_Auto_t * pAutOut;
    Au_Trans_t * pTrans;
    int nFsmIns, nFsmOuts;
    Mvc_Cover_t * pCover;
    Mvc_Cube_t * pCube, * pMask;
    char * pfRemaining;
    int Index, i;
//    int Status;

    // count the number of FSM inputs and outputs
    nFsmIns = 0;
    nFsmOuts = 0;
    for ( i = 0; i < pAutIn->nInputs; i++ )
        if ( pAutIn->ppNamesInput[i][0] == 'x' )
            nFsmIns++;
        else if ( pAutIn->ppNamesInput[i][0] == 'z' )
            nFsmOuts++;
    if ( nFsmIns + nFsmOuts != pAutIn->nInputs )
    {
        fprintf( stdout, "The FSM does not appear to have come from the SIS benchmark set.\n" );
        fprintf( stdout, "Currently, this generator does not work for such benchmarks.\n" );
        return NULL;
    }

    if ( nInsRemove >= nFsmIns )
    {
        fprintf( stdout, "Can only remove less inputs than are present (%d).\n", nFsmIns );
        return NULL;
    }
    if ( nOutsRemove >= nFsmOuts )
    {
        fprintf( stdout, "Can only remove less outputs than are present (%d).\n", nFsmOuts );
        return NULL;
    }

    srand( time(NULL) );

    // remove the I/O variables randomly
    pfRemaining = ALLOC( char, pAutIn->nInputs );
    memset( pfRemaining, 1, sizeof(char) * pAutIn->nInputs );
    for ( i = 0; i < nInsRemove; i++ )
    {
        Index = rand() % nFsmIns;
        while ( pfRemaining[Index] == 0 ) // already removed
            Index = rand() % nFsmIns;
        pfRemaining[Index] = 0;
        printf( "Removing input  \"%s\".\n", pAutIn->ppNamesInput[Index] );
    }
    for ( i = 0; i < nOutsRemove; i++ )
    {
        Index = rand() % nFsmOuts;
        while ( pfRemaining[nFsmIns + Index] == 0 ) // already removed
            Index = rand() % nFsmOuts;
        pfRemaining[nFsmIns + Index] = 0;
        printf( "Removing output \"%s\".\n", pAutIn->ppNamesInput[nFsmIns + Index] );
    }

    // create the reduced automaton
    pAutOut = Au_AutoDup( pAutIn );
    // create the mask to be removed
    pCover = pAutIn->pStates[0]->pHead->pCond;
    pMask = Mvc_CubeAlloc( pCover );
    Mvc_CubeBitClean( pMask );
    for ( i = 0; i < pAutIn->nInputs; i++ )
        if ( !pfRemaining[i] )
        {
            Mvc_CubeBitInsert( pMask, 2 * i     );
            Mvc_CubeBitInsert( pMask, 2 * i + 1 );
        }
    FREE( pfRemaining );

    // remove the corresponding variables from the conditions
    for ( i = 0; i < pAutOut->nStates; i++ )
        Au_StateForEachTransition( pAutOut->pStates[i], pTrans )
            Mvc_CoverForEachCube( pTrans->pCond, pCube )
            {
                Mvc_CubeBitOr( pCube, pCube, pMask );
            }
    Mvc_CubeFree( pCover, pMask );

    // this cannot be used now because there is a bug in 
    // checking containment of the ND automata
/*
    // remove transitions and check for containment
    Status = Au_AutoCheckProduct12( pAutIn, pAutOut );
    assert( Status == 0 );
    do 
    {
        Au_AutoRemoveRandomTransition( pAutOut );
    } 
    while ( !Au_AutoCheckProduct12( pAutIn, pAutOut ) );
*/

    return pAutOut;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Au_AutoRemoveRandomTransition( Au_Auto_t * pAut )
{
    Au_Trans_t * pTrans, * pTrans2, * pTransRem;
    Au_Trans_t ** ppTrans;
    int nTrans, iTrans, i;

    // put all the transitions into one array
    nTrans = Au_AutoCountProducts( pAut );
    ppTrans = ALLOC( Au_Trans_t *, nTrans );
    iTrans = 0;
    for ( i = 0; i < pAut->nStates; i++ )
        Au_StateForEachTransition( pAut->pStates[i], pTrans )
            ppTrans[ iTrans++ ] =  pTrans;
    assert( iTrans == nTrans );

    // pick one transition randomly
    iTrans = rand() % nTrans;

    // remove it
    pTransRem = ppTrans[ iTrans ];
    Au_StateForEachTransitionSafe( pAut->pStates[pTransRem->StateCur], pTrans, pTrans2 )
        if ( pTrans == pTransRem )
        {
            Au_AutoTransFree( pTrans );
            break;
        }
    FREE( ppTrans );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



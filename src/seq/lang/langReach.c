/**CFile****************************************************************

  FileName    [langReach.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Implicit language solving flow.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: langReach.c,v 1.3 2004/02/19 03:06:54 alanmi Exp $]

***********************************************************************/

#include "langInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes the set of reachable states.]

  Description [Uses the variable map of the relation, in which the
  ordering of variables is (PI,PO,CS,NS). Uses the relation of the
  automaton. Assigns the set of reachable states to pAut->bStates.
  Does not make any changes to other parts of the automaton data 
  structure.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lang_Reachability( Lang_Auto_t * pAut, bool fReorder, bool fVerbose )
{
    DdManager * dd;
    DdNode * bTrans, * bQuant, * bCurrent;
    DdNode * bNext, * bReached, * bTemp;
    DdNode * bCubeCs, * bCubeIO;
    int nIters;

    if ( pAut->nStates == 0 )
    {
        printf( "Trying to compute reachability for an automaton with no states.\n" );
        return;
    }

    // quantify the input/output variables in the old manager
    bCubeIO = Lang_AutoCharCubeIO( pAut->dd, pAut );                  Cudd_Ref( bCubeIO );
    bQuant  = Cudd_bddExistAbstract( pAut->dd, pAut->bRel, bCubeIO ); Cudd_Ref( bQuant );
    Cudd_RecursiveDeref( pAut->dd, bCubeIO );

    // create the new manager with the same ordering of variable
    dd = Cudd_Init( pAut->dd->size, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    Cudd_ShuffleHeap( dd, pAut->dd->invperm );

    // set up the internal variable replacement map: CS <-> NS
    Vmx_VarMapRemapRangeSetup( dd, pAut->pVmx, 
        pAut->nInputs + 0, pAut->nLatches, pAut->nInputs + pAut->nLatches, pAut->nLatches ); 

    // transfer the quantified relation 
    bTrans   = Cudd_bddTransfer( pAut->dd, dd, bQuant );              Cudd_Ref( bTrans );
    Cudd_RecursiveDeref( pAut->dd, bQuant );

    // get the initial state
    bCurrent = Cudd_bddTransfer( pAut->dd, dd, pAut->bInit );         Cudd_Ref( bCurrent );
    bCubeCs = Lang_AutoCharCubeCs( dd, pAut );                        Cudd_Ref( bCubeCs );

    // reorder the variables
    Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 100 );

    // perform reachability computation
    bReached = bCurrent;                                              Cudd_Ref( bReached );
    nIters = 0;
    while ( 1 )
    {
        nIters++;
        // compute the next states
//        bNext = Cudd_bddAndAbstract( dd, bTrans, bCurrent, bCubeInCs ); Cudd_Ref( bNext );
        bNext = Cudd_bddAndAbstract( dd, bTrans, bCurrent, bCubeCs ); Cudd_Ref( bNext );
        Cudd_RecursiveDeref( dd, bCurrent );
        // remap these states into the current state vars (NS->CS)
        bNext = Cudd_bddVarMap( dd, bTemp = bNext );                  Cudd_Ref( bNext );
        Cudd_RecursiveDeref( dd, bTemp );
        // check if there are any new states
        if ( Cudd_bddLeq( dd, bNext, bReached ) )
            break;
        // get the new states
        bCurrent = Cudd_bddAnd( dd, bNext, Cudd_Not(bReached) );      Cudd_Ref( bCurrent );
        // minimize the new states with the reached states
//        bCurrent = Cudd_bddConstrain( dd, bTemp = bCurrent, Cudd_Not(bReached) ); Cudd_Ref( bCurrent );
//        Cudd_RecursiveDeref( dd, bTemp );
        // add to the reached states
        bReached = Cudd_bddOr( dd, bTemp = bReached, bNext );         Cudd_Ref( bReached );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bNext );
        // minimize the transition relation
//        bTrans = Cudd_bddConstrain( dd, bTemp = bTrans, Cudd_Not(bReached) ); Cudd_Ref( bTrans );
//        Cudd_RecursiveDeref( dd, bTemp );
    }
    Cudd_RecursiveDeref( dd, bCubeCs );
    Cudd_RecursiveDeref( dd, bTrans );
    Cudd_RecursiveDeref( dd, bNext );
//    Cudd_RecursiveDeref( dd, bCubeInCs ); 
    fprintf( stdout, "Reachability analysis completed in %d iterations.\n", nIters );

    // transfer the reachable states back
    if ( pAut->bStates )
        Cudd_RecursiveDeref( pAut->dd, pAut->bStates );
    pAut->bStates = Cudd_bddTransfer( dd, pAut->dd, bReached );  Cudd_Ref( pAut->bStates );
    Cudd_RecursiveDeref( dd, bReached );

    // get rid of the old manager
    Extra_StopManager( dd );

    // reorder the original manager
//    Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 100 );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



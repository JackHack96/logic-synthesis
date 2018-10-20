/**CFile****************************************************************

  FileName    [langCheck.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Implicit language solving flow.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: langCheck.c,v 1.4 2004/02/19 03:06:53 alanmi Exp $]

***********************************************************************/

#include "langInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Checks the containment of behavior for two ND automata.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lang_AutoCheck( FILE * pOut, Lang_Auto_t * pAut1, Lang_Auto_t * pAut2, bool fError )
{
    Lang_Auto_t * pAutP, * pAutTemp;
    DdManager * dd;
    DdNode * bAcc1, * bAcc2;
    DdNode * bAcc01, * bAcc10;
    DdNode * bTrans, * bCurrent, * bNext, * bReached, * bTemp;
    DdNode * bCubeIO, * bCubeCs, * bCubeCs1, * bCubeCs2;
    bool fAut1Nd, fAut2Nd;
    int iVarCS1, iVarCS2, iVarNS1, iVarNS2;
    int nLat1, nLat2, nLatTot;
    int fAut1ContainsAut2 = 1;
    int fAut2ContainsAut1 = 1;
    int nIters;

    // as Jordi pointed out, this procedure only works for deterministic automata
    fAut1Nd = Lang_AutoCheckNd( stdout, pAut1, pAut1->nInputs, 0 );
    fAut2Nd = Lang_AutoCheckNd( stdout, pAut2, pAut2->nInputs, 0 );
    if ( fAut1Nd )
        printf( "Automaton \"%s\" is non-deterministic.\n", pAut1->pName );
    if ( fAut2Nd )
        printf( "Automaton \"%s\" is non-deterministic.\n", pAut2->pName );
    if ( fAut1Nd || fAut2Nd )
    {
        printf( "Checking cannot be performed.\n" );
        return;
    }

    // swap the automata before computing the product
    if ( pAut1->nStateBits < pAut2->nStateBits )
    {
        pAutTemp = pAut1;
        pAut1    = pAut2;
        pAut2    = pAutTemp;
    }

    // complete both automata
    if ( Lang_AutoComplete( pAut1, pAut1->nInputs, 0, 0 ) )
        printf( "Warning: Automaton \"%s\" is completed before checking.\n", pAut1->pName );
    if ( Lang_AutoComplete( pAut2, pAut2->nInputs, 0, 0 ) )
        printf( "Warning: Automaton \"%s\" is completed before checking.\n", pAut2->pName );

    // compute the incomplete product (without reachability and reencoding)
    pAutP = Lang_AutoProduct( pAut1, pAut2, 0, 1 );
    if ( pAutP == NULL )
        return;
    dd = pAutP->dd;
//Vmx_VarMapPrint( pAut1->pVmx );
//Vmx_VarMapPrint( pAut2->pVmx );
//Vmx_VarMapPrint( pAutP->pVmx );

    // the product automaton has the following arrangment of variables
    // IO1, new(IO2), CS1, CS2, NS1, NS2

    // get the number of latches
    nLat1   = pAut1->nLatches;
    nLat2   = pAut2->nLatches;
    nLatTot = nLat1 + nLat2;
    // mark up the CS/NS variables
    iVarCS1 = pAutP->nInputs;
    iVarCS2 = pAutP->nInputs + nLat1;
    iVarNS1 = pAutP->nInputs + nLatTot;
    iVarNS2 = pAutP->nInputs + nLatTot + nLat1;

    // the set of accepting state of pAutP->bAccepting is expressed using CS1 + CS2 vars
    // get the accepting states of Aut1 by quantifying CS2
    bCubeCs2 = Vmx_VarMapCharCubeRange( dd, pAutP->pVmx, iVarCS2, nLat2 );  Cudd_Ref( bCubeCs2 );
    bAcc1 = Cudd_bddExistAbstract( dd, pAutP->bAccepting, bCubeCs2 );       Cudd_Ref( bAcc1 );
    Cudd_RecursiveDeref( dd, bCubeCs2 );
    // get the accepting states of Aut2 by quantifying CS1
    bCubeCs1 = Vmx_VarMapCharCubeRange( dd, pAutP->pVmx, iVarCS1, nLat1 );  Cudd_Ref( bCubeCs1 );
    bAcc2 = Cudd_bddExistAbstract( dd, pAutP->bAccepting, bCubeCs1 );       Cudd_Ref( bAcc2 );
    Cudd_RecursiveDeref( dd, bCubeCs1 );
//PRB( dd, bAcc1 );
//PRB( dd, bAcc2 );
    // get the two types of special states of the product machine
    bAcc01 = Cudd_bddAnd( dd,          bAcc1 , Cudd_Not(bAcc2) );      Cudd_Ref( bAcc01 );
    bAcc10 = Cudd_bddAnd( dd, Cudd_Not(bAcc1),          bAcc2  );      Cudd_Ref( bAcc10 );
    Cudd_RecursiveDeref( dd, bAcc1 );
    Cudd_RecursiveDeref( dd, bAcc2 );
//PRB( dd, bAcc01 );
//PRB( dd, bAcc10 );

    // set up replacement of CS by NS in the product machine
    Vmx_VarMapRemapRangeSetup( dd, pAutP->pVmx, iVarCS1, nLatTot, iVarNS1, nLatTot ); 

    // quantify the input/output variables
    bCubeIO = Lang_AutoCharCubeIO( dd, pAutP );                            Cudd_Ref( bCubeIO );
    bTrans  = Cudd_bddExistAbstract( dd, pAutP->bRel, bCubeIO );       Cudd_Ref( bTrans );
    Cudd_RecursiveDeref( dd, bCubeIO );
    Cudd_RecursiveDeref( dd, pAutP->bRel );
    pAutP->bRel = NULL;

    // reorder the variables
    Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 100 );

    // get the cube of the CS variables
    bCubeCs = Vmx_VarMapCharCubeRange( dd, pAutP->pVmx, iVarCS1, nLatTot );          Cudd_Ref( bCubeCs );

    // check the initial state
    if ( !Cudd_bddLeq( dd, pAutP->bInit, Cudd_Not(bAcc10) ) )
        fAut1ContainsAut2 = 0;
    if ( !Cudd_bddLeq( dd, pAutP->bInit, Cudd_Not(bAcc01) ) )
        fAut2ContainsAut1 = 0;
//PRB( dd, pAutP->bInit );

    // perform reachability analysis of the product machine
    bCurrent = pAutP->bInit;   Cudd_Ref( bCurrent );
    bReached = bCurrent;       Cudd_Ref( bReached );
    nIters = 0;
    while ( 1 )
    {
        nIters++;
        // compute the next states
        bNext = Cudd_bddAndAbstract( dd, bTrans, bCurrent, bCubeCs ); Cudd_Ref( bNext );
        Cudd_RecursiveDeref( dd, bCurrent );
        // remap these states into the current state vars
        bNext = Cudd_bddVarMap( dd, bTemp = bNext );                  Cudd_Ref( bNext );
        Cudd_RecursiveDeref( dd, bTemp );
//PRB( dd, bNext );

        // update the containment flags
        if ( !Cudd_bddLeq( dd, bNext, Cudd_Not(bAcc10) ) )
            fAut1ContainsAut2 = 0;
        if ( !Cudd_bddLeq( dd, bNext, Cudd_Not(bAcc01) ) )
            fAut2ContainsAut1 = 0;
        // check if the problem is solved
        if ( !fAut1ContainsAut2 && !fAut2ContainsAut1 )
        {
            fprintf( stdout, "There is no behavior containment.\n" );
            break;
        }
        // check if there are any new states
        if ( Cudd_bddLeq( dd, bNext, bReached ) )
        {
            if ( fAut1ContainsAut2 && fAut2ContainsAut1 )
                fprintf( stdout, "Automata \"%s\" and \"%s\" are sequentially equivalent.\n", pAut1->pName, pAut2->pName );
            else if ( fAut1ContainsAut2 )
                fprintf( stdout, "Automaton \"%s\" contains automaton \"%s\".\n", pAut1->pName, pAut2->pName );
            else if ( fAut2ContainsAut1 )
                fprintf( stdout, "Automaton \"%s\" contains automaton \"%s\".\n", pAut2->pName, pAut1->pName );
            break;
        }
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
    Cudd_RecursiveDeref( dd, bReached );
    Cudd_RecursiveDeref( dd, bTrans );
    Cudd_RecursiveDeref( dd, bNext );
    fprintf( stdout, "Reachability analysis completed in %d iterations.\n", nIters );

    Cudd_RecursiveDeref( dd, bAcc01 );
    Cudd_RecursiveDeref( dd, bAcc10 );

    // get rid of the temporary automaton
    Lang_AutoFree( pAutP );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



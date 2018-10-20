/**CFile****************************************************************

  FileName    [langOper.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Implicit language solving flow.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: langOper.c,v 1.4 2004/02/19 03:06:54 alanmi Exp $]

***********************************************************************/

#include "langInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Prints statistics of the automaton.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lang_AutoPrintStats( FILE * pOut, Lang_Auto_t * pAut, int nInputs )
{
    char Buffer1[20], Buffer2[20];
    int nStatesIncomp, nStatesNonDet;
    int c;

    if ( pAut->nStates == 0 )
    {
        printf( "Trying to print stats for an automaton with no states.\n" );
        return;
    }

    // the first line
    nStatesIncomp = Lang_AutoComplete( pAut, nInputs, 0, 1 );
    nStatesNonDet = Lang_AutoCheckNd( stdout, pAut, nInputs, 0 );
    sprintf( Buffer1, " (%d states)", nStatesIncomp );
    sprintf( Buffer2, " (%d states)", nStatesNonDet );

	fprintf( pOut, "%s: The automaton is %s%s and %s%s.\n", pAut->pName,  
        (nStatesIncomp? "incomplete" : "complete"),
        (nStatesIncomp? Buffer1 : ""),
        (nStatesNonDet? "non-deterministic": "deterministic"),
        (nStatesNonDet? Buffer2 : "")     );

    // the third line
    fprintf( pOut, "%d inputs  ",       pAut->nInputs );
	fprintf( pOut, "%d state bits  ",   pAut->nStateBits );
	fprintf( pOut, "%d states  ",       pAut->nStates );
	fprintf( pOut, "%d transitions  ",  Lang_AutoCountTransitions(pAut) );
	fprintf( pOut, "%d BDD nodes\n",    Cudd_DagSize( pAut->bRel ) );

    // the second line
    fprintf( pOut, "Inputs = { ",   pAut->nInputs );
    for ( c = 0; c < pAut->nInputs; c++ )
        fprintf( pOut, "%s%s", ((c==0)? "": ","),  pAut->ppNamesInput[c] );
    fprintf( pOut, " }\n" );
}


/**Function*************************************************************

  Synopsis    [Completes the relation of the automaton.]

  Description [Returns the number of incomplete states; 0 if it was not needed.
  Completion consists in updating the transition relation and adding one
  state to the set of reachable states. The new state is non-accepting
  by default. If the flag "fAccepting" is set, the new state is accepting.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lang_AutoComplete( Lang_Auto_t * pAut, int nInputs, int fAccepting, int fCheckOnly )
{
    DdManager * dd = pAut->dd;
    DdNode * bIncomp, * bProd, * bTemp;
    DdNode * bCodeDcCs, * bCodeDcNs, * bCubeIO, * bRelation;
    int nStates;

    if ( pAut->nStates == 0 )
    {
        printf( "Trying to complete an automaton with no states.\n" );
        return 0;
    }

    // quantify the extra IO vars
    bCubeIO = Vmx_VarMapCharCubeRange( dd, pAut->pVmx, nInputs, pAut->nInputs - nInputs );  Cudd_Ref( bCubeIO );
    bRelation = Cudd_bddExistAbstract( dd, pAut->bRel, bCubeIO );     Cudd_Ref( bRelation );
    Cudd_RecursiveDeref( dd, bCubeIO );

    // get the incomplete states
    if ( fCheckOnly )
        bIncomp = Lang_AutoIncompleteStates( pAut, bRelation, pAut->bStates );  
    else
        bIncomp = Lang_AutoIncompleteDomain( pAut, bRelation, pAut->bStates );  
    Cudd_Ref( bIncomp );
    Cudd_RecursiveDeref( dd, bRelation );

    // if the undefined domain is empty, completion is not needed
    if ( bIncomp == b0 )
    {
        Cudd_RecursiveDeref( dd, bIncomp );
        return 0;
    }

    if ( fCheckOnly )
    {
        // count the number of incomplete states
        nStates = (int)Cudd_CountMinterm( dd, bIncomp, pAut->nStateBits );
        Cudd_RecursiveDeref( dd, bIncomp );
        return nStates;
    }

    // get the CS/NS codes of the DC state
    bCodeDcCs = Lang_AutoGetUnusedCodeMinterm( pAut );                Cudd_Ref( bCodeDcCs );
    bCodeDcNs = Vmx_VarMapRemapRange( dd, bCodeDcCs, pAut->pVmx, 
        pAut->nInputs + 0, pAut->nLatches, pAut->nInputs + pAut->nLatches, pAut->nLatches );  Cudd_Ref( bCodeDcNs );

    // redirect the undefined domain to the DC state
    bProd = Cudd_bddAnd( dd, bIncomp, bCodeDcNs );                    Cudd_Ref( bProd );
    Cudd_RecursiveDeref( dd, bIncomp );
    // add these transitions to the relation
    pAut->bRel = Cudd_bddOr( dd, bTemp = pAut->bRel, bProd );         Cudd_Ref( pAut->bRel );
    Cudd_RecursiveDeref( dd, bTemp );
    Cudd_RecursiveDeref( dd, bProd );

    // add the transitions from the DC state into itself
    bProd = Cudd_bddAnd( dd, bCodeDcCs, bCodeDcNs );                  Cudd_Ref( bProd );
    pAut->bRel = Cudd_bddOr( dd, bTemp = pAut->bRel, bProd );         Cudd_Ref( pAut->bRel );
    Cudd_RecursiveDeref( dd, bTemp );
    Cudd_RecursiveDeref( dd, bProd );

    // add the DC state to the reachable states
    pAut->bStates = Cudd_bddOr( dd, bTemp = pAut->bStates, bCodeDcCs );  Cudd_Ref( pAut->bStates );
    Cudd_RecursiveDeref( dd, bTemp );

    // select the name for the new non-accepting state
    if ( pAut->tCode2Name )
    {
        st_insert( pAut->tCode2Name, (char *)bCodeDcCs, Lang_AutoGetDcStateName(pAut) );
        Cudd_Ref( bCodeDcCs );
    }

    // if the state is accepting, add it to the set of accepting states
    if ( fAccepting )
    {
        pAut->bAccepting = Cudd_bddOr( dd, bTemp = pAut->bAccepting, bCodeDcCs );  
        Cudd_Ref( pAut->bAccepting );
        Cudd_RecursiveDeref( dd, bTemp );
        pAut->nAccepting++;
    }

    Cudd_RecursiveDeref( dd, bCodeDcCs );
    Cudd_RecursiveDeref( dd, bCodeDcNs );

    // increment the number of states
    pAut->nStates++;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns the incomplete IO-CS domain.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Lang_AutoIncompleteDomain( Lang_Auto_t * pAut, DdNode * bRelation, DdNode * bReach )
{
    DdManager * dd = pAut->dd;
    DdNode * bCubeNs, * bRelUndef, * bTemp;

    // find the domain where the transition is not defined
    bCubeNs   = Lang_AutoCharCubeNs( dd, pAut );                        Cudd_Ref( bCubeNs );
    bRelUndef = Cudd_bddExistAbstract( dd, bRelation, bCubeNs );        Cudd_Ref( bRelUndef );
    bRelUndef = Cudd_Not( bRelUndef );
    Cudd_RecursiveDeref( dd, bCubeNs );

    // restrict to only reachable states
    bRelUndef = Cudd_bddAnd( dd, bTemp = bRelUndef, bReach );           Cudd_Ref( bRelUndef );
    Cudd_RecursiveDeref( dd, bTemp );

    Cudd_Deref( bRelUndef );
    return bRelUndef;
}

/**Function*************************************************************

  Synopsis    [Returns the set of incomplete states.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Lang_AutoIncompleteStates( Lang_Auto_t * pAut, DdNode * bRelation, DdNode * bReach )
{
    DdManager * dd = pAut->dd;
    DdNode * bCubeIO, * bRelUndef, * bTemp;

    // get the incomplete domain
    bRelUndef = Lang_AutoIncompleteDomain( pAut, bRelation, bReach );  Cudd_Ref( bRelUndef );

    // if the undefined domain is empty, completion is not needed
    if ( bRelUndef != b0 )
    {
        // remove the input variables
        bCubeIO   = Lang_AutoCharCubeIO( dd, pAut );                         Cudd_Ref( bCubeIO );
        bRelUndef = Cudd_bddExistAbstract( dd, bTemp = bRelUndef, bCubeIO ); Cudd_Ref( bRelUndef );
        Cudd_RecursiveDeref( dd, bCubeIO );
        Cudd_RecursiveDeref( dd, bTemp );
    }
    Cudd_Deref( bRelUndef );
    return bRelUndef;
}



/**Function*************************************************************

  Synopsis    [Returns 1 if the automaton is non-deterministic.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lang_AutoCheckNd( FILE * pOut, Lang_Auto_t * pAut, int nInputs, bool fVerbose )
{
    DdManager * dd = pAut->dd;
    DdNode * bCubeIO, * bCubeNs, * bTemp;
    DdNode * bRel, * bProj, * bDiff, * bQuant;
    int RetValue;

//Vmx_VarMapPrint( pAut->pVmx );
    // quantify the extra IO vars
    bCubeIO = Vmx_VarMapCharCubeRange( dd, pAut->pVmx, nInputs, pAut->nInputs - nInputs );  Cudd_Ref( bCubeIO );
    bRel    = Cudd_bddExistAbstract( dd, pAut->bRel, bCubeIO );                             Cudd_Ref( bRel );
    Cudd_RecursiveDeref( dd, bCubeIO );
    // get the compatible projection w.r.t. NS vars
    bCubeNs = Lang_AutoCharCubeNs( dd, pAut );                                              Cudd_Ref( bCubeNs );
    bProj = Cudd_CProjection( dd, bRel, bCubeNs );                                          Cudd_Ref( bProj );

    // compute compatible projection w.r.t these vars
    if ( bProj == bRel )
        RetValue = 0;
    else
    {
        // get the current states, for which the projection is different
        bDiff   = Cudd_bddAnd( dd, bRel, Cudd_Not(bProj) );                     Cudd_Ref( bDiff );
        bQuant  = Cudd_bddExistAbstract( dd, bDiff, bCubeNs );                  Cudd_Ref( bQuant );
        Cudd_RecursiveDeref( dd, bDiff );

        bCubeIO = Lang_AutoCharCubeIO( dd, pAut );                              Cudd_Ref( bCubeIO );
        bQuant  = Cudd_bddExistAbstract( dd, bTemp = bQuant, bCubeIO );         Cudd_Ref( bQuant );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bCubeIO );

        RetValue = (int)Cudd_CountMinterm( dd, bQuant, pAut->nStateBits );
        Cudd_RecursiveDeref( dd, bQuant );
    }
    Cudd_RecursiveDeref( dd, bRel );
    Cudd_RecursiveDeref( dd, bProj );
    Cudd_RecursiveDeref( dd, bCubeNs );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lang_AutoComplement( Lang_Auto_t * pAut )
{
    DdNode * bTemp;
    if ( pAut->nStates == 0 )
    {
        printf( "Trying to complement an automaton with no states.\n" );
        return 1;
    }
    pAut->bAccepting = Cudd_bddAnd( pAut->dd, pAut->bStates, bTemp = Cudd_Not(pAut->bAccepting) );
    Cudd_Ref( pAut->bAccepting );
    Cudd_RecursiveDeref( pAut->dd, bTemp );

    pAut->nAccepting = (int)Cudd_CountMinterm( pAut->dd, pAut->bAccepting, pAut->nStateBits );
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



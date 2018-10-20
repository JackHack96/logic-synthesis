/**CFile****************************************************************

  FileName    [autFormula.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Derives L language formula for the given automaton.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 18, 2003.]

  Revision    [$Id: dualForm.c,v 1.2 2004/02/19 03:06:52 alanmi Exp $]

***********************************************************************/

#include "auInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void     Dual_ComputeCharacterization( DdManager * dd, Au_Auto_t * pAutR, int Degree );
static DdNode * Dual_ComputeCharacterization_rec( DdManager * dd, Au_Auto_t * pAutR, Au_State_t * pState, int Degree );
static void     Dual_PrintCharacterization( DdManager * dd, Au_Auto_t * pAut );
static int      Dual_CheckDisjointness( DdManager * dd, Au_Auto_t * pAutR );
static void     Dual_ComputeConditions( DdManager * dd, Au_Auto_t * pAut, Vmx_VarMap_t * pVmx );
static void     Dual_EraseFunctions( DdManager * dd, Au_Auto_t * pAut );
static void     Dual_DeriveMvc( DdManager * dd, Au_Auto_t * pAut, Au_Auto_t * pAutR, int Degree, Vmx_VarMap_t * pVmx, Vmx_VarMap_t * pVmxR );
static void     Dual_ResizeManager( DdManager * dd, int nVars );
static void     Dual_MarkStates( Au_Auto_t * pAut );
static void     Dual_WritePlaFile( Au_Auto_t * pAut, int Degree, char * FileName );
static void     Dual_WriteBlifFile( Au_Auto_t * pAut, int Degree, char * FileName );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////
// formula ftest2.aut 1.pla

/**Function*************************************************************

  Synopsis    [Find state characterizations for each state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dual_ConvertAut2Formula( Au_Auto_t * pAut, char * pFileName, int Rank )
{
    Au_Auto_t * pAutR;
    Vm_VarMap_t * pVm, * pVmR;
    Vmx_VarMap_t * pVmx, * pVmxR;
    DdManager * dd;
    int Degree;

    if ( pAut->nStates == 0 )
    {
        printf( "Trying to consider the automaton with no states.\n" );
        return 1;
    }

    // construct the automaton with the reversed transitions
    pAutR = Au_AutoReverse( pAut );

    // get the manager and expand it if necessary
    dd = Cudd_Init( pAut->nInputs, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    // get the var maps
    pVm = Vm_VarMapCreateBinary( Fnc_ManagerReadManVm(pAut->pMan), pAut->nInputs, 0 );
    pVmx = Vmx_VarMapLookup( Fnc_ManagerReadManVmx(pAut->pMan), pVm, -1, NULL );

    // compute the conditions in both automata
    Dual_ComputeConditions( dd, pAut,  pVmx );
    Dual_ComputeConditions( dd, pAutR, pVmx );

    Dual_MarkStates( pAut );
    Dual_MarkStates( pAutR );

    Dual_ComputeCharacterization( dd, pAut, 1 );

//    Cudd_AutodynEnable( dd, CUDD_REORDER_SYMM_SIFT );

    // try to find the fixed point
    Degree = 0;
    while ( 1 )
    {
        Degree++;
        // resize the manager
        Dual_ResizeManager( dd, Degree * pAut->nInputs );
        // compute characterization of the marked states
        Dual_ComputeCharacterization( dd, pAutR, Degree );
//Dual_PrintCharacterization( dd, pAutR );

        // check whether the states are disjoint
        if ( Dual_CheckDisjointness( dd, pAutR ) )
            break;
        // the marked state are those that do not have disjointness
        if ( Degree == Rank )
        {
            printf( "This automaton is not a finite memory automaton (FMA) of rank less than %d. No file written.\n", Rank );
            Dual_EraseFunctions( dd, pAutR );
            Dual_EraseFunctions( dd, pAut );
            Extra_StopManager( dd );
            return 0;
        }
    }
    printf( "Found FMA with the degree equal to %d.\n", Degree );

    // get the var maps
    pVmR = Vm_VarMapCreateBinary( Fnc_ManagerReadManVm(pAut->pMan), Degree * pAut->nInputs, 0 );
    pVmxR = Vmx_VarMapLookup( Fnc_ManagerReadManVmx(pAut->pMan), pVmR, -1, NULL );

//    Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 100 );
//    Cudd_AutodynDisable( dd );

    // in each state save two functions: F(x) and f(x)
    Dual_DeriveMvc( dd, pAut, pAutR, Degree, pVmx, pVmxR );

    // write the output file
//    Dual_WritePlaFile( pAut, Degree, pFileName );
    Dual_WriteBlifFile( pAut, Degree, pFileName );

    Dual_EraseFunctions( dd, pAutR );
    Dual_EraseFunctions( dd, pAut );
    Extra_StopManager( dd );
    Au_AutoFree( pAutR );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Compute the condition for each transition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dual_ComputeConditions( DdManager * dd, Au_Auto_t * pAut, Vmx_VarMap_t * pVmx )
{
    Au_State_t * pState;
    Au_Trans_t * pTrans;
    DdNode ** pbCodes;
    int s;

    // get the codes
    pbCodes = Vmx_VarMapEncodeMap( dd, pVmx );
    // compute the conditions for each transition
    for ( s = 0; s < pAut->nStates; s++ )
    {
        pState = pAut->pStates[s];
        Au_StateForEachTransition( pState, pTrans )
        {
            pTrans->bCond = Fnc_FunctionDeriveMddFromSop( dd, Vmx_VarMapReadVm(pVmx), pTrans->pCond, pbCodes );  
            Cudd_Ref( pTrans->bCond );
        }
    }
    Vmx_VarMapEncodeDeref( dd, pVmx, pbCodes );
}

/**Function*************************************************************

  Synopsis    [Rereferences the functions in states and transitions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dual_EraseFunctions( DdManager * dd, Au_Auto_t * pAut )
{
    Au_State_t * pState;
    Au_Trans_t * pTrans;
    int s;
    for ( s = 0; s < pAut->nStates; s++ )
    {
        // get the state
        pState = pAut->pStates[s];
        // deref the characterization
        if ( pState->bTrans )
            Cudd_RecursiveDeref( dd, pState->bTrans );
        // derefe the conditions
        Au_StateForEachTransition( pState, pTrans )
        {
            if ( pTrans->bCond )
                Cudd_RecursiveDeref( dd, pTrans->bCond );
        }
    }
}

/**Function*************************************************************

  Synopsis    [Resive manager if necessary.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dual_ResizeManager( DdManager * dd, int nVars )
{
    int v;
    if ( dd->size < nVars )
    {
        for ( v = dd->size; v < nVars; v++ )
            Cudd_bddNewVar( dd );
    }
}

/**Function*************************************************************

  Synopsis    [For each marked state, compute its characterization.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dual_ComputeCharacterization( DdManager * dd, Au_Auto_t * pAut, int Degree )
{
    Au_State_t * pState;
    DdNode * bSum;
    int s;    
    // for each state, compute its 1-characterization
    for ( s = 0; s < pAut->nStates; s++ )
    {
        // get the state
        pState = pAut->pStates[s];
        // skip of the state is not marked
        if ( pState->fMark == 0 )
            continue;

        // compute characterization for the given state
        bSum = Dual_ComputeCharacterization_rec( dd, pAut, pState, Degree );  Cudd_Ref( bSum );
//PRB( dd, bSum );

        if ( pState->bTrans )
            Cudd_RecursiveDeref( dd, pState->bTrans );
        pState->bTrans = bSum; // takes ref
    }
}

/**Function*************************************************************

  Synopsis    [For each marked state, compute its characterization.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Dual_ComputeCharacterization_rec( DdManager * dd, Au_Auto_t * pAut, Au_State_t * pState, int Degree )
{
    Au_Trans_t * pTrans;
    DdNode * bSum, * bTemp, * bChar, * bProd;

    // consider the trivial case
    if ( Degree == 0 )
        return b1;
    // go through the transitions and add them to characterization
    bSum = b0;    Cudd_Ref( bSum );
    Au_StateForEachTransition( pState, pTrans )
    {
        // get the characterization of the next state
        bChar = Dual_ComputeCharacterization_rec( dd, pAut,
            pAut->pStates[pTrans->StateNext], Degree - 1 );        Cudd_Ref( bChar );
        // shift it up one level
        bChar = Extra_bddMove( dd, bTemp = bChar, pAut->nInputs ); Cudd_Ref( bChar );
        Cudd_RecursiveDeref( dd, bTemp );
        // create the product with the condition of this transition
        bProd = Cudd_bddAnd( dd, bChar, pTrans->bCond );           Cudd_Ref( bProd );
        Cudd_RecursiveDeref( dd, bChar );
        // add this to the sum of characterizations
        bSum  = Cudd_bddOr( dd, bTemp = bSum, bProd );             Cudd_Ref( bSum );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bProd );
    }
    Cudd_Deref( bSum );
    return bSum;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dual_PrintCharacterization( DdManager * dd, Au_Auto_t * pAut )
{
    Au_State_t * pState;
    int s;    
    // for each state, compute its 1-characterization
    for ( s = 0; s < pAut->nStates; s++ )
    {
        // get the state
        pState = pAut->pStates[s];
PRB( dd, pState->bTrans );
    }
}


/**Function*************************************************************

  Synopsis    [Check the disjointness of states.]

  Description [When this function is called, the non-disjoint state
  are marked. This function check them for disjointness with other
  marked states. If a state is found to be disjoint, it is unmarked.
  Otherwise, it remains marked. Returns 1 if the resulting automaton 
  is disjoint; otherwise returns 0. In case the automaton is not disjoint,
  the non-disjoint states are marked with pState->fMark = 1.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dual_CheckDisjointness( DdManager * dd, Au_Auto_t * pAut )
{
    Au_State_t * pS1, * pS2;
    int s, t;
    int fDisjoint = 1;

    // mark the states that are non-disjoint w.r.t. other states
    for ( s = 0; s < pAut->nStates; s++ )
    {
        // do not try the state if it is known to be disjoint
        pS1 = pAut->pStates[s];
        if ( pS1->fMark == 0 )
            continue;
        // try all other states
        for ( t = 0; t < pAut->nStates; t++ )
        {
            if ( s == t )
                continue;
            // do not try the state if it is known to be disjoint
            pS2 = pAut->pStates[t];
            if ( pS2->fMark == 0 )
                continue;
//PRB( dd, pS1->bTrans );
//PRB( dd, pS2->bTrans );
            // check if these two states are disjoint
            if ( !Cudd_bddLeq( dd, pS1->bTrans, Cudd_Not(pS2->bTrans) ) ) // they overlap
            {
                fDisjoint = 0;
                break;
            }
        }
        // if we tried all and did not find overlapping, unmark it
        if ( t == pAut->nStates )
            pS1->fMark = 0;
    }
    return fDisjoint;
}

/**Function*************************************************************

  Synopsis    [Compute the Mvc_Cover_t * for F(x) and f(x).]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dual_DeriveMvc( DdManager * dd, Au_Auto_t * pAut, Au_Auto_t * pAutR, 
    int Degree, Vmx_VarMap_t * pVmx, Vmx_VarMap_t * pVmxR )
{
    Au_State_t * pState, * pStateR;
    Mvc_Cover_t * pCover1, * pCover2;
    int s;

    // for each state, compute its 1-characterization
    for ( s = 0; s < pAut->nStates; s++ )
    {
        // get the characterization from pAutR
        pStateR = pAutR->pStates[s];
//PRB( dd, pStateR->bTrans );
        pCover1 = Fnc_FunctionDeriveSopFromMddSpecial( Fnc_ManagerReadManMvc(pAut->pMan), 
            dd, pStateR->bTrans, pStateR->bTrans, pVmxR, Degree * pAut->nInputs );
        // get the transition domain from pAut
        pState  = pAut->pStates[s];
//PRB( dd, pState->bTrans );
        pCover2 = Fnc_FunctionDeriveSopFromMddSpecial( Fnc_ManagerReadManMvc(pAut->pMan), 
            dd, pState->bTrans, pState->bTrans, pVmx, pAut->nInputs );

        // save the characterization and domain
        pState->State1 = (int)pCover1;
        pState->State2 = (int)pCover2;
    }
}

/**Function*************************************************************

  Synopsis    [Compute the condition for each transition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dual_MarkStates( Au_Auto_t * pAut )
{
    Au_State_t * pState;
    int s;
    // compute the conditions for each transition
    for ( s = 0; s < pAut->nStates; s++ )
    {
        pState = pAut->pStates[s];
        pState->fMark = 1;
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dual_WritePlaFile( Au_Auto_t * pAut, int Degree, char * FileName )
{
    FILE * pFile;
    Au_State_t * pState;
    Mvc_Cover_t * pCover1, * pCover2;
    Mvc_Cube_t * pCube1, * pCube2;
    int nProducts, Value, d, v, s;

    // count the number of products
    nProducts = 0;
    for ( s = 0; s < pAut->nStates; s++ )
    {
        pState  = pAut->pStates[s];
        pCover1 = (Mvc_Cover_t *)pState->State1;
        pCover2 = (Mvc_Cover_t *)pState->State2;
        nProducts += Mvc_CoverReadCubeNum(pCover1) * Mvc_CoverReadCubeNum(pCover2);
    }
    
    pFile = fopen( FileName, "w" );
    fprintf( pFile, "# L language formula of degree %d for the automaton \"%s\"\n", Degree, pAut->pName );
    fprintf( pFile, ".i %d\n", pAut->nInputs * (Degree + 1) );
    fprintf( pFile, ".o %d\n", 1 );
    fprintf( pFile, ".p %d\n", nProducts );
    fprintf( pFile, ".ilb" );
    // write the names
    for ( d = Degree; d >= 0; d-- )
    {
        for ( v = 0; v < pAut->nInputs; v++ )
            if ( d )
                fprintf( pFile, " %s%d", pAut->ppNamesInput[v], d );
            else
                fprintf( pFile, " %s", pAut->ppNamesInput[v] );
    }
    fprintf( pFile, "\n" );
    fprintf( pFile, ".ob Out\n" );

    // write the products
    for ( s = 0; s < pAut->nStates; s++ )
    {
        pState  = pAut->pStates[s];
        pCover1 = (Mvc_Cover_t *)pState->State1;
        pCover2 = (Mvc_Cover_t *)pState->State2;
        pState->State1 = 0;
        pState->State2 = 0;

        // for each product of the first set, write all products from the second
        Mvc_CoverForEachCube( pCover1, pCube1 )
        {
            Mvc_CoverForEachCube( pCover2, pCube2 )
            {
                // write the first product
                for ( d = Degree - 1; d >= 0; d-- )
                {
                    for ( v = 0; v < pAut->nInputs; v++ )
                    {
                        Value = Mvc_CubeVarValue( pCube1, d * pAut->nInputs + v );
                        if ( Value == 1 )
                            fputc( '0', pFile );
                        else if ( Value == 2 )
                            fputc( '1', pFile );
                        else if ( Value == 3 )
                            fputc( '-', pFile );
                        else
                        {
                            assert( 0 );
                        }
                    }
                    fprintf( pFile, " " );
                }

                // write the second product
                for ( v = 0; v < pAut->nInputs; v++ )
                {
                    Value = Mvc_CubeVarValue( pCube2, v );
                    if ( Value == 1 )
                        fputc( '0', pFile );
                    else if ( Value == 2 )
                        fputc( '1', pFile );
                    else if ( Value == 3 )
                        fputc( '-', pFile );
                    else
                    {
                        assert( 0 );
                    }
                }
                fprintf( pFile, "  1\n" );
            }
        }
        Mvc_CoverFree( pCover1 );
        Mvc_CoverFree( pCover2 );
    }

    fprintf( pFile, ".e\n" );
    fclose( pFile );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dual_WriteBlifFile( Au_Auto_t * pAut, int Degree, char * FileName )
{
    FILE * pFile;
    Au_State_t * pState;
    Mvc_Cover_t * pCover1, * pCover2;
    Mvc_Cube_t * pCube1, * pCube2;
    int nProducts, Value, d, v, s;

    // count the number of products
    nProducts = 0;
    for ( s = 0; s < pAut->nStates; s++ )
    {
        pState  = pAut->pStates[s];
        pCover1 = (Mvc_Cover_t *)pState->State1;
        pCover2 = (Mvc_Cover_t *)pState->State2;
        nProducts += Mvc_CoverReadCubeNum(pCover1) * Mvc_CoverReadCubeNum(pCover2);
    }
    
    pFile = fopen( FileName, "w" );
    fprintf( pFile, "# L language formula of degree %d for the automaton \"%s\"\n", Degree, pAut->pName );
    fprintf( pFile, ".model %s\n", pAut->pName );
    fprintf( pFile, ".inputs" );
    // write the names
    for ( d = Degree; d >= 0; d-- )
    {
        for ( v = 0; v < pAut->nInputs; v++ )
            if ( d )
                fprintf( pFile, " %s-%d", pAut->ppNamesInput[v], d );
            else
                fprintf( pFile, " %s", pAut->ppNamesInput[v] );
    }
    fprintf( pFile, "\n" );
    fprintf( pFile, ".outputs Init Out\n" );

    
    // write the names line for the initial node
    fprintf( pFile, ".names" );
    // write the names
    for ( d = Degree; d >= 0; d-- )
    {
        for ( v = 0; v < pAut->nInputs; v++ )
            if ( d )
                fprintf( pFile, " %s-%d", pAut->ppNamesInput[v], d );
            else
                fprintf( pFile, " %s", pAut->ppNamesInput[v] );
    }
    fprintf( pFile, " Init\n" );

    // write the cubes for the initial states
    if ( pAut->nInitial == 0 )
        pAut->nInitial = 1;

    for ( s = 0; s < pAut->nInitial; s++ )
    {
        pState  = pAut->pStates[s];
        pCover1 = (Mvc_Cover_t *)pState->State1;

        // for each product of the first set, write all products from the second
        Mvc_CoverForEachCube( pCover1, pCube1 )
        {
            // add dashes
            for ( v = 0; v < pAut->nInputs; v++ )
                fputc( '-', pFile );

            // write the product
            for ( d = Degree - 1; d >= 0; d-- )
            {
                for ( v = 0; v < pAut->nInputs; v++ )
                {
                    Value = Mvc_CubeVarValue( pCube1, d * pAut->nInputs + v );
                    if ( Value == 1 )
                        fputc( '0', pFile );
                    else if ( Value == 2 )
                        fputc( '1', pFile );
                    else if ( Value == 3 )
                        fputc( '-', pFile );
                    else
                    {
                        assert( 0 );
                    }
                }
            }
            fprintf( pFile, " 1\n" );
        }
    }


    // write the names line for the first node
    fprintf( pFile, ".names" );
    // write the names
    for ( d = Degree; d >= 0; d-- )
    {
        for ( v = 0; v < pAut->nInputs; v++ )
            if ( d )
                fprintf( pFile, " %s-%d", pAut->ppNamesInput[v], d );
            else
                fprintf( pFile, " %s", pAut->ppNamesInput[v] );
    }
    fprintf( pFile, " Out\n" );


    // write the products
    for ( s = 0; s < pAut->nStates; s++ )
    {
        pState  = pAut->pStates[s];
        pCover1 = (Mvc_Cover_t *)pState->State1;
        pCover2 = (Mvc_Cover_t *)pState->State2;
        pState->State1 = 0;
        pState->State2 = 0;

        // for each product of the first set, write all products from the second
        Mvc_CoverForEachCube( pCover1, pCube1 )
        {
            Mvc_CoverForEachCube( pCover2, pCube2 )
            {
                // write the first product
                for ( d = Degree - 1; d >= 0; d-- )
                {
                    for ( v = 0; v < pAut->nInputs; v++ )
                    {
                        Value = Mvc_CubeVarValue( pCube1, d * pAut->nInputs + v );
                        if ( Value == 1 )
                            fputc( '0', pFile );
                        else if ( Value == 2 )
                            fputc( '1', pFile );
                        else if ( Value == 3 )
                            fputc( '-', pFile );
                        else
                        {
                            assert( 0 );
                        }
                    }
                }

                // write the second product
                for ( v = 0; v < pAut->nInputs; v++ )
                {
                    Value = Mvc_CubeVarValue( pCube2, v );
                    if ( Value == 1 )
                        fputc( '0', pFile );
                    else if ( Value == 2 )
                        fputc( '1', pFile );
                    else if ( Value == 3 )
                        fputc( '-', pFile );
                    else
                    {
                        assert( 0 );
                    }
                }
                fprintf( pFile, " 1\n" );
            }
        }
        Mvc_CoverFree( pCover1 );
        Mvc_CoverFree( pCover2 );
    }

    fprintf( pFile, ".end\n" );
    fclose( pFile );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



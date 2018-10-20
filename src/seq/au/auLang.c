/**CFile****************************************************************

  FileName    [auLang.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generates the language of the given automaton.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: auLang.c,v 1.1 2004/02/19 03:06:46 alanmi Exp $]

***********************************************************************/

#include "auInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Prints out the language of the given automaton.]

  Description [Prints all the strings of the given length that are accepted 
  by the given automaton.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Au_AutoLanguage( FILE * pOut, Au_Auto_t * pAut, int Length )
{
    Au_Rel_t * pTR;
    DdManager * dd;
    int nVars, nVarsBin, nVarsIn, nVarsSt;
    DdNode ** pbRels, ** pbAccs, ** pbLangs, ** pbCodes;
    DdNode * bLang, * bTemp, * bCube, * bAccept;
    int * pPermute;
    Vm_VarMap_t * pVm;
    Vmx_VarMap_t * pVmx;
    int * pValues;
    char * pStr;
    int i, k, v;

    if ( pAut->nStates == 0 )
    {
        fprintf( stdout, "Automaton has no states.\n" );
        return;
    }
    if ( pAut->nStates == 1 )
    {
        fprintf( stdout, "Automaton has only one state.\n" );
        return;
    }

    // get the transition relation
    dd = Mvr_ManagerReadDdLoc( Fnc_ManagerReadManMvr(pAut->pMan) );
    pTR = Au_AutoRelCreate(dd, pAut, 0);
//printf( "Automaton to be determinized has relation with %d BDD nodes.\n", Cudd_DagSize(pTR->bRel) );

    // extend the manager
    assert( pAut->nOutputs == 0 );
    nVarsIn  = pAut->nInputs;
    nVarsSt  = Extra_Base2Log(pAut->nStates);
    nVarsBin = Length * (nVarsIn + nVarsSt) + nVarsSt;
    nVars    = Length * (nVarsIn + 1) + 1;
    if ( dd->size < nVarsBin )
    {
        for ( i = dd->size; i < nVarsBin; i++ )
            Cudd_bddNewVar( dd );
    }

    // create the variable map
    pValues = ALLOC( int, nVars );
    for ( i = 0; i < Length * nVarsIn; i++ )
        pValues[i] = 2;
    for ( ; i < nVars; i++ )
        pValues[i] = pAut->nStates;
    pVm  = Vm_VarMapLookup ( Fnc_ManagerReadManVm(pAut->pMan),  nVars,  0, pValues );
    pVmx = Vmx_VarMapLookup( Fnc_ManagerReadManVmx(pAut->pMan), pVm,   -1, NULL );
    FREE( pValues );

    // get the sum of accepting states
    pbCodes = Vmx_VarMapEncodeVar( dd, pTR->pVmx, nVarsIn + 1 );
    bAccept = Au_AutoStateSumAccepting( pAut, dd, pbCodes );   Cudd_Ref( bAccept );
    Vmx_VarMapEncodeDeref( dd, pTR->pVmx, pbCodes );


    // create the remapped relations and sets of accepting states
    pbRels = ALLOC( DdNode *, Length );
    pbAccs = ALLOC( DdNode *, Length );
    pPermute = ALLOC( int, dd->size );
    for ( i = 0; i < Length; i++ )
    {
        // get the permutation array
        for ( v = 0; v < dd->size; v++ )
            pPermute[v] = v;
        // inputs
        for ( v = 0; v < nVarsIn; v++ )
            pPermute[v] = i * nVarsIn + v;
        // outputs
        for ( v = 0; v < 2 * nVarsSt; v++ )
            pPermute[nVarsIn + v] = Length * nVarsIn + i * nVarsSt + v;
        // permute
        pbRels[i] = Cudd_bddPermute( dd, pTR->bRel, pPermute ); Cudd_Ref( pbRels[i] );
        pbAccs[i] = Cudd_bddPermute( dd, bAccept,   pPermute ); Cudd_Ref( pbAccs[i] );
//PRB( dd, pbRels[i] );
//PRB( dd, pbAccs[i] );
    }
    Cudd_RecursiveDeref( dd, bAccept );
    FREE( pPermute );


    // start with the initial state
    // get the codes of the first state variable
    pbCodes = Vmx_VarMapEncodeVar( dd, pVmx, Length * nVarsIn );
    bLang = pbCodes[0];    Cudd_Ref( bLang );
    Vmx_VarMapEncodeDeref( dd, pVmx, pbCodes );
//PRB( dd, bLang );

    // compute the language
    pbLangs = ALLOC( DdNode *, Length );
    for ( i = 0; i < Length; i++ )
    {
        bCube = Vmx_VarMapCharCube( dd, pVmx, Length * nVarsIn + i );       Cudd_Ref( bCube );
        bLang = Cudd_bddAndAbstract( dd, bTemp = bLang, pbRels[i], bCube ); Cudd_Ref( bLang );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bCube );
        pbLangs[i] = bLang;   Cudd_Ref( bLang );
//PRB( dd, pbRels[i] );
//PRB( dd, pbAccs[i] );
//PRB( dd, pbLangs[i] );
    }
    Cudd_RecursiveDeref( dd, bLang );


    // quantify the last set of vars
    for ( i = 0; i < Length; i++ )
    {
        bCube = Vmx_VarMapCharCube( dd, pVmx, Length * nVarsIn + i + 1 );    Cudd_Ref( bCube );
        pbLangs[i] = Cudd_bddAndAbstract( dd, bTemp = pbLangs[i], pbAccs[i], bCube ); Cudd_Ref( pbLangs[i] );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bCube );
//PRB( dd, pbLangs[i] );
    }

    // print the language
    pStr = ALLOC( char, dd->size );
    for ( i = 0; i < Length; i++ )
    {
        bLang = pbLangs[i];  Cudd_Ref( bLang );
        fprintf( pOut, "The language contains %f strings of length %d.\n", 
            Cudd_CountMinterm(dd, bLang, (i + 1) * nVarsIn), i + 1 );
        while ( bLang != b0 )
        {
            // get one cube
            Extra_bddPickOneCube( dd, bLang, pStr );
            // derive the cube
            bCube = b1; Cudd_Ref( bCube );
            for ( k = 0; k < Length * nVarsIn; k++ )
                if ( pStr[k] == 0 )
                {
                    bCube = Cudd_bddAnd( dd, bTemp = bCube, Cudd_Not(dd->vars[k]) ); Cudd_Ref( bCube );
                    Cudd_RecursiveDeref( dd, bTemp );
                }
                else if ( pStr[k] == 1 )
                {
                    bCube = Cudd_bddAnd( dd, bTemp = bCube, dd->vars[k] ); Cudd_Ref( bCube );
                    Cudd_RecursiveDeref( dd, bTemp );
                }
            // subtract this cube from the language
            bLang = Cudd_bddAnd( dd, bTemp = bLang, Cudd_Not(bCube) ); Cudd_Ref( bLang );
            Cudd_RecursiveDeref( dd, bTemp );
            Cudd_RecursiveDeref( dd, bCube );

            // correct the cube for printing
            for ( k = 0; k < (i + 1) * nVarsIn; k++ )
                if ( pStr[k] == 2 )
                    pStr[k] += '-' - 2;
                else
                    pStr[k] += '0';
            // create the cube from the string
            for ( k = 0; k <= i; k++ )
            {
                if ( nVarsIn > 1 )
                    fprintf( pOut, "(" );
                for ( v = 0; v < nVarsIn; v++ )
                    fprintf( pOut, "%c", pStr[k * nVarsIn + v] );
                if ( nVarsIn > 1 )
                    fprintf( pOut, ")" );
            }
            fprintf( pOut, "\n" );
        }
        Cudd_RecursiveDeref( dd, bLang );
    }
    FREE( pStr );
//    fprintf( pOut, "(The numbers are approximate for automata with encoded inputs/outputs.)\n" );

    for ( i = 0; i < Length; i++ )
    {
        Cudd_RecursiveDeref( dd, pbRels[i] );
        Cudd_RecursiveDeref( dd, pbAccs[i] );
        Cudd_RecursiveDeref( dd, pbLangs[i] );
    }

    FREE( pbRels );
    FREE( pbAccs );
    FREE( pbLangs );
    return;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



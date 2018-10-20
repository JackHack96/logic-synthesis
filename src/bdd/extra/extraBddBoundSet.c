/**CFile****************************************************************

  FileName    [extraBddBoundSet.c]

  PackageName [extra]

  Synopsis    [Characterization of bounds sets in the BDD. This method
  for the implicit computation of all bound sets is described in the paper:
  A. Mishchenko, X. Wang, T. Kam. A New Enhanced Constructive Decomposition 
  and Mapping Algorithm. Proc. Design Automation Conference 2003.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - September 1, 2003.]

  Revision    [$Id: extraBddBoundSet.c,v 1.2 2004/05/12 04:30:10 alanmi Exp $]

***********************************************************************/

#include "extra.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

#define MAX_SUPP_SIZE     32

/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

// this manager is used for the computation of bound sets
static DdManager * s_dd = NULL;

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/


/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

/**AutomaticEnd***************************************************************/

static void extraBddEnumerateBoundSets( DdManager * ddInit, DdManager * dd, DdNode * bFunc, int nRange, 
    Extra_VarSets_t * pVarSets, unsigned uVarSet );
static int extraBoundSetsSortCompare( unsigned ** ppU1, unsigned ** ppU2 );

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function*************************************************************

  Synopsis    [Starts the BDD manager for bound set computation.]

  Description [Currently, these procedures can be applied to the 
  functions whose support does not exceed MAX_SUPP_SIZE = 32 
  variables.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_BoundSetsStart( int nSuppMax )
{
    assert( s_dd == NULL );
    assert( nSuppMax <= MAX_SUPP_SIZE );
    s_dd = Cudd_Init( 3 * nSuppMax, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
}

/**Function*************************************************************

  Synopsis    [Stops the BDD manager for bound set computation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_BoundSetsStop()
{
    assert( s_dd );
    Extra_StopManager( s_dd );
    s_dd = NULL;
}

/**Function*************************************************************

  Synopsis    [Computes *all* support-reducing bound sets of the given size.]

  Description [The function (bFuncInit) should depend on all or some 
  variable among the topmost 32 variables in the BDD manager (ddInit).
  The set of variables (bVarsToUse) specifies the variables to be 
  used in the bound sets. If this variable is set to NULL, computes the
  bound sets for all support variables. Only the bound sets of the given size 
  (nVarsBSet) are considered. Returns the data structure (Extra_VarSets_t) 
  containing information about the bound sets. This data structure contains 
  the number of all bound sets computed (pVarSets->nSets). The i-th bound set 
  is represented by the unsigned integer (pVarSets->pSets[i]), with those bits 
  set to one that correspond to the variables present in the given bound set 
  (the set of variables that can appear in the bound sets is limited by 32). 
  The number of different cofactors in a bound set is pVarSets->pSizes[i].]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Extra_VarSets_t * Extra_BoundSetsCompute( 
  DdManager * ddInit, // the manager containing the original function 
  DdNode * bFuncInit, // the function whose bound sets are computed
  DdNode * bVarsToUse,// the variables to be included into the bound sets
  int nVarsBSet )     // the number of variables in the bound sets
{
    Extra_VarSets_t * pVarSets;
    DdNode ** pbVarsA, ** pbVarsB, ** pbVarsC, ** pbComps;
    DdNode * bFunc, * bTemp, * bCubeAll, * bTuples, * bVars;
    int nSupp, nRange, i, iLevelVar, clk, clkTot;
    int * pPermute;
    unsigned uVarSet;
clkTot = clock();

    // create the set of variables to use
    if ( bVarsToUse == NULL )
        bVars = Cudd_Support( ddInit, bFuncInit );
    else
        bVars = bVarsToUse;
    Cudd_Ref( bVars );

    // set the size of the variable range in the computation manager
    nRange = s_dd->size/3;
    // get the number of support variables
    nSupp = Cudd_SupportSize( ddInit, bFuncInit );

    // make sure the support is not too large 
    assert( nSupp <= nRange );
    assert( nSupp < MAX_SUPP_SIZE );   // MAX_SUPP_SIZE = 32
    // here we should check that the support variables of bFuncInit
    // are concentrated among the topmost 32 variables of ddInit

    // make sure the bound set size is okay
    assert( nVarsBSet < nSupp );

    // make sure that enough vars are given
    assert( Cudd_SupportSize(ddInit,bVars) > nVarsBSet );

    // allocate room for variable sets
    pbVarsA = ALLOC( DdNode *, 2 * s_dd->size );
    pbVarsB = pbVarsA + nRange;
    pbVarsC = pbVarsB + nRange;
    pbComps = pbVarsC + nRange;

    // set up variables for bound set selection
    for ( i = 0; i < nRange; i++ )
    {
        pbVarsA[i] = s_dd->vars[ s_dd->invperm[3*i + 0] ];
        pbVarsB[i] = s_dd->vars[ s_dd->invperm[3*i + 1] ];
        pbVarsC[i] = s_dd->vars[ s_dd->invperm[3*i + 2] ];
    }
    // set the identity composition functions
    for ( i = 0; i < s_dd->size; i++ )
    {
        pbComps[i] = s_dd->vars[i];  Cudd_Ref( pbComps[i] );
    }
    // set up composition only for the selected variables
    // and create the cube of all variables pbVarsA[] that are allowed
    bCubeAll = s_dd->one;  Cudd_Ref( bCubeAll );
    for ( i = 0; i < ddInit->size; i++ )
    {
        // if the variable on the i-th level in ddInit should be used,
        // prepare the variable on the i-th level of the computation manager
        if ( Cudd_bddLeq( ddInit, bVars, ddInit->vars[ ddInit->invperm[i] ] ) )
        { // the var on i-th level is included in bVars
            // set the function for composition
            iLevelVar = s_dd->invperm[i];
            Cudd_RecursiveDeref( s_dd, pbComps[iLevelVar] );
            pbComps[iLevelVar] = Cudd_bddIte( s_dd, pbVarsA[i], pbVarsB[i], pbVarsC[i] ); 
            Cudd_Ref( pbComps[iLevelVar] );
            // add this variable to the cube
            bCubeAll = Cudd_bddAnd( s_dd, bTemp = bCubeAll, pbVarsA[iLevelVar] ); Cudd_Ref( bCubeAll );
            Cudd_RecursiveDeref( s_dd, bTemp );     
        }
    }
    Cudd_RecursiveDeref( ddInit, bVars );


    // transfer the BDD into the computation manager
    bFunc = Extra_TransferLevelByLevel( ddInit, s_dd, bFuncInit );  Cudd_Ref( bFunc );
    // vector-compose the BDD
    bFunc = Cudd_bddVectorCompose( s_dd, bTemp = bFunc, pbComps );  Cudd_Ref( bFunc );
    Cudd_RecursiveDeref( s_dd, bTemp );     
    for ( i = 0; i < s_dd->size; i++ )
        Cudd_RecursiveDeref( s_dd, pbComps[i] );
    FREE( pbVarsA );

    // get the tuples
    bTuples = Extra_bddTuples( s_dd, nVarsBSet, bCubeAll );   Cudd_Ref( bTuples );
    Cudd_RecursiveDeref( s_dd, bCubeAll );
    // restrict to the tuples
    bFunc = Cudd_bddAnd( s_dd, bTemp = bFunc, bTuples );      Cudd_Ref( bFunc );
    Cudd_RecursiveDeref( s_dd, bTemp );
    Cudd_RecursiveDeref( s_dd, bTuples );

    // permute
    pPermute = ALLOC( int, s_dd->size );
    for ( i = 0; i < nRange; i++ )
    {
        pPermute[ s_dd->invperm[3*i + 0] ] = s_dd->invperm[0 * nRange + i];
        pPermute[ s_dd->invperm[3*i + 1] ] = s_dd->invperm[1 * nRange + i]; 
        pPermute[ s_dd->invperm[3*i + 2] ] = s_dd->invperm[2 * nRange + i]; 
    } 

clk = clock();
PRN( bFunc );
    // this is the most time-consuming step
    bFunc = Cudd_bddPermute( s_dd, bTemp = bFunc, pPermute );      Cudd_Ref( bFunc );
PRN( bFunc );
    Cudd_RecursiveDeref( s_dd, bTemp );
    FREE( pPermute );
PRT( "Permute   ", clock() - clk );

    // find the number of combinations of nVarsBSet out of nSupp vars
    pVarSets = ALLOC( Extra_VarSets_t, 1 );
    memset( pVarSets, 0, sizeof(Extra_VarSets_t) );
    pVarSets->nSets = Extra_NumCombinations( nVarsBSet, nSupp );
    pVarSets->pSets = ALLOC( unsigned, pVarSets->nSets );
    pVarSets->pSizes = ALLOC( char, pVarSets->nSets );

    // enumerate the combinations
    uVarSet = 0;
    pVarSets->iSet = 0;
clk = clock();
    extraBddEnumerateBoundSets( ddInit, s_dd, bFunc, nRange, pVarSets, uVarSet );
PRT( "Enumerate ", clock() - clk );
//    assert( pVarSets->iSet == pVarSets->nSets );
    Cudd_RecursiveDeref( s_dd, bFunc );
PRT( "Total     ", clock() - clkTot );
    return pVarSets;
}

/**Function*************************************************************

  Synopsis    [Frees the bound set structure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_BoundSetsFree( Extra_VarSets_t * p )
{
    FREE( p->pSets );
    FREE( p->pSizes );
    FREE( p );
}

/**Function*************************************************************

  Synopsis    [Sort the bound sets by the number of different cofactors.]

  Description [Only works for bound sets of size less or equal 27.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_BoundSetsSort( Extra_VarSets_t * p )
{
    unsigned * pArray;
    unsigned ** pPointers;
    int i, v, iNew;
    pArray     = ALLOC( unsigned, p->nSets );
    pPointers  = ALLOC( unsigned *, p->nSets );
    for ( i = 0; i < p->nSets; i++ )
    {
        pArray[i] = (p->pSizes[i] << 27);
        for ( v = 0; v < 27; v++ )
            if ( (p->pSets[i] & (1 << (26-v))) == 0 )
                pArray[i] |= (1 << v);
        pPointers[i] = pArray + i;
    }
    qsort( (void *)pPointers, p->nSets, sizeof(unsigned *), 
        (int (*)(const void *, const void *)) extraBoundSetsSortCompare );
    assert( extraBoundSetsSortCompare( pPointers, pPointers + p->nSets - 1 ) < 0 );
    for ( i = 0; i < p->nSets; i++ )
        pArray[i] = (((unsigned)p->pSizes[i]) << 27) | p->pSets[i];
    for ( i = 0; i < p->nSets; i++ )
    {
        iNew = pPointers[i] - pArray;
        p->pSizes[i] = (pArray[iNew] >> 27);
        p->pSets[i]  = (pArray[iNew] & 0x7FFFFFF);
    }
    free( pArray );
    free( pPointers );
}

/**Function*************************************************************

  Synopsis    [Prints the set of bound sets of the given function.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_BoundSetsPrint( FILE * pFile, Extra_VarSets_t * p )
{
    int i, v;
    fprintf( pFile, "There are %d bound sets:\n", p->nSets );
    for ( i = 0; i < p->nSets; i++ )
    {
        fprintf( pFile, "%3d : size = %2d  {", i+1, p->pSizes[i] );
        for ( v = 0; v < 32; v++ )
            if ( p->pSets[i] & (1 << v) )
                fprintf( pFile, " %d", v );
        fprintf( pFile, "}\n" );
    }
}

/**Function*************************************************************

  Synopsis    [Tests the procedures by calling them two times for the same function.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_BoundSetsTest( DdManager * dd, DdNode * bFunc, int nVarsBSet )
{
    Extra_VarSets_t * p;
    // prepare the bound set computation
    Extra_BoundSetsStart( dd->size );
    // compute bound sets for the function as it is
    p = Extra_BoundSetsCompute( dd, bFunc, NULL, nVarsBSet );
    Extra_BoundSetsSort( p );
    Extra_BoundSetsPrint( stdout, p );
    Extra_BoundSetsFree( p );
    // reorder the variables in the user's BDD manager
    Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 1 );
    // call the bound set computation again (should give the same result)
    p = Extra_BoundSetsCompute( dd, bFunc, NULL, nVarsBSet );
    Extra_BoundSetsSort( p );
    Extra_BoundSetsPrint( stdout, p );
    Extra_BoundSetsFree( p );
    // quit the computation manager
    Extra_BoundSetsStop();
}

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of static Functions                                            */
/*---------------------------------------------------------------------------*/

/**Function*************************************************************

  Synopsis    [Enumerates the bound sets and counts the number of cofactors.]

  Description [Relies on the fact that all bound sets are of the same size.
  In this case, the paths written into uVarSet cannot have don't-care 
  variables.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void extraBddEnumerateBoundSets( DdManager * ddInit, DdManager * dd, DdNode * bFunc, int nRange, 
    Extra_VarSets_t * pVarSets, unsigned uVarSet )
{
    DdNode * bFR, * bF0, * bF1;  
    int VarInit;

    if ( bFunc == b0 )
        return;

    bFR = Cudd_Regular(bFunc); 
    if ( bFR->index >= nRange ) // uses the fact that dd is never reordered
    {
        // add this bound set
        pVarSets->pSets[ pVarSets->iSet ]  = uVarSet;
        pVarSets->pSizes[ pVarSets->iSet ] = Extra_WidthAtLevel(dd,bFunc,2*nRange);
        pVarSets->iSet++;
        return;
    }

    // cofactor the functions
    if ( bFR != bFunc ) // bFunc is complemented 
    {
        bF0 = Cudd_Not( cuddE(bFR) );
        bF1 = Cudd_Not( cuddT(bFR) );
    }
    else
    {
        bF0 = cuddE(bFR);
        bF1 = cuddT(bFR);
    }

    // traverse the branches
    extraBddEnumerateBoundSets( ddInit, dd, bF0, nRange, pVarSets, uVarSet );

    // find the index of the variable in the original manager (ddInit)
    // which appears on the same level as the given variable in the computation manager (dd)
    VarInit = ddInit->invperm[ bFR->index ];
    uVarSet |=  (1 << VarInit);
    extraBddEnumerateBoundSets( ddInit, dd, bF1, nRange, pVarSets, uVarSet );
    uVarSet &= ~(1 << VarInit);
}


/**Function*************************************************************

  Synopsis    [Compares the vars by their number.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int extraBoundSetsSortCompare( unsigned ** ppU1, unsigned ** ppU2 )
{
    if ( **ppU1 < **ppU2 )
        return -1;
    if ( **ppU1 > **ppU2 )
        return 1;
    assert( 0 );
    return 0;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



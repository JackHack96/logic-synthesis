/**CFile****************************************************************

  FileName    [mvrUcp.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Solves unate covering problem to determine the minimum 
  set of values needed to represent the given MV relation.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvrUcp.c,v 1.1 2003/07/17 01:12:37 alanmi Exp $]

***********************************************************************/

#include "mvrInt.h"
#include "sparse.h"
#include "mincov.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Determines the minimum set of values to cover the relation.]

  Description [This function takes the relation and the BDD representing
  a cofactor of this relation. The BDD should be expressed in the same
  BDD manager as the relation. It returns the minimum number of values 
  needed to cover the complete boolean space of the relation. If pValues 
  is not NULL, this function attaches to this pointer the array containing
  the smallest set of values needed to cover the complete boolean space of 
  the relation. It is the user's responsibility to free this array when
  it is no longer needed.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvr_RelationMinimumValueSet( Mvr_Relation_t * pMvr, DdNode * bRel, int ** pValues )
{
	DdNode * bDomain, * bSplitDomain, * bTemp;
    DdNode * pbCofs[32];
    DdManager * dd;
	sm_matrix * A;
	sm_row * Solution;
	unsigned ValueSet;
	array_t * aValueDomains, * aValueSets;
	int nValueSetCounter, nValueSetCounterStart;
	int nSolution, i, v;
    int iOutVar, nOutValues;
//	static Counter = 0;

    dd = Mvr_RelationReadDd( pMvr );

    // get the number of input variables and the number of output values
    iOutVar    = Vm_VarMapReadVarsInNum( pMvr->pVmx->pVm );
    nOutValues = Vm_VarMapReadValuesOutNum( pMvr->pVmx->pVm );

    // the number of values should satisfy the range
	assert( nOutValues >= 2 );
	assert( nOutValues <= 32 );

    // the relation should be well-defined
	assert( bRel != b0 );

	if ( bRel == b1 )
	{
		*pValues = NULL;
		return 0;
	}
 
	// allocate room for domains and value sets
	aValueDomains = array_alloc( DdNode *, 100 );
	aValueSets    = array_alloc( unsigned, 100 );

	// get started
	array_insert_last( DdNode *, aValueDomains, b1 );  Cudd_Ref( b1 );
	array_insert_last( unsigned, aValueSets,    0  );
	nValueSetCounter = 1;

    // set the cofactor instead of the relation
    // and cofactor it w.r.t. the output variable
    // the result goes into pbCofs[32]
    bTemp = pMvr->bRel;
    pMvr->bRel = bRel;
    Mvr_RelationCofactorsDerive( pMvr, pbCofs, iOutVar, nOutValues );
    pMvr->bRel = bTemp;

	// iteratively refine domains for each value and create value sets
	for ( v = 0; v < nOutValues; v++ )
	{
		// intersect this domain with the already available domains
		nValueSetCounterStart = nValueSetCounter;
		for ( i = 0; i < nValueSetCounterStart; i++ )
		{
			// get the domain and the value set of this cell
			bDomain  = array_fetch( DdNode *, aValueDomains, i );
			ValueSet = array_fetch( unsigned, aValueSets, i );

			// check intersection
//			if ( bdd_leq( bDomain, pbCofs[v], 1, 0 ) == 0 ) // they intersect
			if ( !Cudd_bddLeq( dd, bDomain, Cudd_Not(pbCofs[v]) ) ) // they intersect
			{
				// refine
                bSplitDomain = Cudd_bddAnd( dd, bDomain, pbCofs[v] );  Cudd_Ref( bSplitDomain );
				array_insert( DdNode *, aValueDomains, i, bSplitDomain );
				array_insert( unsigned, aValueSets,    i, (ValueSet | (1<<v)) );

				// add the complement
//				if ( bdd_leq( bDomain, bTemp, 1, 0 ) == 0 ) // they intersect
				if ( !Cudd_bddLeq( dd, bDomain, pbCofs[v] ) ) // they intersect
				{
                    bSplitDomain = Cudd_bddAnd( dd, bDomain, Cudd_Not(pbCofs[v]) );  Cudd_Ref( bSplitDomain );
					array_insert_last( DdNode *, aValueDomains, bSplitDomain );
					array_insert_last( unsigned, aValueSets,  ValueSet );
					nValueSetCounter++;
				}
				Cudd_RecursiveDeref( dd, bDomain ); // bDomain is replaced during refinement
			}
		}
	}
    Mvr_RelationCofactorsDeref( pMvr, pbCofs, iOutVar, nOutValues );

	// this point we can print all domains and verify them

	// free the domains
	for ( i = 0; i < nValueSetCounter; i++ )
	{
		bDomain = array_fetch( DdNode *, aValueDomains, i );
		Cudd_RecursiveDeref( dd, bDomain );
	}
	array_free( aValueDomains );

	// create the sparse matrix
	// this matrix has as many rows as there are different value subsets
	// and as many columns as there are values
	A = sm_alloc();
    sm_resize( A, nValueSetCounter, nOutValues );
	// go through the value subsets and add their values to the matrix
	for ( i = 0; i < nValueSetCounter; i++ )
	{
		ValueSet = array_fetch( unsigned, aValueSets, i );
		for ( v = 0; v < nOutValues; v++ )
			if ( ValueSet & (1<<v) )
				sm_insert(A, i, v);
	}
	array_free( aValueSets );

/*
//	if ( A->nrows > 3 ) 
	if ( Counter++ == 0 )
	{
		FILE* fpo = fopen( "cc.mtr", "w" );
		sm_print( fpo, A );
		fclose( fpo );
	}
*/

	// solve the problem
	Solution  = sm_minimum_cover( A, NULL, 0 /*heuristic*/, 0 /*verbose*/ );
	nSolution = Solution->length;


    // return the array of values
	if ( pValues != NULL )
	{
		// save the solution
		int * pColArray;
		sm_element *p;
		int nSol = 0;

		pColArray = (int*) malloc( Solution->length * sizeof(int) );
		for( p = Solution->first_col; p != 0; p = p->next_col ) 
			pColArray[nSol++] = p->col_num;
		assert( nSol == nSolution );

		// return the array of values
		*pValues = pColArray;
	}

	// free the matrix and the solution
	sm_free(A);
	sm_row_free(Solution);
	return nSolution;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



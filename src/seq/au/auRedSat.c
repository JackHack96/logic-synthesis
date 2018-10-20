/**CFile****************************************************************

  FileName    [auReduce.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Various procedures working with automata and FSMs.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: auRedSat.c,v 1.1 2004/02/19 03:06:48 alanmi Exp $]

***********************************************************************/

#include "auInt.h"
#include "sparse.h"
#include "mincov.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static sm_matrix * Au_AutoSetupMatrix( Au_Auto_t * pAut );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Finds one deterministic reduction of the FSM-ND automaton.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Au_Auto_t * Au_AutoReductionSat( Au_Auto_t * pAut, int nInputs, bool fVerbose )
{
    Au_Auto_t * pAutR;
    DdManager * dd;
    Vm_VarMap_t * pVm;
    Vmx_VarMap_t * pVmx;

    sm_matrix * pMatrix;
    sm_row * cover;
    int * weights;
	int heuristic = 0;
	int verbose = 0;
    sm_element * pElem;
	int nSolution = 0;
    int i;


    if ( nInputs > pAut->nInputs )
    {
        printf( "The number of FSM inputs is larger than the number of all inputs.\n" );
        return NULL;
    }

    // get the manager and expand it if necessary
    dd = Cudd_Init( pAut->nInputs, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    // get the var maps
    pVm = Vm_VarMapCreateBinary( Fnc_ManagerReadManVm(pAut->pMan), nInputs, 0 );
    pVmx = Vmx_VarMapLookup( Fnc_ManagerReadManVmx(pAut->pMan), pVm, -1, NULL );

    // get the conditions for each arch
    Au_ComputeConditions( dd, pAut, pVmx );
    // compute partitions
    Au_AutoStatePartitions( dd, pAut );
    if ( fVerbose )
        Au_AutoStatePartitionsPrint( pAut );
    // clean the current automaton
    Au_DeleteConditions( dd, pAut );
    Extra_StopManager( dd );

    // set up the BCP problem
	pMatrix = Au_AutoSetupMatrix( pAut );

// test the pMatrix
//{
//FILE* fpo = fopen("pMatrix.out", "w");
//sm_print(fpo, pMatrix);
//fclose( fpo );
//}

    weights = ALLOC( int, 2 * pAut->nStates );
    for ( i = 0; i < pAut->nStates; i++ )
    {
        weights[2*i+0] = 1;
        weights[2*i+1] = 0;
    }
	cover = sm_mat_bin_minimum_cover( pMatrix, weights, heuristic, verbose, 0, 6176, NULL);
    FREE( weights );

// for debugging, print the solution
//printf("Solution is ");
//sm_row_print(stdout, cover);
//printf("\n");
if ( fVerbose )
printf("Solution is ");
    // select only those state that correspond to positive polarity columns
    for ( i = 0; i < pAut->nStates; i++ )
        pAut->pStates[i]->fMark = 0;
    for ( pElem = cover->first_col; pElem; pElem = pElem->next_col ) 
		if ( pElem->col_num % 2 == 0 )
        {
            pAut->pStates[pElem->col_num/2]->fMark = 1;
            if ( fVerbose )
                printf( " %d", pElem->col_num/2 );
        }
if ( fVerbose )
printf("\n");

	// free the pMatrix and the solution
	sm_free(pMatrix);
	sm_row_free(cover);

    // extract the automaton with marked states and transitions
    pAutR = Au_AutoExtract( pAut, 0 );
    Au_AutoStatePartitionsFree( pAut );
    return pAutR;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
sm_matrix * Au_AutoSetupMatrix( Au_Auto_t * pAut )
{
    Au_State_t * pState;
    sm_matrix * pMatrix;
    Au_SPInfo_t * p;
    int nClauses, iClause;
    int s, i, k;

    // count the number of non-trivial clauses in the problem
    nClauses = 1;
    for ( s = 0; s < pAut->nStates; s++ )
    {
        pState = pAut->pStates[s];
        p = Au_AutoInfoGet( pState );
        for ( i = 0; i < p->nParts; i++ )
        {
            // skip self referencing clause
            for ( k = 0; k < p->pSizes[i]; k++ )
                if ( p->ppParts[i][k] == s )
                    break;
            if ( k < p->pSizes[i] )
                continue;
            nClauses++;
        }
    }

    // add the first clause for the initial state
	pMatrix = sm_alloc();
    sm_resize( pMatrix, nClauses, 2 * pAut->nStates );
    sm_insert( pMatrix, 0, 0 );
    // go through the states and add clauses
    iClause = 1;
    for ( s = 0; s < pAut->nStates; s++ )
    {
        pState = pAut->pStates[s];
        p = Au_AutoInfoGet( pState );
        for ( i = 0; i < p->nParts; i++ )
        {
            // skip self referencing clause
            for ( k = 0; k < p->pSizes[i]; k++ )
                if ( p->ppParts[i][k] == s )
                    break;
            if ( k < p->pSizes[i] )
                continue;
            // add this clause
            sm_insert( pMatrix, iClause, 2*s + 1 );
            for ( k = 0; k < p->pSizes[i]; k++ )
                sm_insert( pMatrix, iClause, 2*p->ppParts[i][k] );
            iClause++;
        }
    }
    return pMatrix;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



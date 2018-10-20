/**CFile****************************************************************

  FileName    [dualNormal.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Computation of the normal form of a formula.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 18, 2003.]

  Revision    [$Id: dualNormal.c,v 1.3 2004/02/19 03:06:53 alanmi Exp $]

***********************************************************************/

#include "dualInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dual_SpecNormal( Dual_Spec_t * p )
{
    Extra_NodeSet_t * pSet;
    DdManager * dd = p->dd;
    DdNode * bNode, * bFunc, * bPath;
    DdNode * bCube, * bTemp;
    int nStatesOld;
    int Level, i, k;

    // get the disjoint paths and the correponding cofactors
    Level = p->nRank * p->nVars;
    pSet = Extra_NodePaths( p->dd, p->bSpec, Level );

    // alloc the state info
//    p->nStatesAlloc = 3 * st_count(tNodes);
    p->nStatesAlloc = 3 * pSet->pnPaths[Level];
    p->pbMarksP = ALLOC( DdNode *, p->nStatesAlloc );
    p->pbMarks  = ALLOC( DdNode *, p->nStatesAlloc );
    p->pbTrans  = ALLOC( DdNode *, p->nStatesAlloc );

    p->nStates = 0;
//    st_foreach_item( tNodes, gen, (char **)&bNode, (char **)&bPath )

/*
    // verify the expansion
    {
        DdNode * bSum, * bProd;
        bSum = b0;  Cudd_Ref( b0 );
        for ( i = 0; i < pSet->pnPaths[Level]; i++ )
        {
            bNode = pSet->pbCofs[Level][i];
            bPath = pSet->pbPaths[Level][i];
            bProd = Cudd_bddAnd( dd, bNode, bPath );      Cudd_Ref( bProd );
            bSum = Cudd_bddOr( dd, bTemp = bSum, bProd ); Cudd_Ref( bSum );
            Cudd_RecursiveDeref( dd, bTemp );
            Cudd_RecursiveDeref( dd, bProd );
        }
        assert( bSum == p->bSpec );
        Cudd_RecursiveDeref( dd, bSum );
    }
*/
//PRB( dd, p->bCondInit );
    for ( i = 0; i < pSet->pnPaths[Level]; i++ )
    {
        bNode = pSet->pbCofs[Level][i];
        bPath = pSet->pbPaths[Level][i];
        if ( bNode == b0 )
            continue;
//PRB( dd, bPath );
//PRB( dd, bNode );
        p->pbMarksP[ p->nStates ] = bPath;    Cudd_Ref( bPath );
        p->pbMarks[ p->nStates ]  = Extra_bddMove( dd, bPath, p->nVars );  Cudd_Ref( p->pbMarks[ p->nStates ] );
        p->pbTrans[ p->nStates ]  = bNode;    Cudd_Ref( bNode );
        p->nStates++;
    }
//    st_free_table(tNodes);
    Extra_NodeSetDeref( pSet );


    // perform state splitting
    if ( p->nRank > 1 )
    {
        // try to split each state with each
        bCube = Extra_bddComputeCube( dd, p->pbVars, p->nVars );        Cudd_Ref( bCube );
        for ( i = 0; i < p->nStates; i++ )
        for ( k = 0; k <= i; k++ )
        {
            if ( !p->pbMarksP[i] || !p->pbMarksP[k] )
                continue;

            // try splitting i with k

            bFunc = Cudd_bddAnd( dd, p->pbMarksP[i], p->pbTrans[i] );   Cudd_Ref( bFunc );
            // multiply with the original spec
            bFunc = Cudd_bddAnd( dd, bTemp = bFunc, p->pbMarks[k] );    Cudd_Ref( bFunc );
            Cudd_RecursiveDeref( dd, bTemp );
            // quantify the extra vars
            bFunc = Cudd_bddExistAbstract( dd, bTemp = bFunc, bCube );  Cudd_Ref( bFunc );
            Cudd_RecursiveDeref( dd, bTemp );
            // check if there is state split
            if ( bFunc != b0 && bFunc != p->pbMarksP[i] )
            { // state split
                assert( Cudd_bddLeq( dd, bFunc, p->pbMarksP[i] ) );

                // realloc storage for states if necessary
                if ( p->nStatesAlloc <= p->nStates + 2 )
                {
                    p->pbMarksP  = REALLOC( DdNode *, p->pbMarksP,  2 * p->nStatesAlloc );
                    p->pbMarks   = REALLOC( DdNode *, p->pbMarks,   2 * p->nStatesAlloc );
                    p->pbTrans   = REALLOC( DdNode *, p->pbTrans,   2 * p->nStatesAlloc );
                    p->nStatesAlloc *= 2;
                }

                // create new entries
                p->pbMarksP[p->nStates] = bFunc;   // takes ref
                p->pbMarks[p->nStates]  = Extra_bddMove( dd, p->pbMarksP[p->nStates], p->nVars ); Cudd_Ref( p->pbMarks[p->nStates] );
                p->pbTrans[p->nStates]  = p->pbTrans[i];    Cudd_Ref( p->pbTrans[i] );

                p->pbMarksP[p->nStates+1] = Cudd_bddAnd( dd, p->pbMarksP[i], Cudd_Not(bFunc) );  Cudd_Ref( p->pbMarksP[p->nStates+1] );
                p->pbMarks[p->nStates+1]  = Extra_bddMove( dd, p->pbMarksP[p->nStates+1], p->nVars );  Cudd_Ref( p->pbMarks[p->nStates+1] );
                p->pbTrans[p->nStates+1]  = p->pbTrans[i];  Cudd_Ref( p->pbTrans[i] );

                Cudd_RecursiveDeref( dd, p->pbMarksP[i] );
                Cudd_RecursiveDeref( dd, p->pbMarks[i] );
                Cudd_RecursiveDeref( dd, p->pbTrans[i] );

                p->pbMarksP[i] = NULL;
                p->pbMarks[i]  = NULL;
                p->pbTrans[i]  = NULL;

                p->nStates += 2;
                break;
            }
            else
            {
                Cudd_RecursiveDeref( dd, bFunc );
            }

            // try splitting k with i

            bFunc = Cudd_bddAnd( dd, p->pbMarksP[k], p->pbTrans[k] );   Cudd_Ref( bFunc );
            // multiply with the original spec
            bFunc = Cudd_bddAnd( dd, bTemp = bFunc, p->pbMarks[i] );    Cudd_Ref( bFunc );
            Cudd_RecursiveDeref( dd, bTemp );
            // quantify the extra vars
            bFunc = Cudd_bddExistAbstract( dd, bTemp = bFunc, bCube );  Cudd_Ref( bFunc );
            Cudd_RecursiveDeref( dd, bTemp );
            // check if there is state split
            if ( bFunc != b0 && bFunc != p->pbMarksP[k] )
            { // state split
                assert( Cudd_bddLeq( dd, bFunc, p->pbMarksP[k] ) );

                // realloc storage for states if necessary
                if ( p->nStatesAlloc <= p->nStates + 2 )
                {
                    p->pbMarksP  = REALLOC( DdNode *, p->pbMarksP,  2 * p->nStatesAlloc );
                    p->pbMarks   = REALLOC( DdNode *, p->pbMarks,   2 * p->nStatesAlloc );
                    p->pbTrans   = REALLOC( DdNode *, p->pbTrans,   2 * p->nStatesAlloc );
                    p->nStatesAlloc *= 2;
                }

                // create new entries
                p->pbMarksP[p->nStates] = bFunc;   // takes ref
                p->pbMarks[p->nStates]  = Extra_bddMove( dd, p->pbMarksP[p->nStates], p->nVars ); Cudd_Ref( p->pbMarks[p->nStates] );
                p->pbTrans[p->nStates]  = p->pbTrans[k];    Cudd_Ref( p->pbTrans[k] );

                p->pbMarksP[p->nStates+1] = Cudd_bddAnd( dd, p->pbMarksP[k], Cudd_Not(bFunc) );  Cudd_Ref( p->pbMarksP[p->nStates+1] );
                p->pbMarks[p->nStates+1]  = Extra_bddMove( dd, p->pbMarksP[p->nStates+1], p->nVars );  Cudd_Ref( p->pbMarks[p->nStates+1] );
                p->pbTrans[p->nStates+1]  = p->pbTrans[k];  Cudd_Ref( p->pbTrans[k] );

                Cudd_RecursiveDeref( dd, p->pbMarksP[k] );
                Cudd_RecursiveDeref( dd, p->pbMarks[k] );
                Cudd_RecursiveDeref( dd, p->pbTrans[k] );

                p->pbMarksP[k] = NULL;
                p->pbMarks[k]  = NULL;
                p->pbTrans[k]  = NULL;

                p->nStates += 2;
                break;
            }
            else
            {
                Cudd_RecursiveDeref( dd, bFunc );
            }
        }
        Cudd_RecursiveDeref( dd, bCube );

        // compact the entries
        nStatesOld = p->nStates;
        p->nStates = 0;
        for ( i = 0; i < nStatesOld; i++ )
            if ( p->pbMarksP[i] )
            {
                p->pbMarksP[p->nStates] = p->pbMarksP[i];
                p->pbMarks [p->nStates] = p->pbMarks [i];
                p->pbTrans [p->nStates] = p->pbTrans [i];
                p->nStates++;
            }
    }
//for ( i = 0; i < p->nStates; i++ )
//{
//    fprintf( stdout, "State %d:\n", i );
//    PRB( dd, p->pbMarks[i] );
//    PRB( dd, p->pbTrans[i] );
//}
    return p->nStates;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



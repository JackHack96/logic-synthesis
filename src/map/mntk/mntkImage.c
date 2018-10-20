/**CFile****************************************************************

  FileName    [mntkImage.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic mapped network.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: mntkImage.c,v 1.1 2005/02/28 05:34:30 alanmi Exp $]

***********************************************************************/

#include "mntkInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Mntk_NodeAddClauses( Sat_Solver_t * pSat, Mntk_Node_t * pNode );

static void Mntk_ResynthesisCone( Mntk_Man_t * p, Mntk_Node_t * pNodes[], int nNodes );
static void Mntk_ResynthesisCone_rec( Mntk_Node_t * pNode );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes the resubstitution function for the node subset.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mntk_ResynthesisImage( Mntk_Node_t * pPivot, Mntk_Node_t * ppCands[], int nCands )
{
    Mntk_Man_t * p = pPivot->p;
    Sat_Solver_t * pSat = pPivot->p->pSatImg;
//    Mntk_NodeVec_t * vWinTotal = pPivot->p->vWinTotalCone;
//    Mntk_NodeVec_t * vWinNodes = pPivot->p->vWinNodesCone;
    Mntk_NodeVec_t * vWinTotal = pPivot->p->vWinTotal;
    Mntk_NodeVec_t * vWinNodes = pPivot->p->vWinNodes;
    Mntk_Node_t * pNodes[10];
    int nBackTrackLimit = 30;
//    int nBackTrackLimit = -1;
    int i, m, v, nNodes, nMints, iMint, iVar, RetValue, clk;
    int nSols, * pSols;
    int fAddMints = 1;
    int fDelimitCones = 1;

    if ( fDelimitCones )
    {
        vWinTotal = pPivot->p->vWinTotalCone;
        vWinNodes = pPivot->p->vWinNodesCone;    
    }
    else
    {
        vWinTotal = pPivot->p->vWinTotal;    
        vWinNodes = pPivot->p->vWinNodes;
    }

    assert( nCands <= MNTK_MAX_FANINS );
    assert( nCands >= 2 );

    // load the nodes into one array
    pNodes[0] = pPivot;
    for ( i = 0; i < nCands; i++ )
        pNodes[i+1] = ppCands[i];
    nNodes = nCands + 1;

    ////////////////////////////////////////////
    // limit search to only vars in the cone
    if ( fDelimitCones )
        Mntk_ResynthesisCone( p, pNodes, nNodes );
    ////////////////////////////////////////////

    // clean the solver
    Sat_SolverClean( pSat, vWinTotal->nSize );

    // set the SAT variable numbers of the nodes
    for ( i = 0; i < vWinTotal->nSize; i++ )
        vWinTotal->pArray[i]->Num2 = i;

    // add the SAT clauses
    for ( i = 0; i < vWinNodes->nSize; i++ )
        if ( !Mntk_NodeAddClauses( pSat, vWinNodes->pArray[i] ) )
        {
            assert( 0 );
            return 0;
        }

    if ( fAddMints )
    {
    // add the clauses corresponding to the simulation info
//    Sat_SolverMarkClausesStart( pSat );
    nMints = (1 << nCands);
    for ( m = 0; m < nMints; m++ )
    {
        if ( p->uRes0All[m/32] & (1 << (m%32)) )
        {
            // the blocking clause
            Sat_IntVecClear( p->vLits );
            Sat_IntVecPush( p->vLits, SAT_VAR2LIT(pPivot->Num2, 0) );
            // add the negative minterm
            for ( v = 0; v < nCands; v++ )
                if ( m & (1 << v) )
                    Sat_IntVecPush( p->vLits, SAT_VAR2LIT(ppCands[v]->Num2, 1) );
                else
                    Sat_IntVecPush( p->vLits, SAT_VAR2LIT(ppCands[v]->Num2, 0) );
//            RetValue = Sat_SolverAddLearned( pSat, p->vLits );
            RetValue = Sat_SolverAddClause( pSat, p->vLits );
            assert( RetValue );
        }
        if ( p->uRes1All[m/32] & (1 << (m%32)) )
        {
            // the blocking clause
            Sat_IntVecClear( p->vLits );
            Sat_IntVecPush( p->vLits, SAT_VAR2LIT(pPivot->Num2, 1) );
            // add the negative minterm
            for ( v = 0; v < nCands; v++ )
                if ( m & (1 << v) )
                    Sat_IntVecPush( p->vLits, SAT_VAR2LIT(ppCands[v]->Num2, 1) );
                else
                    Sat_IntVecPush( p->vLits, SAT_VAR2LIT(ppCands[v]->Num2, 0) );
//            RetValue = Sat_SolverAddLearned( pSat, p->vLits );
            RetValue = Sat_SolverAddClause( pSat, p->vLits );
            assert( RetValue );
        }
    }
    }
//Sat_SolverPrintClauses( pSat );


//    RetValue = Sat_SolverSimplifyDB( pSat );
//    assert( RetValue );
//    if ( RetValue == 0 )
//        return 0;

    // collect the projection variables
    Sat_IntVecClear( p->vProj );
    assert( pPivot->Num2 < (unsigned)vWinTotal->nSize ); 
    Sat_IntVecPush( p->vProj, pPivot->Num2 );
    for ( i = 0; i < nCands; i++ )
    {
        assert( ppCands[i]->Num2 < (unsigned)vWinTotal->nSize );
        Sat_IntVecPush( p->vProj, ppCands[i]->Num2 );
    }

//    Sat_SolverWriteDimacs( pSat, "frg2_bug.cnf" );

    // run the solver
clk = clock();
    RetValue = Sat_SolverSolve( pSat, NULL, p->vProj, nBackTrackLimit );
//    assert( RetValue == SAT_FALSE );
    if ( RetValue != SAT_FALSE )
        return 0;
//p->timeSat += clock() - clk;
//    Sat_SolverRemoveMarked( pSat );

    // get the solutions from the solver
    nSols = Sat_SolverReadSolutions( pSat );
    pSols = Sat_SolverReadSolutionsArray( pSat );
    for ( i = 0; i < nSols; i++ )
    {
        iMint = pSols[i];
        iVar  = iMint/2;
        if ( iMint & 1 ) // positive cofactor
        {
//            assert( (p->uRes1All[iVar/32] & (1 << (iVar%32))) == 0 );
            if ( (p->uRes1All[iVar/32] & (1 << (iVar%32))) == 1 )
                return 0;
            p->uRes1All[iVar/32] |= (1 << (iVar%32));
        }
        else
        {
//            assert( (p->uRes0All[iVar/32] & (1 << (iVar%32))) == 0 );
            if ( (p->uRes0All[iVar/32] & (1 << (iVar%32))) == 1 )
                return 0;
            p->uRes0All[iVar/32] |= (1 << (iVar%32));
        }
    }

    // check that there is no overlap between the sub-images
//    for ( i = 0; i < 4; i++ )
//        if ( p->uRes0All[i] & p->uRes1All[i] )
//            return 0;
    return 1;
}
 

/**Function*************************************************************

  Synopsis    [Adds the clauses corresponding to one AND gate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mntk_NodeAddClauses( Sat_Solver_t * pSat, Mntk_Node_t * pNode )
{
    Sat_IntVec_t * vLits = pNode->p->vProj;
    Mntk_Node_t * ppNodes[10];
    int fComps[10], Vars[10];
    int RetValue, nVars, Var, i, iVar, Value;
    Mvc_Cover_t * pMvc, * pMvcC;
    Mvc_Cube_t * pCube;

    assert( !Mntk_NodeIsVar( pNode ) );
    nVars = Sat_SolverReadVarNum( pSat );

    pMvc  = Mio_GateReadMvc(pNode->pGate);
    pMvcC = Mio_GateReadMvcC(pNode->pGate);

    // get the predecessor nodes
    // get the complemented attributes of the nodes
    // get variables and check that they are in the SAT manager
    Var = pNode->Num2;
    assert( Var  < nVars ); 
    for ( i = 0; i < (int)pNode->nLeaves; i++ )
    {
        ppNodes[i] = Mntk_Regular(pNode->ppLeaves[i]);
        fComps[i]  = Mntk_IsComplement(pNode->ppLeaves[i]);
        Vars[i]    = Mntk_Regular(pNode->ppLeaves[i])->Num2;
        assert( Vars[i] < nVars );
    }

//Mvc_CoverPrint( pMvc );
//Mvc_CoverPrint( pMvcC );

    // suppose the gate is Y = F(X)
    // the ON-set is Y * F(X) + Y' * F(X)'
    // the OFF-set is Y' * F(X) + Y * F(X)'
    // the CNF is the OFF-set, in which cubes are clauses
    // and literals change their polarity
    Mvc_CoverForEachCube( pMvc, pCube )
    {
        Sat_IntVecClear( vLits );
        Sat_IntVecPush( vLits, SAT_VAR2LIT(Var, 0) );
        Mvc_CubeForEachVarValue( pMvc, pCube, iVar, Value )
        {
            if ( Value == 1 )
                Sat_IntVecPush( vLits, SAT_VAR2LIT(Vars[iVar], fComps[iVar]) );
            else if ( Value == 2 )
                Sat_IntVecPush( vLits, SAT_VAR2LIT(Vars[iVar], !fComps[iVar]) );
        }
        RetValue = Sat_SolverAddClause( pSat, vLits );
        assert( RetValue );
    }
    Mvc_CoverForEachCube( pMvcC, pCube )
    {
        Sat_IntVecClear( vLits );
        Sat_IntVecPush( vLits, SAT_VAR2LIT(Var, 1) );
        Mvc_CubeForEachVarValue( pMvcC, pCube, iVar, Value )
        {
            if ( Value == 1 )
                Sat_IntVecPush( vLits, SAT_VAR2LIT(Vars[iVar], fComps[iVar]) );
            else if ( Value == 2 )
                Sat_IntVecPush( vLits, SAT_VAR2LIT(Vars[iVar], !fComps[iVar]) );
        }
        RetValue = Sat_SolverAddClause( pSat, vLits );
        assert( RetValue );
    }
    return 1;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mntk_ResynthesisCone( Mntk_Man_t * p, Mntk_Node_t * pNodes[], int nNodes )
{
    int i;
    // mark all the SAT variables
    for ( i = 0; i < p->vWinTotal->nSize; i++ )
        p->vWinTotal->pArray[i]->fUsed = 1;
    // make sure the nodes are marked
    for ( i = 0; i < nNodes; i++ )
        assert( pNodes[i]->fUsed == 1 );
    for ( i = 0; i < p->vWinNodes->nSize; i++ )
        assert( p->vWinNodes->pArray[i]->fUsed == 1 );
    // unmark the nodes in the TFI cones of the selected nodes
    for ( i = 0; i < nNodes; i++ )
        Mntk_ResynthesisCone_rec( pNodes[i] );
    // select only unmarked
    Mntk_NodeVecClear( p->vWinTotalCone );
    for ( i = 0; i < p->vWinTotal->nSize; i++ )
        if ( p->vWinTotal->pArray[i]->fUsed == 0 )
            Mntk_NodeVecPush( p->vWinTotalCone, p->vWinTotal->pArray[i] );
    // select only unmarked
    Mntk_NodeVecClear( p->vWinNodesCone );
    for ( i = 0; i < p->vWinNodes->nSize; i++ )
        if ( p->vWinNodes->pArray[i]->fUsed == 0 )
            Mntk_NodeVecPush( p->vWinNodesCone, p->vWinNodes->pArray[i] );
    // unmark the total variables
    for ( i = 0; i < p->vWinTotal->nSize; i++ )
        p->vWinTotal->pArray[i]->fUsed = 0;
/*
    // check if the nodes have duplicated fanins
    for ( i = 0; i < p->vWinNodesCone->nSize; i++ )
    {
        int fPrint = 0;
        Mntk_Node_t * pNode = p->vWinNodesCone->pArray[i];
        int k, m;
        for ( k = 0; k < pNode->nLeaves; k++ ) 
        for ( m = k+1; m < pNode->nLeaves; m++ ) 
            if ( Mntk_Regular(pNode->ppLeaves[k]) == Mntk_Regular(pNode->ppLeaves[m]) )
            {
                printf( "Resynthesizing node %d. Node %d. Fanin %d is duplicated.\n", 
                    pNodes[0]->Num, pNode->Num, Mntk_Regular(pNode->ppLeaves[m])->Num );
                fPrint = 1;
            }
if ( fPrint )
printf( "\n" );
    }
*/
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mntk_ResynthesisCone_rec( Mntk_Node_t * pNode )
{
    int i;
    assert( !Mntk_IsComplement(pNode) );
    if ( pNode->fUsed == 0 )
        return;
    pNode->fUsed = 0;
    for ( i = 0; i < pNode->nLeaves; i++ )
        Mntk_ResynthesisCone_rec( Mntk_Regular(pNode->ppLeaves[i]) );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


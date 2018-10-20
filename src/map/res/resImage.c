/**CFile****************************************************************

  FileName    [resImage.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic resynthesis engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: resImage.c,v 1.1 2005/01/23 06:59:48 alanmi Exp $]

***********************************************************************/

#include "resInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
 
static int Fpga_PrepareArray( Fpga_Man_t * pMan, Fpga_NodeVec_t * vNodeCones, int Limit, Fpga_Node_t * pNodes[], int nNodes );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

 
/**Function*************************************************************

  Synopsis    [Computes the resubstitution function for the node subset.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Res_ResynthesisImage2( Res_Man_t * p, Fpga_Node_t * pPivot, Fpga_Node_t * ppCands[], int nCands )
{
    Fpga_Node_t * pNodes[10];
    int nBackTrackLimit = 10;
//    int nBackTrackLimit = -1;
    int i, m, v, nNodes, nMints, iMint, iVar, RetValue, clk;
    int nSols, * pSols;

    assert( nCands >= 2 );
    assert( nCands <= 7 );

    // load the nodes into one array
    pNodes[0] = pPivot;
    for ( i = 0; i < nCands; i++ )
        pNodes[i+1] = ppCands[i];
    nNodes = nCands + 1;

    // set the SAT variable numbers of the nodes
    for ( i = 0; i < p->vWinTotal->nSize; i++ )
        p->vWinTotal->pArray[i]->Num2 = i;

    // clean the solver
    Sat_SolverClean( p->pSatImg, p->vWinTotal->nSize );
    // add the SAT clauses
    for ( i = 0; i < p->vWinNodes->nSize; i++ )
    {
        assert( Fpga_NodeIsAnd( p->vWinNodes->pArray[i] ) );
        if ( !Res_NodeAddClauses( p, p->pSatImg, p->vWinNodes->pArray[i] ) )
        {
            assert( 0 );
            return 0;
        }
    }

    // add the clauses corresponding to the simulation info
//    Sat_SolverMarkClausesStart( p->pSatImg );
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
//            RetValue = Sat_SolverAddLearned( p->pSatImg, p->vLits );
            RetValue = Sat_SolverAddClause( p->pSatImg, p->vLits );
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
//            RetValue = Sat_SolverAddLearned( p->pSatImg, p->vLits );
            RetValue = Sat_SolverAddClause( p->pSatImg, p->vLits );
            assert( RetValue );
        }
    }

//    RetValue = Sat_SolverSimplifyDB( p->pSatImg );
//    assert( RetValue );
//    if ( RetValue == 0 )
//        return 0;

    // collect the projection variables
    Sat_IntVecClear( p->vProj );
    assert( pPivot->Num2 < p->vWinTotal->nSize ); 
    Sat_IntVecPush( p->vProj, pPivot->Num2 );
    for ( i = 0; i < nCands; i++ )
    {
        assert( ppCands[i]->Num2 < p->vWinTotal->nSize );
        Sat_IntVecPush( p->vProj, ppCands[i]->Num2 );
    }
//Sat_SolverWriteDimacs( p->pSatImg, "problem.cnf");

    // run the solver
clk = clock();
    RetValue = Sat_SolverSolve( p->pSatImg, NULL, p->vProj, nBackTrackLimit );
//    assert( RetValue == SAT_FALSE );
    if ( RetValue != SAT_FALSE )
        return 0;
//p->timeSat += clock() - clk;
//    Sat_SolverRemoveMarked( p->pSatImg );

    // get the solutions from the solver
    nSols = Sat_SolverReadSolutions( p->pSatImg );
    pSols = Sat_SolverReadSolutionsArray( p->pSatImg );
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

  Synopsis    [Computes the resubstitution function for the node subset.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Res_ResynthesisImage( Res_Man_t * p, Fpga_Node_t * pPivot, Fpga_Node_t * ppCands[], int nCands )
{
    Fpga_Node_t * pNodes[10];
    Fpga_NodeVec_t * vNodeCones;
    int nBackTrackLimit = 100;
//    int nBackTrackLimit = -1;
    int i, m, v, nNodes, nMints, iMint, iVar, RetValue, nNodesNew, clk;
    int nSols, * pSols;
//    int NodeLimit = 50, VarLimit = 100;
//    int NodeLimit = 500, VarLimit = 1000;
    int NodeLimit = 1000000, VarLimit = 1000000;

    assert( nCands >= 2 );
    assert( nCands <= 7 );

    // load the nodes into one array
    pNodes[0] = pPivot;
    for ( i = 0; i < nCands; i++ )
        pNodes[i+1] = ppCands[i];
    nNodes = nCands + 1;

    // collect the TFI cones of G and Fs
    vNodeCones = Fpga_MappingDfsNodes( p->pMan, pNodes, nNodes, 0 ); 
    // if the number of nodes exceed the limit, update the array
    // make sure the remaining nodes contain (pNodes,nNodes) 
    // and have their fanin numbers (Num2) set correctly
    nNodesNew = Fpga_PrepareArray( p->pMan, vNodeCones, NodeLimit, pNodes, nNodes );
    if ( nNodesNew > VarLimit )
        return 0;

    Sat_SolverClean( p->pSatImg, nNodesNew );
    for ( i = 0; i < vNodeCones->nSize; i++ )
    {
        if ( Fpga_NodeIsAnd( vNodeCones->pArray[i] ) )
            if ( !Res_NodeAddClauses( p, p->pSatImg, vNodeCones->pArray[i] ) )
            {
                assert( 0 );
                return 0;
            }
    }
    Fpga_NodeVecFree( vNodeCones );


    // prepare the solver to run incrementally on these variables
//    Sat_SolverPrepare( p->pSatImg, p->vVars );

//Extra_PrintBinary( stdout, p->uRes0All, (1 << nCands) ); printf( "\n" );
//Extra_PrintBinary( stdout, p->uRes1All, (1 << nCands) ); printf( "\n" );
//printf( "\n" );


    // add the clauses corresponding to the simulation info
//    Sat_SolverMarkClausesStart( p->pSatImg );
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
//            RetValue = Sat_SolverAddLearned( p->pSatImg, p->vLits );
            RetValue = Sat_SolverAddClause( p->pSatImg, p->vLits );
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
//            RetValue = Sat_SolverAddLearned( p->pSatImg, p->vLits );
            RetValue = Sat_SolverAddClause( p->pSatImg, p->vLits );
            assert( RetValue );
        }
    }

//    RetValue = Sat_SolverSimplifyDB( p->pSatImg );
//    assert( RetValue );
//    if ( RetValue == 0 )
//        return 0;

    // collect the projection variables
    Sat_IntVecClear( p->vProj );
    Sat_IntVecPush( p->vProj, pPivot->Num2 );
    for ( i = 0; i < nCands; i++ )
    {
//        assert( ppCands[i]->Num2 < pPivot->Num2 );
        Sat_IntVecPush( p->vProj, ppCands[i]->Num2 );
    }
//Sat_SolverWriteDimacs( p->pSatImg, "problem.cnf");

    // run the solver
clk = clock();
    RetValue = Sat_SolverSolve( p->pSatImg, NULL, p->vProj, nBackTrackLimit );
//    assert( RetValue == SAT_FALSE );
    if ( RetValue != SAT_FALSE )
        return 0;
//p->timeSat += clock() - clk;
//    Sat_SolverRemoveMarked( p->pSatImg );

    // get the solutions from the solver
    nSols = Sat_SolverReadSolutions( p->pSatImg );
    pSols = Sat_SolverReadSolutionsArray( p->pSatImg );
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
int Res_NodeAddClauses( Res_Man_t * p, Sat_Solver_t * pSat, Fpga_Node_t * pNode )
{
    Sat_IntVec_t * vLits = p->vProj;
    Fpga_Node_t * pNode1, * pNode2;
    int fComp1, fComp2, RetValue, nVars, Var, Var1, Var2;
    int fUseNum2 = 1;

    assert( Fpga_NodeIsAnd( pNode ) );
    nVars = Sat_SolverReadVarNum( pSat );

    // get the predecessor nodes
    pNode1 = Fpga_NodeReadOne( pNode );
    pNode2 = Fpga_NodeReadTwo( pNode );

    // get the complemented attributes of the nodes
    fComp1 = Fpga_IsComplement( pNode1 );
    fComp2 = Fpga_IsComplement( pNode2 );

    // determine the variable numbers
    if ( fUseNum2 ) 
    {
        Var  = pNode->Num2;
        Var1 = Fpga_Regular(pNode1)->Num2;
        Var2 = Fpga_Regular(pNode2)->Num2;
    }
    else
    {
        Var  = pNode->Num;
        Var1 = Fpga_Regular(pNode1)->Num;
        Var2 = Fpga_Regular(pNode2)->Num;
    }
    // check that the variables are in the SAT manager
    assert( Var  < nVars ); 
    assert( Var1 < nVars );
    assert( Var2 < nVars );

    // suppose the AND-gate is A * B = C
    // add !A => !C   or   A + !C
//  fprintf( pFile, "%d %d 0%c", Var1, -Var, 10 );
    Sat_IntVecClear( vLits );
    Sat_IntVecPush( vLits, SAT_VAR2LIT(Var1, fComp1) );
    Sat_IntVecPush( vLits, SAT_VAR2LIT(Var,  1) );
    RetValue = Sat_SolverAddClause( pSat, vLits );
//    assert( RetValue );
    if ( RetValue == 0 )
        return 0;

    // add !B => !C   or   B + !C
//  fprintf( pFile, "%d %d 0%c", Var2, -Var, 10 );
    Sat_IntVecClear( vLits );
    Sat_IntVecPush( vLits, SAT_VAR2LIT(Var2, fComp2) );
    Sat_IntVecPush( vLits, SAT_VAR2LIT(Var,  1) );
    RetValue = Sat_SolverAddClause( pSat, vLits );
//    assert( RetValue );
    if ( RetValue == 0 )
        return 0;

    // add A & B => C   or   !A + !B + C
//  fprintf( pFile, "%d %d %d 0%c", -Var1, -Var2, Var, 10 );
    Sat_IntVecClear( vLits );
    Sat_IntVecPush( vLits, SAT_VAR2LIT(Var1, !fComp1) );
    Sat_IntVecPush( vLits, SAT_VAR2LIT(Var2, !fComp2) );
    Sat_IntVecPush( vLits, SAT_VAR2LIT(Var, 0) );
    RetValue = Sat_SolverAddClause( pSat, vLits );
//    assert( RetValue );
    if ( RetValue == 0 )
        return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Concatenate the array of nodes in the TFI cone.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fpga_PrepareArray( Fpga_Man_t * pMan, Fpga_NodeVec_t * vNodeCones, int Limit, Fpga_Node_t * pNodes[], int nNodes )
{
    Fpga_Node_t * pNode, * pNode1, * pNode2;
    int LevelMin, LimitNew, nVarsNew, i;

    if ( vNodeCones->nSize <= Limit )
    {
        // set the numbers of nodes in the cones
        for ( i = 0; i < vNodeCones->nSize; i++ )
            vNodeCones->pArray[i]->Num2 = i;
        return vNodeCones->nSize;
    }

    assert( vNodeCones->nSize > Limit );
    // get the lowest level of the nodes in the set
    LevelMin = 100000;
    for ( i = 0; i < nNodes; i++ )
        if ( LevelMin > (int)pNodes[i]->Level )
            LevelMin = pNodes[i]->Level;
    if ( LevelMin < 2 )
    {
        // set the numbers of nodes in the cones
        for ( i = 0; i < vNodeCones->nSize; i++ )
            vNodeCones->pArray[i]->Num2 = i;
        return vNodeCones->nSize;
    }

    // find the cut point
    Fpga_MappingSortByLevel( pMan, vNodeCones, 0 );
    for ( LimitNew = Limit; LimitNew < vNodeCones->nSize; LimitNew++ )
        if ( (int)vNodeCones->pArray[LimitNew]->Level < LevelMin )
            break;
    assert( LimitNew < vNodeCones->nSize );

    // reduce the array
    vNodeCones->nSize = LimitNew;

    // assign the number (Num2) to the nodes in the array
    for ( i = 0; i < LimitNew; i++ )
    {
        vNodeCones->pArray[i]->Num2 = i;
        vNodeCones->pArray[i]->fMark1 = 1;
    }

    // and to the potential PIs
    nVarsNew = LimitNew;
    for ( i = 0; i < LimitNew; i++ )
    {
        pNode  = vNodeCones->pArray[i];
        if ( !Fpga_NodeIsAnd(pNode) )
            continue;
        pNode1 = Fpga_Regular(pNode->p1);
        pNode2 = Fpga_Regular(pNode->p2);
        if ( pNode1->fMark1 == 0 )
        {
            pNode1->fMark1 = 1;
            pNode1->Num2 = nVarsNew++;
        }
        if ( pNode2->fMark1 == 0 )
        {
            pNode2->fMark1 = 1;
            pNode2->Num2 = nVarsNew++;
        }
    }

    // make sure the candidates are included
    for ( i = 0; i < nNodes; i++ )
        assert( pNodes[i]->fMark1 );

    // remove the marks
    for ( i = 0; i < LimitNew; i++ )
    {
        pNode  = vNodeCones->pArray[i];
        pNode->fMark1  = 0;
        if ( !Fpga_NodeIsAnd(pNode) )
            continue;
        pNode1 = Fpga_Regular(pNode->p1);
        pNode2 = Fpga_Regular(pNode->p2);
        pNode1->fMark1 = 0;
        pNode2->fMark1 = 0;
    }
    return nVarsNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


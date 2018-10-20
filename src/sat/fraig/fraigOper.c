/**CFile****************************************************************

  FileName    [fraigOper.c]

  PackageName [FRAIG: Functionally reduced AND-INV graphs.]

  Synopsis    [Various Boolean operations on the graphs.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.1. Started - October 1, 2004]

  Revision    [$Id: fraigOper.c,v 1.6 2005/07/08 01:01:32 alanmi Exp $]

***********************************************************************/

#include "fraigInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Fraig_Node_t * Fraig_Cofactor_rec( Fraig_Man_t * p, Fraig_Node_t * pFunc, Fraig_Node_t * pVar );
static Fraig_Node_t * Fraig_CofactorTwo_rec( Fraig_Man_t * p, Fraig_Node_t * pFunc, Fraig_Node_t * pVar1, Fraig_Node_t * pVar2 );
static void Fraig_Cofactors_rec( Fraig_Man_t * p, Fraig_Node_t * pFunc, Fraig_Node_t * pVar );
static Fraig_Node_t * Fraig_Transfer_rec( Fraig_Man_t * pManS, Fraig_Man_t * pManD, Fraig_Node_t * pNode, int * pVarMap );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_ManIncrementTravId( Fraig_Man_t * pMan )
{
    pMan->nTravIds2++;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_NodeSetTravIdCurrent( Fraig_Man_t * pMan, Fraig_Node_t * pNode )
{
    pNode->TravId2 = pMan->nTravIds2;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_NodeIsTravIdCurrent( Fraig_Man_t * pMan, Fraig_Node_t * pNode )
{
    return pNode->TravId2 == pMan->nTravIds2;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_NodeIsTravIdPrevious( Fraig_Man_t * pMan, Fraig_Node_t * pNode )
{
    return pNode->TravId2 == pMan->nTravIds2 - 1;
}



/**Function*************************************************************

  Synopsis    [Perfoms cofactoring w.r.t. a variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_Node_t * Fraig_Cofactor( Fraig_Man_t * p, Fraig_Node_t * pFunc, Fraig_Node_t * pVar )
{
    Fraig_Node_t * pRes;
    Fraig_ManIncrementTravId( p );
    pRes = Fraig_Cofactor_rec( p, Fraig_Regular(pFunc), pVar );
    if ( Fraig_IsComplement(pFunc) )
        pRes = Fraig_Not(pRes);
    return pRes;
}

/**Function*************************************************************

  Synopsis    [Perfoms cofactoring w.r.t. a variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_Node_t * Fraig_Cofactor_rec( Fraig_Man_t * p, Fraig_Node_t * pFunc, Fraig_Node_t * pVar )
{
    Fraig_Node_t * pVarR, * pRes0, * pRes1, * pRes;
    assert( !Fraig_IsComplement(pFunc) );
    // get the regular versions of the pointer
    pVarR  = Fraig_Regular(pVar);
    // if the node does not have the var in its structural support, return the function
//    if ( !Fraig_NodeHasVarStr(pFunc, pVarR->Num) )
//        return pFunc;

    // if the node is the variable itself, return it
    if ( pFunc == pVarR )
        return Fraig_NotCond( p->pConst1, Fraig_IsComplement(pVar) );
    // if the node is another variable, return it
    if ( !Fraig_NodeIsAnd(pFunc) )
        return pFunc;

    // if the node was visited already, return the result
    if ( Fraig_NodeIsTravIdCurrent( p, pFunc ) )
        return pFunc->pData0;
    // set the traversal ID
    Fraig_NodeSetTravIdCurrent( p, pFunc );
    // solve for the children
    pRes0 = Fraig_Cofactor_rec( p, Fraig_Regular(pFunc->p1), pVar );   Fraig_Ref( pRes0 );
    pRes0 = Fraig_NotCond( pRes0, Fraig_IsComplement(pFunc->p1) );
    pRes1 = Fraig_Cofactor_rec( p, Fraig_Regular(pFunc->p2), pVar );   Fraig_Ref( pRes1 );
    pRes1 = Fraig_NotCond( pRes1, Fraig_IsComplement(pFunc->p2) );
    // get the final result
    pRes = Fraig_NodeAndCanon( p, pRes0, pRes1 );  Fraig_Ref( pRes );
    Fraig_RecursiveDeref( p, pRes0 );
    Fraig_RecursiveDeref( p, pRes1 );
    // store the result in the node
    pFunc->pData0 = pRes;
    return pRes;
}




/**Function*************************************************************

  Synopsis    [Perfoms cofactoring w.r.t. two variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_Node_t * Fraig_CofactorTwo( Fraig_Man_t * p, Fraig_Node_t * pFunc, Fraig_Node_t * pVar1, Fraig_Node_t * pVar2 )
{
    Fraig_Node_t * pRes;
    Fraig_ManIncrementTravId( p );
    pRes = Fraig_CofactorTwo_rec( p, Fraig_Regular(pFunc), pVar1, pVar2 );
    if ( Fraig_IsComplement(pFunc) )
        pRes = Fraig_Not(pRes);
    return pRes;
}

/**Function*************************************************************

  Synopsis    [Perfoms cofactoring w.r.t. a variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_Node_t * Fraig_CofactorTwo_rec( Fraig_Man_t * p, Fraig_Node_t * pFunc, Fraig_Node_t * pVar1, Fraig_Node_t * pVar2 )
{
    Fraig_Node_t * pVarR1, * pVarR2, * pRes0, * pRes1, * pRes;
    assert( !Fraig_IsComplement(pFunc) );
    // get the regular versions of the pointer
    pVarR1  = Fraig_Regular(pVar1);
    pVarR2  = Fraig_Regular(pVar2);
    // if the node does not have the var in its structural support, return the function
//    if ( !Fraig_NodeHasVarStr(pFunc, pVarR1->Num) && !Fraig_NodeHasVarStr(pFunc, pVarR2->Num) )
//        return pFunc;

    // if the node is the variable itself, return it
    if ( pFunc == pVarR1 )
        return Fraig_NotCond( p->pConst1, Fraig_IsComplement(pVar1) );
    if ( pFunc == pVarR2 )
        return Fraig_NotCond( p->pConst1, Fraig_IsComplement(pVar2) );
    // if the node is another variable, return it
    if ( !Fraig_NodeIsAnd(pFunc) )
        return pFunc;

    // if the node was visited already, return the result
    if ( Fraig_NodeIsTravIdCurrent( p, pFunc ) )
        return pFunc->pData0;
    // set the traversal ID
    Fraig_NodeSetTravIdCurrent( p, pFunc );
    // solve for the children
    pRes0 = Fraig_CofactorTwo_rec( p, Fraig_Regular(pFunc->p1), pVar1, pVar2 );   Fraig_Ref( pRes0 );
    pRes0 = Fraig_NotCond( pRes0, Fraig_IsComplement(pFunc->p1) );
    pRes1 = Fraig_CofactorTwo_rec( p, Fraig_Regular(pFunc->p2), pVar1, pVar2 );   Fraig_Ref( pRes1 );
    pRes1 = Fraig_NotCond( pRes1, Fraig_IsComplement(pFunc->p2) );
    // get the final result
    pRes = Fraig_NodeAndCanon( p, pRes0, pRes1 );  Fraig_Ref( pRes );
    Fraig_RecursiveDeref( p, pRes0 );
    Fraig_RecursiveDeref( p, pRes1 );
    // store the result in the node
    pFunc->pData0 = pRes;
    return pRes;
}



/**Function*************************************************************

  Synopsis    [Perfoms cofactoring w.r.t. a variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_Cofactors( Fraig_Man_t * p, Fraig_Node_t * pFunc, Fraig_Node_t * pVar, 
                     Fraig_Node_t ** ppCof0, Fraig_Node_t ** ppCof1 )
{
    Fraig_Node_t * pFuncR;
    Fraig_ManIncrementTravId( p );
    pFuncR = Fraig_Regular( pFunc );
    Fraig_Cofactors_rec( p, pFuncR, pVar );
    if ( Fraig_IsComplement( pFunc ) )
    {
        *ppCof0 = Fraig_Not( pFuncR->pData0 );
        *ppCof1 = Fraig_Not( pFuncR->pData1 );        
    }
    else
    {
        *ppCof0 = pFuncR->pData0;
        *ppCof1 = pFuncR->pData1;        
    }
}

/**Function*************************************************************

  Synopsis    [Perfoms cofactoring w.r.t. a variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_Cofactors_rec( Fraig_Man_t * p, Fraig_Node_t * pFunc, Fraig_Node_t * pVar )
{
    Fraig_Node_t * pChild0R, * pChild1R;
    assert( !Fraig_IsComplement(pFunc) );
    assert( !Fraig_IsComplement(pVar) );
    // if the node is a variable, return
    if ( pFunc == pVar )
    {
        pFunc->pData0 = Fraig_Not(p->pConst1);
        pFunc->pData1 = p->pConst1;  
        return;
    }
    if ( !Fraig_NodeIsAnd( pFunc ) )
    {
        pFunc->pData0 = pFunc;
        pFunc->pData1 = pFunc;  
        return;
    }

    // if the node does not have the var in its structural support, return the function
//    if ( !Fraig_NodeHasVarStr(pFunc, pVar->Num) )
//    {
//        pFunc->pData0 = pFunc;
//        pFunc->pData1 = pFunc;  
//        return;
//    }

    // if the node was visited already, return the result
    if ( Fraig_NodeIsTravIdCurrent( p, pFunc ) )
        return;
    // set the traversal ID
    Fraig_NodeSetTravIdCurrent( p, pFunc );
    // solve for the children
    Fraig_Cofactors_rec( p, pChild0R = Fraig_Regular(pFunc->p1), pVar );   
    Fraig_Cofactors_rec( p, pChild1R = Fraig_Regular(pFunc->p2), pVar );
    // compute Cof0
    pFunc->pData0 = Fraig_NodeAndCanon( p, 
        Fraig_NotCond( pChild0R->pData0, Fraig_IsComplement(pFunc->p1) ), 
        Fraig_NotCond( pChild1R->pData0, Fraig_IsComplement(pFunc->p2) ) );  Fraig_Ref( pFunc->pData0 );
    Fraig_RecursiveDeref( p, pChild0R->pData0 );
    Fraig_RecursiveDeref( p, pChild1R->pData0 );
    // compute Cof1
    pFunc->pData1 = Fraig_NodeAndCanon( p, 
        Fraig_NotCond( pChild0R->pData1, Fraig_IsComplement(pFunc->p1) ), 
        Fraig_NotCond( pChild1R->pData1, Fraig_IsComplement(pFunc->p2) ) );  Fraig_Ref( pFunc->pData1 );  
    Fraig_RecursiveDeref( p, pChild0R->pData1 );
    Fraig_RecursiveDeref( p, pChild1R->pData1 );
}

#if 0

/**Function*************************************************************

  Synopsis    [Perfoms quantification w.r.t. a variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_Node_t * Fraig_ExistAbstract( Fraig_Man_t * p, Fraig_Node_t * pFunc, Fraig_Node_t * pVar )
{
    Fraig_Node_t * pRes, * pCof0, * pCof1, * pCof0c, * pCof1c;
    int clk, nCalls;
    // compute the cofactors and reference them
clk = clock();
nCalls = p->nSatCalls;
    Fraig_Cofactors( p, pFunc, pVar, &pCof0, &pCof1 );
printf( "Calls = %d.\n", p->nSatCalls - nCalls );
p->time2 += clock() - clk;
    Fraig_Ref(pCof0);
    Fraig_Ref(pCof1);
    if ( Fraig_NodeIsConst(pCof0) || Fraig_NodeIsConst(pCof1) )
        return Fraig_NodeOr( p, pCof0, pCof1 );
    // constrain Cof0 using Cof1 as the care set
    pCof0c = Fraig_NodeConstrain( p, pCof0, pCof1  );  Fraig_Ref(pCof0c);
    Fraig_RecursiveDeref(p, pCof0);
    // constrain Cof1 using Cof0c as the care set
    pCof1c = Fraig_NodeConstrain( p, pCof1, pCof0c );  Fraig_Ref(pCof1c);
    Fraig_RecursiveDeref(p, pCof1);
    // create the new node 
    pRes = Fraig_NodeOr( p, pCof0c, pCof1c );    Fraig_Ref(pRes);
    Fraig_RecursiveDeref(p, pCof0c);
    Fraig_RecursiveDeref(p, pCof1c);
    // return the new node
printf( "Total number of nodes in the cone = %d.\n", Fraig_CountNodesOne( p, pRes ) );
    Fraig_Deref(pRes);
    return pRes;
}

/**Function*************************************************************

  Synopsis    [Perfoms quantification w.r.t. a variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_ExistAbstractTest( Fraig_Man_t * p )
{
    Fraig_Node_t * pFunc, * pVar, * pRes;
    int i, clk, Counter = 0;

    pFunc = p->vOutputs->pArray[0];
    for ( i = 0; i < p->vInputs->nSize; i++ )
        if ( Fraig_NodeHasVarStr( pFunc, i ) )
        {
            printf( "Var #%d:\n", i );
            pVar = p->vAnds->pArray[i];
clk = clock();
            pRes = Fraig_ExistAbstract( p, pFunc, pVar );
p->time1 += clock() - clk;
            if ( ++Counter == 10 )
                break;
        }
}
#endif


/**Function*************************************************************

  Synopsis    [Transfers the AIG into the new manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_Node_t * Fraig_Transfer( Fraig_Man_t * pManS, Fraig_Man_t * pManD, Fraig_Node_t * pNode, int * pVarMap )
{
    Fraig_Node_t * pRes;
    Fraig_ManIncrementTravId( pManS );
    pRes = Fraig_Transfer_rec( pManS, pManD, Fraig_Regular(pNode), pVarMap );
    if ( Fraig_IsComplement(pNode) )
        pRes = Fraig_Not(pRes);
    return pRes;
}

/**Function*************************************************************

  Synopsis    [Transfers the AIG into the new manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_Node_t * Fraig_Transfer_rec( Fraig_Man_t * pManS, Fraig_Man_t * pManD, Fraig_Node_t * pNode, int * pVarMap )
{
    Fraig_Node_t * pRes0, * pRes1, * pRes;
    assert( !Fraig_IsComplement(pNode) );
    // if the node is the variable itself, return it
    if ( Fraig_NodeIsVar(pNode) )
    {
        int Var = pVarMap? pVarMap[ pNode->NumPi ] : pNode->NumPi;
        return (Var >= 0)? pManD->vInputs->pArray[Var] : pManD->pConst1;
    }
    // if the node was visited already, return the result
    if ( Fraig_NodeIsTravIdCurrent( pManS, pNode ) )
        return pNode->pData0;
    // set the traversal ID
    Fraig_NodeSetTravIdCurrent( pManS, pNode );
    // solve for the children
    pRes0 = Fraig_Transfer_rec( pManS, pManD, Fraig_Regular(pNode->p1), pVarMap );   Fraig_Ref( pRes0 );
    pRes0 = Fraig_NotCond( pRes0, Fraig_IsComplement(pNode->p1) );
    pRes1 = Fraig_Transfer_rec( pManS, pManD, Fraig_Regular(pNode->p2), pVarMap );   Fraig_Ref( pRes1 );
    pRes1 = Fraig_NotCond( pRes1, Fraig_IsComplement(pNode->p2) );
    // get the final result
    pRes = Fraig_NodeAndCanon( pManD, pRes0, pRes1 );  Fraig_Ref( pRes );
    Fraig_RecursiveDeref( pManD, pRes0 );
    Fraig_RecursiveDeref( pManD, pRes1 );
    // store the result in the node
    pNode->pData0 = pRes;
    return pRes;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



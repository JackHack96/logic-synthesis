/**CFile****************************************************************

  FileName    [shOper.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Various operations on AND/INV graphs.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: shOper.c,v 1.5 2004/10/06 03:33:47 alanmi Exp $]

***********************************************************************/

#include "shInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern Sh_Node_t * Sh_NodeExistAbstractOne( Sh_Manager_t * pMan, Sh_Node_t * gNode, Sh_Node_t * pVar );
static void Sh_NodeExistAbstract_rec( Sh_Manager_t * pMan, Sh_Node_t * gNode, Sh_Node_t * gVar, Sh_Node_t ** pgCof0, Sh_Node_t ** pgCof1 );
static void Sh_NodeExistAbstractDeref_rec( Sh_Manager_t * pMan, Sh_Node_t * gNode );

static Sh_Node_t * Sh_NodeVarReplace_rec( Sh_Manager_t * pMan, Sh_Node_t * gNode );
static void        Sh_NodeVarReplaceDeref_rec( Sh_Manager_t * pMan, Sh_Node_t * gNode );

static void Sh_NodeCountChanged_rec( Sh_Manager_t * pMan, Sh_Node_t * gNode, int * pnTotal, int * pnChanged );
static void Sh_NodeCountChanged( Sh_Manager_t * pMan, Sh_Node_t * gNode );

static void Sh_NodeVarInfluenceCone( Sh_Manager_t * pMan, Sh_Node_t * gNode, Sh_Node_t ** pgVars, int nVars );
static void Sh_NodeVarInfluenceConeOne( Sh_Manager_t * pMan, Sh_Node_t * gNode, Sh_Node_t * gVar );
static bool Sh_NodeVarInfluenceCone_rec( Sh_Manager_t * pMan, Sh_Node_t * gNode, Sh_Node_t * gVar, int * pnTotal );


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes the square of the transition relation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Node_t * Sh_RelationSquare( Sh_Manager_t * pMan, Sh_Node_t * gRel, 
    Sh_Node_t ** pgVarsCs, Sh_Node_t ** pgVarsNs, Sh_Node_t ** pgVarsIs, int nVars )
{
    Sh_Node_t * gRel1, * gRel2, * gRes;
    gRel1 = Sh_NodeVarReplace( pMan, gRel, pgVarsNs, pgVarsIs, nVars ); shRef( gRel1 );
    gRel2 = Sh_NodeVarReplace( pMan, gRel, pgVarsCs, pgVarsIs, nVars ); shRef( gRel2 );
    gRes  = Sh_NodeAndAbstract( pMan, gRel1, gRel2, pgVarsIs, nVars );  shRef( gRes );
    Sh_RecursiveDeref( pMan, gRel1 );
    Sh_RecursiveDeref( pMan, gRel2 );
    shDeref( gRes );
    return gRes;
}


/**Function*************************************************************

  Synopsis    [Computes image by taking the product and quantifying the vars.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Node_t * Sh_NodeAndAbstract( Sh_Manager_t * pMan, 
    Sh_Node_t * gRel, Sh_Node_t * gCare, Sh_Node_t ** pgVars, int nVars )
{
    Sh_Node_t * gProd, * gRes;
    gProd = Sh_NodeAnd( pMan, gRel, gCare );                     shRef( gProd );
    gRes  = Sh_NodeExistAbstract( pMan, gProd, pgVars, nVars );  shRef( gRes );
    Sh_RecursiveDeref( pMan, gProd );
    shDeref( gRes );
    return gRes;
}

/**Function*************************************************************

  Synopsis    [Quantifies the array of variables from the AND/INV graph.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Node_t * Sh_NodeExistAbstract( Sh_Manager_t * pMan, Sh_Node_t * gNode, Sh_Node_t ** pgVars, int nVars )
{
    Sh_Node_t * gTemp, * gRes;
    int i;
//printf( "Quantifying %d variables\n", nVars );
    // start with the result being equal to the original graph
//Sh_NodeVarInfluenceCone( pMan, gNode, pgVars, nVars );
    gRes = gNode;    shRef( gRes );
//    for ( i = 0; i < nVars; i++ )
    for ( i = nVars - 1; i >= 0; i-- )
    {
        gRes = Sh_NodeExistAbstractOne( pMan, gTemp = gRes, pgVars[i] );  shRef( gRes );           
        Sh_RecursiveDeref( pMan, gTemp );
    }
    shDeref( gRes );
    return gRes;
}


/**Function*************************************************************

  Synopsis    [Quantifies one variable from the AND/INV graph.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Node_t * Sh_NodeExistAbstractOne( Sh_Manager_t * pMan, Sh_Node_t * gNode, Sh_Node_t * pVar )
{
    Sh_Node_t * gRes, * gCof0, * gCof1;

    // compute the quantification
    Sh_ManagerIncrementTravId( pMan );
    Sh_NodeExistAbstract_rec( pMan, Sh_Regular(gNode), pVar, &gCof0, &gCof1 );
    if ( Sh_IsComplement(gNode) )
    {
        gCof0 = Sh_Not(gCof0);
        gCof1 = Sh_Not(gCof1);
    }
    // compute the OR of cofactors
    gRes = Sh_NodeOr( pMan, gCof0, gCof1 );  shRef( gRes );

    // dereference the intermediate results
    Sh_ManagerIncrementTravId( pMan );
    Sh_NodeExistAbstractDeref_rec( pMan, Sh_Regular(gNode) );

    shDeref( gRes );
    return gRes;
}

/**Function*************************************************************

  Synopsis    [Quantifies one variable from the AND/INV graph.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_NodeExistAbstract_rec( Sh_Manager_t * pMan, Sh_Node_t * gNode, Sh_Node_t * pVar, 
                                Sh_Node_t ** ppCof0, Sh_Node_t ** ppCof1 )
{
    Sh_Node_t * pCof00, * pCof10, * pCof01, * pCof11;

    assert( !Sh_IsComplement(gNode) );
    if ( shNodeIsConst(gNode) )
    {
        *ppCof0 = gNode; 
        *ppCof1 = gNode;
        return;
    }
    if ( shNodeIsVar(gNode) )
    {
        if ( gNode == pVar )
        {
            *ppCof0 = Sh_Not(pMan->pConst1); 
            *ppCof1 = pMan->pConst1;
            return;
        }
        *ppCof0 = gNode; 
        *ppCof1 = gNode;
        return;
    }
    // check if the result is already computed
    if ( Sh_NodeIsTravIdCurrent(pMan, gNode) )
    {
        *ppCof0 = (Sh_Node_t *)gNode->pData; 
        *ppCof1 = (Sh_Node_t *)gNode->pData2;
        return;
    }
    Sh_NodeSetTravIdCurrent( pMan, gNode );

    // solve the problem for the cofactors
    Sh_NodeExistAbstract_rec( pMan, Sh_Regular(gNode->pOne), pVar, &pCof00, &pCof01 );
    if ( Sh_IsComplement(gNode->pOne) )
    {
        pCof00 = Sh_Not(pCof00);
        pCof01 = Sh_Not(pCof01);
    }
    Sh_NodeExistAbstract_rec( pMan, Sh_Regular(gNode->pTwo), pVar, &pCof10, &pCof11 );
    if ( Sh_IsComplement(gNode->pTwo) )
    {
        pCof10 = Sh_Not(pCof10);
        pCof11 = Sh_Not(pCof11);
    }
    *ppCof0 = Sh_NodeAnd( pMan, pCof00, pCof10 );  shRef( *ppCof0 );
    *ppCof1 = Sh_NodeAnd( pMan, pCof01, pCof11 );  shRef( *ppCof1 );

    // set the computed result
    gNode->pData  = (unsigned)*ppCof0; 
    gNode->pData2 = (unsigned)*ppCof1; 
}


/**Function*************************************************************

  Synopsis    [Dereferences intermediate results after quantification.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_NodeExistAbstractDeref_rec( Sh_Manager_t * pMan, Sh_Node_t * gNode )
{
    assert( !Sh_IsComplement(gNode) );
    if ( shNodeIsConst(gNode) )
        return;
    if ( shNodeIsVar(gNode) )
        return;
    if ( Sh_NodeIsTravIdCurrent(pMan, gNode) )
        return;
    Sh_NodeSetTravIdCurrent( pMan, gNode );
    Sh_RecursiveDeref( pMan, (Sh_Node_t *)gNode->pData  ); gNode->pData  = 0;
    Sh_RecursiveDeref( pMan, (Sh_Node_t *)gNode->pData2 ); gNode->pData2 = 0;
    Sh_NodeExistAbstractDeref_rec( pMan, Sh_Regular(gNode->pOne) );
    Sh_NodeExistAbstractDeref_rec( pMan, Sh_Regular(gNode->pTwo) );
}

/**Function*************************************************************

  Synopsis    [Replaces a set of variables by a set of variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Node_t * Sh_NodeVarReplace( Sh_Manager_t * pMan, Sh_Node_t * gNode, 
      Sh_Node_t ** ppVarsCs, Sh_Node_t ** ppVarsNs, int nVars )
{
    Sh_Node_t * gRes;
    int i;

    Sh_ManagerIncrementTravId( pMan );
    for ( i = 0; i < pMan->nVars; i++ )
        pMan->pVars[i]->pData2 = 0;
    for ( i = 0; i < nVars; i++ )
    {
        ppVarsCs[i]->pData2 = (unsigned)ppVarsNs[i];
        ppVarsNs[i]->pData2 = (unsigned)ppVarsCs[i];
    }
    gRes = Sh_NodeVarReplace_rec( pMan, Sh_Regular(gNode) );   shRef( gRes );
    if ( Sh_IsComplement(gNode) )
        gRes = Sh_Not(gRes);

    Sh_ManagerIncrementTravId( pMan );
    Sh_NodeVarReplaceDeref_rec( pMan, Sh_Regular(gNode) );

    shDeref( gRes );
    return gRes;
}

/**Function*************************************************************

  Synopsis    [Replaces a set of variables by a set of variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Node_t * Sh_NodeVarReplace_rec( Sh_Manager_t * pMan, Sh_Node_t * gNode )
{
    Sh_Node_t * pCof0, * pCof1, * gRes;

    assert( !Sh_IsComplement(gNode) );
    if ( shNodeIsConst(gNode) )
        return gNode;
    if ( shNodeIsVar(gNode) )
    {
        if ( gNode->pData2 )
            return (Sh_Node_t *)gNode->pData2;
        return gNode;
    }
    // check if the result is already computed
    if ( Sh_NodeIsTravIdCurrent(pMan, gNode) )
        return (Sh_Node_t *)gNode->pData2; 
    Sh_NodeSetTravIdCurrent( pMan, gNode );

    // solve the problem for the cofactors
    pCof0 = Sh_NodeVarReplace_rec( pMan, Sh_Regular(gNode->pOne) );
    pCof1 = Sh_NodeVarReplace_rec( pMan, Sh_Regular(gNode->pTwo) );
    if ( Sh_IsComplement(gNode->pOne) )
        pCof0 = Sh_Not(pCof0);
    if ( Sh_IsComplement(gNode->pTwo) )
        pCof1 = Sh_Not(pCof1);
    gRes = Sh_NodeAnd( pMan, pCof0, pCof1 );  shRef(gRes);

    // set the computed result
    gNode->pData2 = (unsigned)gRes;  // takes ref
    return gRes;
}

/**Function*************************************************************

  Synopsis    [Quantifies one variable from the AND/INV graph.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_NodeVarReplaceDeref_rec( Sh_Manager_t * pMan, Sh_Node_t * gNode )
{
    assert( !Sh_IsComplement(gNode) );
    if ( shNodeIsConst(gNode) )
        return;
    if ( shNodeIsVar(gNode) )
        return;
    if ( Sh_NodeIsTravIdCurrent(pMan, gNode) )
        return;
    Sh_NodeSetTravIdCurrent( pMan, gNode );
    Sh_RecursiveDeref( pMan, (Sh_Node_t *)gNode->pData2 ); gNode->pData2 = 0;
    Sh_NodeVarReplaceDeref_rec( pMan, Sh_Regular(gNode->pOne) );
    Sh_NodeVarReplaceDeref_rec( pMan, Sh_Regular(gNode->pTwo) );
}

/**Function*************************************************************

  Synopsis    [Counts the number of changed nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_NodeCountChanged( Sh_Manager_t * pMan, Sh_Node_t * gNode )
{
    int nTotal = 0, nChanged = 0;
    Sh_ManagerIncrementTravId( pMan );
    Sh_NodeCountChanged_rec( pMan, Sh_Regular(gNode), &nTotal, &nChanged );
printf( "Total = %5d. Changed = %5d.\n", nTotal, nChanged );
}

/**Function*************************************************************

  Synopsis    [Counts the number of changed nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_NodeCountChanged_rec( Sh_Manager_t * pMan, Sh_Node_t * gNode, 
    int * pnTotal, int * pnChanged )
{
    assert( !Sh_IsComplement(gNode) );
    if ( shNodeIsConst(gNode) )
        return;
    if ( shNodeIsVar(gNode) )
        return;
    // check if the result is already computed
    if ( Sh_NodeIsTravIdCurrent(pMan, gNode) )
        return; 
    Sh_NodeSetTravIdCurrent( pMan, gNode );
    if ( gNode != (Sh_Node_t *)gNode->pData2 )
        (*pnChanged)++;
    (*pnTotal)++;
    Sh_NodeCountChanged_rec( pMan, Sh_Regular(gNode->pOne), pnTotal, pnChanged );
    Sh_NodeCountChanged_rec( pMan, Sh_Regular(gNode->pTwo), pnTotal, pnChanged );
}



/**Function*************************************************************

  Synopsis    [Counts the number of changed nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_NodeVarInfluenceCone( Sh_Manager_t * pMan, Sh_Node_t * gNode, Sh_Node_t ** pgVars, int nVars )
{
    int i;
    for ( i = 0; i < nVars; i++ )
        Sh_NodeVarInfluenceConeOne( pMan, gNode, pgVars[i] );
}

/**Function*************************************************************

  Synopsis    [Counts the number of changed nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_NodeVarInfluenceConeOne( Sh_Manager_t * pMan, Sh_Node_t * gNode, Sh_Node_t * gVar )
{
    int nTotal = 0;
    Sh_ManagerIncrementTravId( pMan );
    Sh_NodeVarInfluenceCone_rec( pMan, Sh_Regular(gNode), gVar, &nTotal );
printf( "Var %3d : Influence cone = %5d.\n", Sh_NodeReadIndex(gVar), nTotal );
}

/**Function*************************************************************

  Synopsis    [Counts the number of changed nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Sh_NodeVarInfluenceCone_rec( Sh_Manager_t * pMan, Sh_Node_t * gNode, Sh_Node_t * gVar, int * pnTotal )
{
    assert( !Sh_IsComplement(gNode) );
    if ( shNodeIsConst(gNode) )
        return 0;
    if ( shNodeIsVar(gNode) )
        return (bool)( gNode == gVar );
    // check if the result is already computed
    if ( Sh_NodeIsTravIdCurrent(pMan, gNode) )
        return gNode->pData; 
    Sh_NodeSetTravIdCurrent( pMan, gNode );
    gNode->pData = 0;
    gNode->pData += Sh_NodeVarInfluenceCone_rec( pMan, Sh_Regular(gNode->pOne), gVar, pnTotal );
    gNode->pData += Sh_NodeVarInfluenceCone_rec( pMan, Sh_Regular(gNode->pTwo), gVar, pnTotal );
    if ( gNode->pData )
        (*pnTotal)++;
    return gNode->pData;
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



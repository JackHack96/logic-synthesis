/**CFile****************************************************************

  FileName    [ntkiDefault.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    []

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkiDefault.c,v 1.7 2004/04/08 05:05:04 alanmi Exp $]

***********************************************************************/

#include "ntkiInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Minimizes the nodes in order to have a good default.]

  Description [Can reduce the behavior of the nodes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkResetDefault( Ntk_Network_t * pNet )
{
    ProgressBar * pProgress;
    Ntk_Node_t * pNode;
    int AcceptType, nNodes;
    bool fVerbose = 0;

    // start the progress var
    if ( !fVerbose )
        pProgress = Extra_ProgressBarStart( stdout, Ntk_NetworkReadNodeIntNum(pNet) );

    // get the type of the current cost function
    AcceptType = Ntk_NetworkGetAcceptType( pNet );

    // go through the nodes and set the defaults
    nNodes = 0;
    Ntk_NetworkForEachNode( pNet, pNode )
    {
        Ntk_NodeResetDefault( pNode, AcceptType );
        if ( ++nNodes % 5 == 0 && !fVerbose )
            Extra_ProgressBarUpdate( pProgress, nNodes, NULL );
    }

    if ( !fVerbose )
        Extra_ProgressBarStop( pProgress );
}

/**Function*************************************************************

  Synopsis    [Forces the internal nodes to have default cover whenever possible.]

  Description [Cannot reduce the behavior of the nodes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkForceDefault( Ntk_Network_t * pNet )
{
    Ntk_Node_t * pNode;
    // go through the nodes and set the defaults
    Ntk_NetworkForEachNode( pNet, pNode )
        Ntk_NodeForceDefault( pNode );
}

/**Function*************************************************************

  Synopsis    [Computes the default cover of the internal nodes.]

  Description [Does not change the behavior of the nodes but increases
  the number of literals.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkComputeDefault( Ntk_Network_t * pNet )
{
    Ntk_Node_t * pNode;
    Cvr_Cover_t * pCvr;
    Mvc_Cover_t ** ppIsets;
    int DefValue;

    // go through the nodes and set the defaults
    Ntk_NetworkForEachNode( pNet, pNode )
    {
        pCvr = Ntk_NodeReadFuncCvr( pNode );
        ppIsets = Cvr_CoverReadIsets( pCvr );
        DefValue = Cvr_CoverReadDefault( pCvr );
        if ( DefValue >= 0 )
            ppIsets[DefValue] = Ntk_NodeComputeDefault( pNode );
        Cvr_CoverFreeFactor( pCvr );
    }
}

/**Function*************************************************************

  Synopsis    [Forces the node to have a default value.]

  Description [This function does not reduce the befavior of the node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeResetDefault( Ntk_Node_t * pNode, int AcceptType )
{
    FILE * pOutput;
    Cvr_Cover_t * pCvr, * pCvrMin;
    Mvr_Relation_t * pMvr;

    // get the output stream
    pOutput = Ntk_NodeReadMvsisOut(pNode);

    // force the default whenever possible
    Ntk_NodeForceDefault( pNode );
    // get the current Mvr
    pMvr = Ntk_NodeGetFuncMvr( pNode );
//fprintf( Ntk_NetworkReadMvsisOut(pNet), "Nodes in Mvr = %d\n", Mvr_RelationGetNodes(pMvr) );

//if ( strcmp( pNode->lFanouts.pHead->pNode->pName, "pe2" ) == 0 )
//{
//    int i = 0;
//}
    // when Mvr is derived from Cvr, if the resulting Mvr is not well-defined
    // the don't-care minterms are added to all i-sets of Mvr 
    // while the original Cvr is freed, because it is no longer valid 

    // in this case, we do not derive another Cvr
    // therefore, we "read" it and not "get" it here
    pCvr = Ntk_NodeReadFuncCvr( pNode );

    // minimize the relation with Espresso
    pCvrMin = Fnc_FunctionMinimizeCvr( Ntk_NodeReadManMvc(pNode), pMvr, 0 ); // espresso
    // if failed, try minimizing using ISOP
    if ( pCvrMin == NULL )
        pCvrMin = Fnc_FunctionMinimizeCvr( Ntk_NodeReadManMvc(pNode), pMvr, 1 ); // isop

    // consider four situations depending on the present/absence of Cvr and CvrMin
    if ( pCvr && pCvrMin )
    { // both Cvrs are avaible - choose the best one

        // compare the minimized relation with the original one
        if ( Ntk_NodeTestAcceptCvr( pCvr, pCvrMin, AcceptType, 0 ) )
        { // accept the minimized node
            // this will remove Mvr and old Cvr
            Ntk_NodeSetFuncCvr( pNode, pCvrMin );
            Ntk_NodeMakeMinimumBase( pNode );
        }
        else
        { // reject the minimized node
            Cvr_CoverFree( pCvrMin );
        }
    }
    else if ( pCvr )
    { // only the old Cvr is available - leave it as it is
    }
    else if ( pCvrMin )
    { // only the new Cvr is available - set it
        Ntk_NodeSetFuncCvr( pNode, pCvrMin );
        Ntk_NodeMakeMinimumBase( pNode );
    }
    else
    { // none of them is available
        fprintf( pOutput, "Failed to minimize MV SOP of node \"%s\" in the given resource limits.\n", 
            Ntk_NodeGetNamePrintable(pNode) );
        fprintf( pOutput, "Currently the node does not have MV SOP, only MV relation.\n" );
    }
}

/**Function*************************************************************

  Synopsis    [Decides whether to accept the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Ntk_NodeTestAcceptCvr( Cvr_Cover_t * pCvr, Cvr_Cover_t * pCvrMin, int AcceptType, bool fVerbose )
{
    int  nOld, nNew;
    bool fAccepted=FALSE;
    
    if (pCvrMin==NULL || pCvr==NULL) 
    {
        fail("SimpAcceptResult: empty covers.\n");
    }
    
    if (AcceptType == 0)   // SIMP_CUBE
    {
        nOld = Cvr_CoverReadCubeNum(pCvr);
        nNew = Cvr_CoverReadCubeNum(pCvrMin);
        if (nOld == nNew) {
            nOld = Cvr_CoverReadLitSopNum(pCvr);
            nNew = Cvr_CoverReadLitSopNum(pCvrMin);
            // use the number of leaf values as the final tie braker
            if (nOld == nNew) {
                nOld = -Cvr_CoverReadLeafValueNum(pCvr);
                nNew = -Cvr_CoverReadLeafValueNum(pCvrMin);
           }
        }
    }
    else if (AcceptType == 1)  // SIMP_SOP_LIT
    {
        nOld = Cvr_CoverReadLitSopNum(pCvr);
        nNew = Cvr_CoverReadLitSopNum(pCvrMin);
        if (nOld == nNew) {
            nOld = Cvr_CoverReadLitFacNum(pCvr);
            nNew = Cvr_CoverReadLitFacNum(pCvrMin);
            // use the number of leaf values as the final tie braker
            if (nOld == nNew) {
                nOld = -Cvr_CoverReadLeafValueNum(pCvr);
                nNew = -Cvr_CoverReadLeafValueNum(pCvrMin);
           }
        }
    }
    else if (AcceptType == 2)  // SIMP_FCT_LIT
    {
        nOld = Cvr_CoverReadLitFacNum(pCvr);
        nNew = Cvr_CoverReadLitFacNum(pCvrMin);
//printf( "New = %d.\n", nNew );

        if (nOld == nNew) {
            nOld = Cvr_CoverReadLitSopNum(pCvr);
            nNew = Cvr_CoverReadLitSopNum(pCvrMin);
            // use the number of leaf values as the final tie braker
            if (nOld == nNew) {
                nOld = -Cvr_CoverReadLeafValueNum(pCvr);
                nNew = -Cvr_CoverReadLeafValueNum(pCvrMin);
           }
        }
//Ft_TreePrint( stdout, Cvr_CoverReadTree(pCvr),    NULL, NULL );
//Ft_TreePrint( stdout, Cvr_CoverReadTree(pCvrMin), NULL, NULL );
    }
    else if (AcceptType == 3) // SIMP_ALWAYS
    { 
        fAccepted = TRUE;
    }
    else 
    {
        fail("SimpAcceptResult: Wrong acceptance criterion\n");
    }
    if (fAccepted || nNew < nOld)
        return TRUE;
    else
        return FALSE;
}

/**Function*************************************************************

  Synopsis    [Forces the node to have a default value.]

  Description [This function does not reduce the befavior of the node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeForceDefault( Ntk_Node_t * pNode )
{
    Cvr_Cover_t * pCvr;
    Mvr_Relation_t * pMvr;

    // check the case when the cover is present
    // and the default value is already set
    pCvr = Ntk_NodeReadFuncCvr( pNode );
    if ( pCvr && Cvr_CoverReadDefault(pCvr) >= 0 ) 
        return;

    // get the relation
    pMvr = Ntk_NodeGetFuncMvr( pNode );
    if ( Mvr_RelationReadMark(pMvr) )
    { // the don't-care was computed and added to the node
        fprintf( Ntk_NodeReadMvsisOut(pNode), 
            "Local MV relation of node \"%s\" is not well-defined. Don't-care is assumed.\n", 
            Ntk_NodeGetNamePrintable(pNode) );
        Mvr_RelationCleanMark( pMvr );
        // in this case, there is no way of assigning the default value 
        // without reducing behavior
        // remove the old CVR
        Ntk_NodeSetFuncMvr( pNode, pMvr );
        return;
    }
    // if the relation is ND, forcing the default value will reduce behavior
    if ( Mvr_RelationIsND(pMvr) )
        return;

    // get the cover
    pCvr = Ntk_NodeGetFuncCvr( pNode );
    // if the default value was already set while deriving Cvr, skip
    if ( Cvr_CoverReadDefault(pCvr) >= 0 ) 
        return;

    // derive the default value
    Cvr_CoverResetDefault( pCvr );
}


/**Function*************************************************************

  Synopsis    [Get the acceptable type for the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkGetAcceptType( Ntk_Network_t * pNet )
{
    char * sValue;
    int iCost;
    sValue = Cmd_FlagReadByName( pNet->pMvsis, "cost" );
    if ( sValue ) 
        iCost = atoi( sValue );
    else 
        iCost = 2;
    return iCost;
}



/**Function*************************************************************

  Synopsis    [Computes and returns the default cover.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Cover_t * Ntk_NodeComputeDefault( Ntk_Node_t * pNode )
{
    Mvr_Relation_t * pMvr;
    DdNode ** pbIsets;
    Mvc_Cover_t * pCover;
    int nInputs, Default;

    Default = Cvr_CoverReadDefault( Ntk_NodeGetFuncCvr(pNode) );
    if ( Default < 0 )
        return NULL;

    // get the relation
    pMvr = Ntk_NodeGetFuncMvr( pNode );
    // get the i-sets (the cofs w.r.t. the output var)
    nInputs = Ntk_NodeReadFaninNum(pNode);
    pbIsets = ALLOC( DdNode *, pNode->nValues );
    Mvr_RelationCofactorsDerive( pMvr, pbIsets, nInputs, pNode->nValues );
    // derive the cover for the default i-set
    pCover = Fnc_FunctionDeriveSopFromMdd( Ntk_NodeReadManMvc(pNode), pMvr, pbIsets[Default], pbIsets[Default], nInputs );
    // deref the value codes
    Mvr_RelationCofactorsDeref( pMvr, pbIsets, nInputs, pNode->nValues );
    free( pbIsets );
    return pCover;
}

/**Function*************************************************************

  Synopsis    [Computes the factored form of the default value.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ft_Node_t * Ntk_NodeFactorDefault( Ntk_Node_t * pNode )
{
    Ft_Tree_t * pTree;
    Mvc_Cover_t * pMvcDef;
    Cvr_Cover_t * pCvr;
    int Default;

    // if there is no default value
    // there is no need for factoring the default value
    pCvr = Ntk_NodeGetFuncCvr(pNode);
    Default = Cvr_CoverReadDefault( pCvr );
    if ( Default < 0 )
        return NULL;

    // if the default value has already been factored, get it
    pTree = Cvr_CoverFactor( pCvr );
    assert( pTree->pRoots[Default] == NULL );
    if ( pTree->pDefault )
        return pTree->pDefault;
    // otherwise, factor the default value

    // get the default i-set
    pMvcDef = Ntk_NodeComputeDefault(pNode);
    // factor this i-set
    pTree->pDefault = Ft_Factor( pTree, pMvcDef );
    // remove the default cover
    Mvc_CoverFree( pMvcDef );
    // return the default value FF
    return pTree->pDefault;
}



/**Function*************************************************************

  Synopsis    [Resets the default using FFs.]

  Description [Uses the factored form to compute the complement of a cover.
  Tries to phase assign the node using the cover.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkFfResetDefault( Ntk_Network_t * pNet, bool fVerbose )
{
    Ntk_Node_t * pNode;
    int AcceptType;

    // get the type of the current cost function
    AcceptType = Ntk_NetworkGetAcceptType( pNet );

    // go through the nodes and set the defaults
    Ntk_NetworkForEachNode( pNet, pNode )
        Ntk_NodeFfResetDefault( pNode, AcceptType, fVerbose );
}

/**Function*************************************************************

  Synopsis    [Resets the default using FFs.]

  Description [Uses the factored form to compute the complement of a cover.
  Tries to phase assign the node using the cover.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeFfResetDefault( Ntk_Node_t * pNode, int AcceptType, bool fVerbose )
{
    Cvr_Cover_t * pCvr;
    Ft_Tree_t * pTree;
    Mvc_Cover_t ** ppIsets;
    int DefValue;
    int nCubesBefore;
    int nCubesAfter;

    if ( pNode->nValues != 2 )
        return;

    pCvr = Ntk_NodeGetFuncCvr( pNode );
    pTree = Cvr_CoverFactor( pCvr );
    ppIsets = Cvr_CoverReadIsets( pCvr );
    DefValue = Cvr_CoverReadDefault( pCvr );
    if ( DefValue < 0 )
        return;

    // compute the default cover by complementing the non-default cover

    // compare
    nCubesBefore = Mvc_CoverReadCubeNum(ppIsets[DefValue ^ 1]);
    nCubesAfter  = Ft_UnfactorCount( pTree, DefValue ^ 1, 1 );

    if ( fVerbose )
    {
        fprintf( Ntk_NodeReadMvsisOut(pNode), "%5s : ", Ntk_NodeGetNamePrintable(pNode) );
        fprintf( Ntk_NodeReadMvsisOut(pNode), "Def = %d  ", DefValue );
        fprintf( Ntk_NodeReadMvsisOut(pNode), "Non-def cubes = %4d  ", nCubesBefore );
        fprintf( Ntk_NodeReadMvsisOut(pNode), "Def compl cubes = %5d  ", nCubesAfter );
    }

    if ( nCubesBefore <= nCubesAfter ) // no improvement
    {
        if ( fVerbose )
            fprintf( Ntk_NodeReadMvsisOut(pNode), "\n" );
    }
    else // there is some improvement
    {
        if ( fVerbose )
            fprintf( Ntk_NodeReadMvsisOut(pNode), "Accept\n" );
        Mvc_CoverFree( ppIsets[DefValue ^ 1] );

        ppIsets[DefValue ^ 1] = NULL;
        ppIsets[DefValue    ] = Ft_Unfactor( Ntk_NodeReadManMvc(pNode), pTree, DefValue ^ 1, 1 );

//        ppIsets[DefValue    ] = NULL;
//        ppIsets[DefValue ^ 1] = Ft_Unfactor( Ntk_NodeReadManMvc(pNode), pTree, DefValue ^ 1, 1 );

        Cvr_CoverFreeFactor( pCvr ); 
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



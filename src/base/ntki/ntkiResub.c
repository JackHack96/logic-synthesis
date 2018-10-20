/**CFile****************************************************************

  FileName    [ntkiResub.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Performs resubstitution for the network or for one node.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkiResub.c,v 1.9 2004/04/08 05:05:05 alanmi Exp $]

***********************************************************************/

#include "ntkiInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Ntk_Node_t *     Ntk_NetworkCompareResubNode( Ntk_Node_t * pNode, Mvr_Relation_t * pMvr, int AcceptType, int nCandsMax, bool fVerbose );
static Mvr_Relation_t * Ntk_NetworkGetResubMvr( Ntk_Node_t * pNode, Mvr_Relation_t * pMvr );
static int              Ntk_NetworkGetResubCands( Ntk_Node_t * pNode, int nCandMax );
static Mvr_Relation_t * Ntk_NetworkDeriveResubMvr( Ntk_Node_t * pNode, Mvr_Relation_t * pMvr, Ntk_Node_t * ppCands[], int nCands );
static void             Ntk_NetworkSortResubCands( Ntk_Network_t * pNet, int nCands, int nCandLimit );
static int              Ntk_NetworkResubCompareNodes( Ntk_Node_t ** ppN1, Ntk_Node_t ** ppN2 );;

static int              Ntk_NetworkMfsCompareNodes( Ntk_Node_t ** ppN1, Ntk_Node_t ** ppN2 );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs boolean resubstitution for the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkResub( Ntk_Network_t * pNet, int nCandsMax, int nFaninMax, bool fVerbose )
{
    ProgressBar * pProgress;
    Ntk_Node_t * pNode;
    int nNodes, AcceptType;

    if ( nCandsMax < 1 )
    {
        fprintf( Ntk_NetworkReadMvsisOut(pNet), "Ntk_NetworkResub(): The number of candidates to consider is %d.\n", nCandsMax );
        fprintf( Ntk_NetworkReadMvsisOut(pNet), "Resubsitution is not performed.\n" );
        return;
    }
 
    // sweep the buffers/intervers
    Ntk_NetworkSweep( pNet, 1, 1, 0, 0 );

    // start the progress var
    if ( !fVerbose )
        pProgress = Extra_ProgressBarStart( stdout, Ntk_NetworkReadNodeIntNum(pNet) );

    // get the type of the current cost function
    AcceptType = Ntk_NetworkGetAcceptType( pNet );
    // go through the nodes and set the defaults
    nNodes = 0;
    Ntk_NetworkForEachNode( pNet, pNode )
    {
        if ( Ntk_NodeReadFaninNum(pNode) < 2 )
            continue;
        if ( Ntk_NodeReadFaninNum(pNode) > nFaninMax )
            continue;
        Ntk_NetworkResubNode( pNode, Ntk_NodeGetFuncMvr(pNode), AcceptType, nCandsMax, fVerbose );
        if ( ++nNodes % 50 == 0 && !fVerbose )
            Extra_ProgressBarUpdate( pProgress, nNodes, NULL );
    }
    if ( !fVerbose )
        Extra_ProgressBarStop( pProgress );
 
    // sweep the buffers/intervers
    Ntk_NetworkSweep( pNet, 1, 1, 0, 0 );

    if ( !Ntk_NetworkCheck( pNet ) )
        fprintf( Ntk_NetworkReadMvsisOut(pNet), "Ntk_NetworkResub(): Network check has failed.\n" );
}

/**Function*************************************************************

  Synopsis    [Performs boolean resubstitution for one node.]

  Description [Derive the node after resubtitution and, if the node
  is acceptable, makes changes to the network. Returns 1 if resubstitution
  was accepted.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Ntk_NetworkResubNode( Ntk_Node_t * pNode, Mvr_Relation_t * pMvr, int AcceptType, int nCandsMax, bool fVerbose )
{
    Ntk_Node_t * pNodeNew;
    if ( nCandsMax <= 0 )
        return 0;
    // derive the new resubstituted node if it exists
    pNodeNew = Ntk_NetworkCompareResubNode( pNode, pMvr, AcceptType, nCandsMax, fVerbose );
    if ( pNodeNew )
    {
        Ntk_NodeReplace( pNode, pNodeNew );
        return 1;
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Derives the node after resubstitution.]

  Description [Compares the derived node with the original node
  and return the derived one only if it gives some improvement
  in terms of the current cost function.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NetworkCompareResubNode( Ntk_Node_t * pNode, Mvr_Relation_t * pMvr, int AcceptType, int nCandsMax, bool fVerbose )
{
    FILE * pOutput;
    Cvr_Cover_t * pCvr, * pCvrMin;
    Ntk_Node_t * pNodeResub;
    Mvr_Relation_t * pMvrResub;
    int nCands;

    // derive the resubstituted node
    pNodeResub = Ntk_NetworkGetResubNode( pNode, pMvr, nCandsMax );
    if ( pNodeResub == NULL )
        return NULL;

    // consider the current Cvr
    pCvr = Ntk_NodeReadFuncCvr( pNode );

    // minimize the resubstituted relation
    pMvrResub = Ntk_NodeReadFuncMvr( pNodeResub );
    pCvrMin = Fnc_FunctionMinimizeCvr( Ntk_NodeReadManMvc(pNode), pMvrResub, 0 ); // espresso
    if ( pCvrMin == NULL )
        pCvrMin = Fnc_FunctionMinimizeCvr( Ntk_NodeReadManMvc(pNode), pMvrResub, 1 ); // isop

    // print statistics about resubstitution
    if ( fVerbose )
    {
        pOutput = Ntk_NodeReadMvsisOut(pNode);
        fprintf( pOutput, "%5s : ", Ntk_NodeGetNamePrintable(pNode) );
        nCands = Ntk_NodeReadFaninNum(pNodeResub) - Ntk_NodeReadFaninNum(pNode);
        if ( pCvr )
            fprintf( pOutput, "CUR  cb= %3d lit= %3d  ", Cvr_CoverReadCubeNum(pCvr), Cvr_CoverReadLitFacNum(pCvr) );
        else
            fprintf( pOutput, "CUR  cb= %3s lit= %3s  ", "-", "-" );

        if ( pCvrMin )
            fprintf( pOutput, "RESUB cand = %5d   cb= %3d lit= %3d   ", nCands, Cvr_CoverReadCubeNum(pCvrMin), Cvr_CoverReadLitFacNum(pCvrMin) );
        else
            fprintf( pOutput, "RESUB cand = %5d   cb= %3s lit= %3s   ", nCands, "-", "-" );
//        fprintf( pOutput, "\n" );
    }

    // consider four situations depending on the presence/absence of Cvr and CvrMin
    if ( pCvr && pCvrMin )
    { // both Cvrs are available - choose the best one

        // compare the minimized relation with the original one
        if ( Ntk_NodeTestAcceptCvr( pCvr, pCvrMin, AcceptType, 0 ) )
        { // accept the minimized, resubstituted node
            if ( fVerbose )
                fprintf( pOutput, "Accept\n" );
            // finalize the resubstituted node
            // set the new Cvr - this will remove pMvrResub
            Ntk_NodeSetFuncCvr( pNodeResub, pCvrMin );
            Ntk_NodeMakeMinimumBase( pNodeResub );
            Ntk_NodeOrderFanins( pNodeResub );
            return pNodeResub;
        }
        else
        { // reject the resubstituted node
            if ( fVerbose )
                fprintf( pOutput, "\n" );
            Cvr_CoverFree( pCvrMin );
            Ntk_NodeDelete( pNodeResub );
            return NULL;
        }
    }
    else if ( pCvr )
    { // only the old Cvr is available - leave it as it is
        if ( fVerbose )
            fprintf( pOutput, "\n" );
        Ntk_NodeDelete( pNodeResub );
        return NULL;
    }
    else if ( pCvrMin )
    { // only the new Cvr is available - set it
        if ( fVerbose )
            fprintf( pOutput, "Accept\n" );
        // finalize the resubstituted node
        // set the new Cvr - this will remove pMvrResub
        Ntk_NodeSetFuncCvr( pNodeResub, pCvrMin );
        Ntk_NodeMakeMinimumBase( pNodeResub );
        Ntk_NodeOrderFanins( pNodeResub );
        return pNodeResub;
    }
    else
    {
        if ( fVerbose )
            fprintf( pOutput, "\n" );
        Ntk_NodeDelete( pNodeResub );
        return NULL;
    }
}

/**Function*************************************************************

  Synopsis    [Derives the node after resubstitution.]

  Description [The derived node is not minimum base and does not have
  the fanins ordered. Returns NULL if there are no resub candidates
  for the original node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NetworkGetResubNode( Ntk_Node_t * pNode, Mvr_Relation_t * pMvr, int nCandsMax )
{
    Ntk_Node_t * pNodeNew;
    Mvr_Relation_t * pMvrNew;
    Ntk_Node_t ** ppCands;
    int nCands, RetValue, i;

    // get resubustitution candidates for this node
    nCands = Ntk_NetworkGetResubCands( pNode, nCandsMax );
    if ( nCands == 0 )
        return NULL;

    // load the candidates into the array
    ppCands = ALLOC( Ntk_Node_t *, nCands );
    RetValue = Ntk_NetworkCreateArrayFromSpecial( pNode->pNet, ppCands );

    // create the resulting relation
    pMvrNew = Ntk_NetworkDeriveResubMvr( pNode, pMvr, ppCands, nCands );

    // create the resub node
    pNodeNew = Ntk_NodeClone( pNode->pNet, pNode );
    // add the additional fanins
    for ( i = 0; i < nCands; i++ )
        Ntk_NodeAddFanin( pNodeNew, ppCands[i] );
    FREE( ppCands );

    // set the var map and the relation
    Ntk_NodeWriteFuncVm( pNodeNew, Mvr_RelationReadVm(pMvrNew) );
    Ntk_NodeWriteFuncMvr( pNodeNew, pMvrNew );
    return pNodeNew;
}


/**Function*************************************************************

  Synopsis    [Selects resubustitution candidates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkGetResubCands( Ntk_Node_t * pNode, int nCandMax )
{
    Ntk_Node_t * pFanin, * pFanout;
    Ntk_Pin_t * pPin, * pPin2;
    int nCands;

    // mark the TFO cone of the node
    Ntk_NetworkComputeNodeTfo( pNode->pNet, &pNode, 1, 10000, 0, 1 );

    // increment the traversal ID
    Ntk_NetworkIncrementTravId( pNode->pNet );

    // start the linked list
    Ntk_NetworkStartSpecial( pNode->pNet );
    // go through the fanin's fanouts
    nCands = 0;
    Ntk_NodeForEachFanin( pNode, pPin, pFanin )
    {
        Ntk_NodeForEachFanout( pFanin, pPin2, pFanout )
        {
            // skip the nodes in the TFO of node
            if ( Ntk_NodeIsTravIdPrevious(pFanout) )
                continue;
            // skip the visited node
            if ( Ntk_NodeIsTravIdCurrent(pFanout) )
                continue;
            // skip the CI/CO nodes
            if ( !Ntk_NodeIsInternal(pFanout) )
                continue;
            // mark the node as visited
            Ntk_NodeSetTravIdCurrent( pFanout );

            // skip if pFanout is a fanin of pNode
            if ( Ntk_NodeReadFaninIndex( pNode, pFanout ) >= 0 )
                continue;
            // skip the nodes, whose support is not contained
            if ( !Ntk_NodeSupportContain( pNode, pFanout ) )
                continue;
            // skip the dangling nodes
            if ( !Ntk_NodeHasCoInTfo( pFanout ) )
                continue;

            // add and mark the node
            Ntk_NetworkAddToSpecial( pFanout );
            // count the node
            nCands++;
            if ( nCands >= 100 )
                break;
        }
        if ( nCands >= 100 )
            break;
    }
    // finalize the linked list
    Ntk_NetworkStopSpecial( pNode->pNet );

    // take the smallest nCandMax candidates
    if ( nCands <= nCandMax )
        return nCands;
    Ntk_NetworkSortResubCands( pNode->pNet, nCands, nCandMax );
    return nCandMax;
}

/**Function*************************************************************

  Synopsis    [Derives the relation of the node after resubstitution.]

  Description [Takes the network with the list of candidates (nCands).
  Returns the network with the list of candidates (nCandLimit), 
  the smallest by size.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkSortResubCands( Ntk_Network_t * pNet, int nCands, int nCandLimit )
{
    Ntk_Node_t ** ppNodes;
    int RetValue, i;

    assert( nCands > nCandLimit );
    ppNodes = ALLOC( Ntk_Node_t *, nCands );
    RetValue = Ntk_NetworkCreateArrayFromSpecial( pNet, ppNodes );
    assert( RetValue == nCands );

    // sort
    qsort( (void *)ppNodes, nCands, sizeof(Ntk_Node_t *), 
        (int (*)(const void *, const void *)) Ntk_NetworkResubCompareNodes );
    assert( Ntk_NetworkResubCompareNodes( ppNodes, ppNodes + nCands - 1 ) <= 0 );

    // take the largest nCandLimit resub candidates available
    Ntk_NetworkStartSpecial( pNet );
    for ( i = 0; i < nCandLimit; i++ )
        Ntk_NetworkAddToSpecial( ppNodes[i] );
    Ntk_NetworkStopSpecial( pNet );
    FREE( ppNodes );
}

/**Function*************************************************************

  Synopsis    [Compares two nodes by the number of their fanins.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkResubCompareNodes( Ntk_Node_t ** ppN1, Ntk_Node_t ** ppN2 )
{
    // compare using number of fanins
    if ( (*ppN1)->lFanins.nItems < (*ppN2)->lFanins.nItems )
        return  1;
    if ( (*ppN1)->lFanins.nItems > (*ppN2)->lFanins.nItems )
        return -1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Derives the relation of the node after resubstitution.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvr_Relation_t * Ntk_NetworkDeriveResubMvr( Ntk_Node_t * pNode, 
     Mvr_Relation_t * pMvr, Ntk_Node_t * ppCands[], int nCands )
{
    Ntk_Node_t * pFanin;
    Ntk_Pin_t * pPin;
    DdManager * dd;
    Vm_VarMap_t * pVm, * pVmNew;
    Vmx_VarMap_t * pVmx, * pVmxCand, * pVmxNew;
    DdNode * bRelNew, * bComp, * bTemp;
    Mvr_Relation_t * pMvrCand, * pMvrNew;
    int * pValues, * pValuesNew;
    int * pTransMapInv, * pTransMap;
    int i, k, nVarsIn, nVarsTotal;

    // read the parameters
    pVm     = Mvr_RelationReadVm(pMvr);
    pVmx    = Mvr_RelationReadVmx(pMvr);
    assert( pVm == Ntk_NodeReadFuncVm(pNode) );

    nVarsIn    = Vm_VarMapReadVarsInNum( pVm );
    nVarsTotal = nVarsIn + nCands + 1;
    pValues    = Vm_VarMapReadValuesArray( pVm );

    // create the var map for the resulting relation
    pValuesNew = ALLOC( int, nVarsTotal );
    pTransMapInv = ALLOC( int, nVarsTotal ); // for each new var, its place in the old map
    for ( i = 0; i < nVarsIn; i++ )
    {
        pValuesNew[i] = pValues[i];
        pTransMapInv[i] = i;
    }
    for ( i = 0; i < nCands; i++ )
    {
        pValuesNew[nVarsIn+i] = ppCands[i]->nValues;
        pTransMapInv[nVarsIn+i] = -1;
    }
    pValuesNew[nVarsIn+nCands] = pValues[nVarsIn];
    pTransMapInv[nVarsIn+nCands] = nVarsIn;

    pVmNew  = Vm_VarMapLookup( Vm_VarMapReadMan(pVm), nVarsTotal - 1, 1, pValuesNew );
    pVmxNew = Vmx_VarMapCreateExpanded( pVmx, pVmNew, pTransMapInv );
    FREE( pValuesNew );
    FREE( pTransMapInv );

    // remap the relations of the nodes and add them to the original relation
    pTransMap = ALLOC( int, nVarsTotal );  // for each old var, its place in the new map
    dd        = Mvr_RelationReadDd( pMvr );
    bRelNew   = Mvr_RelationReadRel(pMvr);  Cudd_Ref( bRelNew );
    for ( i = 0; i < nCands; i++ )
    {
        // get the relation of the node
        pMvrCand = Ntk_NodeGetFuncMvr( ppCands[i] );
        // create the var map for remapping the relation
        for ( k = 0; k < nVarsTotal; k++ )
            pTransMap[k] = -1;
        Ntk_NodeForEachFaninWithIndex( ppCands[i], pPin, pFanin, k )
        {
            pTransMap[k] = Ntk_NodeReadFaninIndex( pNode, pFanin );
            assert( pTransMap[k] >= 0 );
        }
        pTransMap[k] = nVarsIn + i; // the output
        // permute the relation
        bComp    = Mvr_RelationReadRel(pMvrCand);
        pVmxCand = Mvr_RelationReadVmx(pMvrCand);
        bComp = Mvr_RelationRemap( pMvrCand, bComp, pVmxCand, pVmxNew, pTransMap );
        Cudd_Ref( bComp );

        // add the permuted relation
        bRelNew = Cudd_bddOr( dd, bTemp = bRelNew, Cudd_Not(bComp) ); Cudd_Ref( bRelNew );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bComp );
    }
    FREE( pTransMap );

    // create the relation
    pMvrNew = Mvr_RelationCreate( Mvr_RelationReadMan(pMvr), pVmxNew, bRelNew ); 
    Cudd_Deref( bRelNew );
    // reorder the relation
    Mvr_RelationReorder( pMvrNew );
    return pMvrNew;
}



/**Function*************************************************************

  Synopsis    [Orders the internal nodes by level.]

  Description [Orders the internal nodes by level. If the flag is set to 1,
  the DFS is performed starting from the COs. In this case, levels are 
  assigned starting from the CIs. The resulting array begins with nodes 
  that are closest to the CIs and ends with the nodes that are closest 
  to the COs. If the flag is set to 0, the situation is reversed.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t ** Ntk_NetworkOrderNodesByLevel( Ntk_Network_t * pNet, bool fFromOutputs )
{
    Ntk_Node_t ** ppNodes;
    Ntk_Node_t * pNode;
    int nNodes, Level, i;

    // collect all nodes into an array
    nNodes = Ntk_NetworkReadNodeIntNum( pNet );
    ppNodes = ALLOC( Ntk_Node_t *, nNodes );
    i = 0;
    Ntk_NetworkForEachNode( pNet, pNode )
        ppNodes[i++] = pNode;
    assert( i == nNodes );

    // levelize the nodes
    Ntk_NetworkLevelizeNodes( ppNodes, nNodes, fFromOutputs );

    // collect nodes into the array by level
    i = 0;
    for ( Level = 0; Level <= pNet->nLevels + 1; Level++ )
        Ntk_NetworkForEachNodeSpecialByLevel( pNet, Level, pNode )
            if ( Ntk_NodeIsInternal(pNode) )
            {
                ppNodes[i++] = pNode;
                // set the number of literals
                pNode->pCopy = (Ntk_Node_t *)Cvr_CoverReadLitFacNum( Ntk_NodeGetFuncCvr(pNode) );

            }
    assert( i == nNodes );

    // sort the nodes by level and then by the number of literals
    qsort( (void *)ppNodes, nNodes, sizeof(Ntk_Node_t *), 
        (int (*)(const void *, const void *)) Ntk_NetworkMfsCompareNodes );
    assert( Ntk_NetworkMfsCompareNodes( ppNodes, ppNodes + nNodes - 1 ) <= 0 );

    return ppNodes;
}

/**Function*************************************************************

  Synopsis    [Orders the CO nodes by the size of their logic cone.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t ** Ntk_NetworkOrderCoNodesBySize( Ntk_Network_t * pNet )
{
    Ntk_Node_t ** ppNodes;
    Ntk_Node_t * pNode, * pFanin;
    Ntk_Pin_t * pPin;
    int nNodesCo, Cost, i;
    bool fUseNumberOfLevels = 0;

    // create the DFS order of all internal nodes
    Ntk_NetworkDfs( pNet, 1 );
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
    {
        pNode->Level = 0;
        if ( pNode->Type == MV_NODE_CI )
        {
            pNode->pCopy = NULL;
            continue;
        }
        if ( pNode->Type == MV_NODE_CO )
            Cost = (int)Ntk_NodeReadFaninNode( pNode, 0 )->pCopy;
        else
        {
            if ( fUseNumberOfLevels ) 
            {
                // use the number of logic levels
                Cost = -1;
                Ntk_NodeForEachFanin( pNode, pPin, pFanin )
                    if ( Cost < (int)pFanin->pCopy )
                        Cost = (int)pFanin->pCopy;
                Cost++;
            }
            else
            {
                Cost = Cvr_CoverReadLitFacNum( Ntk_NodeGetFuncCvr(pNode) );
                Ntk_NodeForEachFanin( pNode, pPin, pFanin )
                    Cost += (int)pFanin->pCopy;
            }
        }
        pNode->pCopy = (Ntk_Node_t *)Cost;
    }

    // collect all CO nodes into an array
    nNodesCo = Ntk_NetworkReadCoNum( pNet );
    ppNodes = ALLOC( Ntk_Node_t *, nNodesCo );
    i = 0;
    Ntk_NetworkForEachCo( pNet, pNode )
        ppNodes[i++] = pNode;
    assert( i == nNodesCo );

    // sort the nodes by level and then by the number of literals
    qsort( (void *)ppNodes, nNodesCo, sizeof(Ntk_Node_t *), 
        (int (*)(const void *, const void *)) Ntk_NetworkMfsCompareNodes );
    assert( Ntk_NetworkMfsCompareNodes( ppNodes, ppNodes + nNodesCo - 1 ) <= 0 );

    return ppNodes;
}

/**Function*************************************************************

  Synopsis    [Compares two nodes by the number of their fanins.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkMfsCompareNodes( Ntk_Node_t ** ppN1, Ntk_Node_t ** ppN2 )
{
    // compare using number of levels
    if ( (*ppN1)->Level < (*ppN2)->Level )
        return -1;
    if ( (*ppN1)->Level > (*ppN2)->Level )
        return  1;
    // now compute using the number of lits in the FF
    if ( (*ppN1)->pCopy > (*ppN2)->pCopy )
        return -1;
    if ( (*ppN1)->pCopy < (*ppN2)->pCopy )
        return  1;
    // as a tiebreaker, sort based on name
    return Ntk_NodeCompareByNameAndId( ppN1, ppN2 );
}

/**Function*************************************************************

  Synopsis    [Collects the intenal nodes in a "random" order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t ** Ntk_NetworkCollectInternal( Ntk_Network_t * pNet )
{
    Ntk_Node_t ** ppNodes;
    Ntk_Node_t * pNode;
    int nNodes, i;

    // collect all nodes into an array
    nNodes = Ntk_NetworkReadNodeIntNum( pNet );
    ppNodes = ALLOC( Ntk_Node_t *, nNodes );
    i = 0;
    Ntk_NetworkForEachNode( pNet, pNode )
        ppNodes[i++] = pNode;
    assert( i == nNodes );
    return ppNodes;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



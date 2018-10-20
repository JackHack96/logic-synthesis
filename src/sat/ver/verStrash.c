/**CFile****************************************************************

  FileName    [verStrash.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Structural hashing for verification and sweeping.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: verStrash.c,v 1.4 2004/08/17 21:34:25 satrajit Exp $]

***********************************************************************/

#include "verInt.h"


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Ver_NetworkStrashNode( Sh_Manager_t * pMan, Ntk_Node_t * pNode );
static Sh_Node_t * Ver_NetworkStrashFactor_rec( Sh_Manager_t * pMan, Ft_Tree_t * pFtTree, Ft_Node_t * pFtNode );

static void Ver_NetworkStrashNodeBdd( Sh_Manager_t * pMan, Ntk_Node_t * pNode );
static Sh_Node_t * Ver_NetworkStrashBdd_rec( Sh_Manager_t * pMan, DdManager * dd, DdNode * bFunc, Sh_Node_t ** pgVars, st_table * tVisited );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Strashes the network and derives the primary outputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Network_t * Ver_NetworkStrash( Sh_Manager_t * pMan, Ntk_Network_t * pNet, bool fUseBdds )
{
    ProgressBar * pProgress;
    Sh_Network_t * pShNet;
    Sh_Node_t ** pgInputs, ** pgOutputs;
    Sh_Node_t * gNode;
    Ntk_Node_t * pNode, * pDriver;
    int nInputs, nOutputs, nNodes, i;

    // start the strashed network
    nInputs  = Ntk_NetworkReadCiNum(pNet);
    nOutputs = Ntk_NetworkReadCoNum(pNet);
    pShNet   = Sh_NetworkCreate( pMan, nInputs, nOutputs );

    // clean the data
    Ntk_NetworkDfs( pNet, 1 );
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
        pNode->pCopy = NULL;

    // set the leaves
    i = 0;
    pgInputs = Sh_ManagerReadVars( pMan );
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        VER_STRASH_WRITE( pNode, pgInputs[i] );
        i++;
    }
    assert( i == nInputs );

    // build the network for the roots
    pProgress = Extra_ProgressBarStart( stdout, Ntk_NetworkReadNodeIntNum(pNet) );
    nNodes = 0;
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
    {
        if ( pNode->Type != MV_NODE_INT )
            continue;
        if ( fUseBdds )
            Ver_NetworkStrashNodeBdd( pMan, pNode );
        else
            Ver_NetworkStrashNode( pMan, pNode );
        if ( ++nNodes % 1000 == 0 )
            Extra_ProgressBarUpdate( pProgress, nNodes, NULL );
    }
    Extra_ProgressBarStop( pProgress );


    // derive the miter for the primary outputs of the resulting nodes
    pgOutputs = Sh_NetworkReadOutputs( pShNet );
    i = 0;
    Ntk_NetworkForEachCoDriver( pNet, pNode, pDriver )
    {
        gNode = VER_STRASH_READ( pDriver );
        pgOutputs[i++] = gNode;   shRef( gNode );
    }

    // dereference intermediate Sh_Node's
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
    {
        if ( pNode->Type != MV_NODE_INT )
            continue;
        Sh_RecursiveDeref( pMan, VER_STRASH_READ( pNode ) );
    }

    return pShNet;
}

/**Function*************************************************************

  Synopsis    [Strashes the network and derives intermediate nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Network_t * Ver_NetworkStrashInt( Sh_Manager_t * pMan, Ntk_Network_t * pNet )
{
    ProgressBar * pProgress;
    Sh_Network_t * pShNet;
    Sh_Node_t ** pgInputs, ** pgOutputs, * gNode;
    Ntk_Node_t * pNode;
    int nInputs, nOutputs, nNodes, i;

    nOutputs = 0;
    // clean the data
    Ntk_NetworkDfs( pNet, 1 );
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
    {
        pNode->pCopy = NULL;
        if ( pNode->Type == MV_NODE_INT ) nOutputs++;
    }
    
    // Instead of setting nOutputs to all internal nodes
    // we set it only to those nodes encountered along DFS
    // i.e. we cannot assert nOutputs == Ntk_NetworkReadNodeIntNum(pNet);
    //nOutputs = Ntk_NetworkReadNodeIntNum(pNet);
    
    nInputs  = Ntk_NetworkReadCiNum(pNet);

    // start the strashed network
    pShNet   = Sh_NetworkCreate( pMan, nInputs, nOutputs );

    // set the leaves
    pgInputs = Sh_ManagerReadVars( pMan );
    i = 0;
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        VER_STRASH_WRITE( pNode, pgInputs[i] );
        i++;
    }
    assert( i == nInputs );

    // build the network for the roots
    pProgress = Extra_ProgressBarStart( stdout, Ntk_NetworkReadNodeIntNum(pNet) );
    nNodes = 0;
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
    {
        if ( pNode->Type != MV_NODE_INT )
            continue;
        Ver_NetworkStrashNode( pMan, pNode );
        assert ( VER_STRASH_READ( pNode ) );
        if ( ++nNodes % 1000 == 0 )
            Extra_ProgressBarUpdate( pProgress, nNodes, NULL );
    }
    Extra_ProgressBarStop( pProgress );

    // set the outputs of the internal nodes
    pgOutputs = Sh_NetworkReadOutputs( pShNet );
    i = 0;
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
    {
        if ( pNode->Type == MV_NODE_INT )
        {
            gNode = VER_STRASH_READ( pNode );
            assert( gNode );
            pgOutputs[i++] = gNode;  //shRef( gNode );
        }
    }
    assert( i == nOutputs );

    // dereference intermediate Sh_Node's
    // no need to do so because the references are taken by the network
    return pShNet;
}

/**Function*************************************************************

  Synopsis    [Strashes the network and derives the miter.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Network_t * Ver_NetworkStrashMiter( Sh_Manager_t * pMan, Ntk_Network_t * pNet1, Ntk_Network_t * pNet2, int fPoOnly )
{
    ProgressBar * pProgress;
    Sh_Network_t * pShNet;
    Sh_Node_t ** pgInputs;
    Sh_Node_t ** pgOutputs;
    Sh_Node_t * gNode, * gNode1, * gNode2, * gExor, * gTemp;
    Ntk_Node_t * pNode1, * pNode2, * pDriver1, * pDriver2;
    int nInputs, nNodes, i;

    // get the number of network inputs
    nInputs = Ntk_NetworkReadCiNum(pNet1);
    assert( nInputs == Ntk_NetworkReadCiNum(pNet2) );

    // start the network
    pShNet  = Sh_NetworkCreate( pMan, nInputs, 1 );

    // clean the data 
    Ntk_NetworkDfs( pNet1, 1 );
    Ntk_NetworkForEachNodeSpecial( pNet1, pNode1 )
        pNode1->pCopy = NULL;
    Ntk_NetworkDfs( pNet2, 1 );
    Ntk_NetworkForEachNodeSpecial( pNet2, pNode2 )
        pNode2->pCopy = NULL;

    // set the leaves
    pgInputs = Sh_ManagerReadVars( pMan );
    i = 0;
    Ntk_NetworkForEachCi( pNet1, pNode1 )
    {
        pNode2 = Ntk_NetworkFindNodeByName( pNet2, pNode1->pName );
        VER_STRASH_WRITE( pNode1, pgInputs[i] );
        VER_STRASH_WRITE( pNode2, pgInputs[i] );
        i++;
    }
    assert( i == nInputs );


    // build the network for the roots
    pProgress = Extra_ProgressBarStart( stdout, 
        Ntk_NetworkReadNodeIntNum(pNet1) + Ntk_NetworkReadNodeIntNum(pNet2) );
    nNodes = 0;
    Ntk_NetworkForEachNodeSpecial( pNet1, pNode1 )
    {
        if ( pNode1->Type != MV_NODE_INT )
            continue;
        Ver_NetworkStrashNode( pMan, pNode1 );
        if ( ++nNodes % 1000 == 0 )
            Extra_ProgressBarUpdate( pProgress, nNodes, NULL );
    }
    Ntk_NetworkForEachNodeSpecial( pNet2, pNode2 )
    {
        if ( pNode2->Type != MV_NODE_INT )
            continue;
        Ver_NetworkStrashNode( pMan, pNode2 );
        if ( ++nNodes % 1000 == 0 )
            Extra_ProgressBarUpdate( pProgress, nNodes, NULL );
    }
    Extra_ProgressBarStop( pProgress );


    // derive the miter for the primary outputs of the resulting nodes
    gNode = Sh_Not( Sh_ManagerReadConst1( pMan ) );    shRef( gNode );
    Ntk_NetworkForEachCoDriver( pNet1, pNode1, pDriver1 )
    {
        if ( fPoOnly && pNode1->Subtype == MV_BELONGS_TO_LATCH )
            continue;
        pNode2 = Ntk_NetworkFindNodeByName( pNet2, pNode1->pName );
        pDriver2 = Ntk_NodeReadFaninNode( pNode2, 0 );
        gNode1 = VER_STRASH_READ( pDriver1 );  // the node holds the reference
        gNode2 = VER_STRASH_READ( pDriver2 );  // the node holds the reference
        if ( gNode1 != gNode2 )
        {
            gExor = Sh_NodeExor( pMan, gNode1, gNode2 );      shRef( gExor );
            gNode = Sh_NodeOr( pMan, gTemp = gNode, gExor );  shRef( gNode );
            Sh_RecursiveDeref( pMan, gTemp );
            Sh_RecursiveDeref( pMan, gExor );
        }
    }
    // write the fanin outputs first, then miter
    pgOutputs = Sh_NetworkReadOutputs( pShNet );
    pgOutputs[0] = gNode;   // takes ref

    // dereference intermediate Sh_Node's
    Ntk_NetworkForEachNodeSpecial( pNet1, pNode1 )
    {
        if ( pNode1->Type != MV_NODE_INT )
            continue;
        Sh_RecursiveDeref( pMan, VER_STRASH_READ( pNode1 ) );
    }
    Ntk_NetworkForEachNodeSpecial( pNet2, pNode2 )
    {
        if ( pNode2->Type != MV_NODE_INT )
            continue;
        Sh_RecursiveDeref( pMan, VER_STRASH_READ( pNode2 ) );
    }
    return pShNet;
}

/**Function*************************************************************

  Synopsis    [Derive the transition relation of the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Node_t * Ver_NetworkStrashTransRelation( Sh_Manager_t * pMan, Ntk_Network_t * pNet )
{
    Ntk_Node_t * pNode;
    Sh_Network_t * pShNet;
    Sh_Node_t ** pgOutputs, ** pgVars, ** pgVarsCs, ** pgVarsNs;
    Sh_Node_t * gRel, * gExor, * gTemp;
    int nInputs, nLatches, i, k;

    // get the number of network inputs
    nInputs  = Ntk_NetworkReadCiNum(pNet) - Ntk_NetworkReadLatchNum(pNet);
    nLatches = Ntk_NetworkReadLatchNum(pNet);
    pgVars   = Sh_ManagerReadVars( pMan );
    pgVarsCs = pgVars + nInputs;
    pgVarsNs = pgVars + nInputs + nLatches;

    // perform the strashing
    pShNet = Ver_NetworkStrash( pMan, pNet, 0 );

    // derive transition relation: Rel(x,cs,ns) = Prod i [nsi = NSi(x,cs)]
    i = k = 0;
    gRel = Sh_ManagerReadConst1( pMan );   shRef( gRel );
    pgOutputs = Sh_NetworkReadOutputs( pShNet );
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        if ( pNode->Subtype == MV_BELONGS_TO_NET )
        {
            k++;
            continue;
        }
        gExor = Sh_NodeExor( pMan, pgOutputs[k++], pgVarsNs[i++] );   shRef( gExor );
        gRel = Sh_NodeAnd( pMan, gTemp = gRel, Sh_Not(gExor) );       shRef( gRel );
        Sh_RecursiveDeref( pMan, gTemp );
        Sh_RecursiveDeref( pMan, gExor );
    }
    assert( i == Ntk_NetworkReadLatchNum(pNet) );

    // delete the intermediate strashed network
    Sh_NetworkFree( pShNet );
    shDeref( gRel );
    return gRel;
}

/**Function*************************************************************

  Synopsis    [Derive the transition relation of the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Node_t * Ver_NetworkStrashInitState( Sh_Manager_t * pMan, Ntk_Network_t * pNet )
{
    Ntk_Latch_t * pLatch;
    Ntk_Node_t * pNode;
    Sh_Node_t ** pgVars, ** pgVarsCs, ** pgVarsNs;
    Sh_Node_t * gInit, * gTemp;
    int nInputs, nLatches, i;

    // get the number of network inputs
    nInputs  = Ntk_NetworkReadCiNum(pNet) - Ntk_NetworkReadLatchNum(pNet);
    nLatches = Ntk_NetworkReadLatchNum(pNet);
    pgVars   = Sh_ManagerReadVars( pMan );
    pgVarsCs = pgVars + nInputs;
    pgVarsNs = pgVars + nInputs + nLatches;

    i = 0;
    gInit = pMan->pConst1;   shRef( gInit );
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        // check if it is the latch
        if ( pNode->Subtype != MV_BELONGS_TO_LATCH )
            continue;
        // find the corresponding latch
        Ntk_NetworkForEachLatch( pNet, pLatch )
            if ( pNode == Ntk_LatchReadOutput( pLatch ) )
                break;
        assert( pLatch );
        // get the reset value of this latch
        if ( Ntk_LatchReadReset( pLatch ) == 1 )
            gInit = Sh_NodeAnd( pMan, gTemp = gInit, pgVarsCs[i] );
        else
            gInit = Sh_NodeAnd( pMan, gTemp = gInit, Sh_Not(pgVarsCs[i]) );
        shRef( gInit );
        Sh_RecursiveDeref( pMan, gTemp );
        i++;
    }
    assert( i == Ntk_NetworkReadLatchNum(pNet) );
    shDeref( gInit );
    return gInit;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ver_NetworkStrashNode( Sh_Manager_t * pMan, Ntk_Node_t * pNode )
{
    Ntk_Node_t * pFanin;
    Ntk_Pin_t * pPin;
    Sh_Node_t * gNode;
    Cvr_Cover_t * pCvr;
    Ft_Tree_t * pTree;
    int i;

    // get the factored form of this node
    pCvr = Ntk_NodeGetFuncCvr( pNode );
    pTree = Cvr_CoverFactor( pCvr );

    // init the leaves the factored form
    Ntk_NodeForEachFaninWithIndex( pNode, pPin, pFanin, i )
        pTree->uLeafData[i] = (unsigned)VER_STRASH_READ( pFanin );
    // strash the factored form
    if ( pTree->pRoots[0] == NULL )
    {
        gNode = Ver_NetworkStrashFactor_rec( pMan, pTree, pTree->pRoots[1] );  shRef( gNode );
        VER_STRASH_WRITE( pNode, gNode );
    }
    else
    {
        gNode = Ver_NetworkStrashFactor_rec( pMan, pTree, pTree->pRoots[0] );  shRef( gNode );
        VER_STRASH_WRITE( pNode, Sh_Not(gNode) );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Node_t * Ver_NetworkStrashFactor_rec( Sh_Manager_t * pMan, Ft_Tree_t * pFtTree, Ft_Node_t * pFtNode )
{
    Sh_Node_t * gNode1, * gNode2, * gNode;    
    if ( pFtNode->Type == FT_NODE_LEAF )
    {
        if ( pFtNode->uData == 1 )
            return Sh_Not( pFtTree->uLeafData[pFtNode->VarNum] );
        if ( pFtNode->uData == 2 )
            return (Sh_Node_t *)pFtTree->uLeafData[pFtNode->VarNum];
        assert( 0 );
        return NULL;
    }
    if ( pFtNode->Type == FT_NODE_AND )
    {
        gNode1 = Ver_NetworkStrashFactor_rec( pMan, pFtTree, pFtNode->pOne );  shRef( gNode1 );
        gNode2 = Ver_NetworkStrashFactor_rec( pMan, pFtTree, pFtNode->pTwo );  shRef( gNode2 );
        gNode = Sh_NodeAnd( pMan, gNode1, gNode2 );                            shRef( gNode );
        Sh_RecursiveDeref( pMan, gNode1 );
        Sh_RecursiveDeref( pMan, gNode2 );
        shDeref( gNode );
        return gNode;
    }
    if ( pFtNode->Type == FT_NODE_OR )
    {
        gNode1 = Ver_NetworkStrashFactor_rec( pMan, pFtTree, pFtNode->pOne );  shRef( gNode1 );
        gNode2 = Ver_NetworkStrashFactor_rec( pMan, pFtTree, pFtNode->pTwo );  shRef( gNode2 );
        gNode = Sh_NodeOr( pMan, gNode1, gNode2 );                             shRef( gNode );
        Sh_RecursiveDeref( pMan, gNode1 );
        Sh_RecursiveDeref( pMan, gNode2 );
        shDeref( gNode );
        return gNode;
    }
    if ( pFtNode->Type == FT_NODE_0 )
        return Sh_Not( Sh_ManagerReadConst1( pMan ) );
    if ( pFtNode->Type == FT_NODE_1 )
        return Sh_ManagerReadConst1( pMan );
    assert( 0 ); // unknown node type
    return NULL;
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ver_NetworkStrashNodeBdd( Sh_Manager_t * pMan, Ntk_Node_t * pNode )
{
    Ntk_Node_t * pFanin;
    Ntk_Pin_t * pPin;
    DdManager * dd;
    Mvr_Relation_t * pMvr;
    DdNode * bOnSet, * bTemp;
    Sh_Node_t * pgVars[500];
    Sh_Node_t * gNode, * gTemp;
    st_table * tVisited;
    st_generator * gen;
    int i;

    // get the BDD of the on-set of the node
    pMvr = Ntk_NodeGetFuncMvr( pNode );
    dd = Mvr_RelationReadDd( pMvr );
    bOnSet = Mvr_RelationGetIset( pMvr, 1 );   Cudd_Ref( bOnSet );

    // init the elementary variables of the BDD
    Ntk_NodeForEachFaninWithIndex( pNode, pPin, pFanin, i )
        pgVars[i] = VER_STRASH_READ( pFanin );
    // create the strashed BDD
    tVisited = st_init_table(st_ptrcmp,st_ptrhash);
    gNode = Ver_NetworkStrashBdd_rec( pMan, dd, Cudd_Regular(bOnSet), pgVars, tVisited );
    shRef( gNode );
    if ( Cudd_IsComplement(bOnSet) )
        gNode = Sh_Not(gNode);
    Cudd_RecursiveDeref( dd, bOnSet );
    // set the result
    VER_STRASH_WRITE( pNode, gNode );
    // deref the intermediate results
    st_foreach_item( tVisited, gen, (char **)&bTemp, (char **)&gTemp )
        Sh_RecursiveDeref( pMan, gTemp );
    st_free_table( tVisited );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Node_t * Ver_NetworkStrashBdd( Sh_Manager_t * pMan, DdManager * dd, DdNode * bFunc )
{
    DdNode * bTemp;
    Sh_Node_t * gNode, * gTemp;
    st_table * tVisited;
    st_generator * gen;
    // create the strashed BDD
    tVisited = st_init_table(st_ptrcmp,st_ptrhash);
    gNode = Ver_NetworkStrashBdd_rec( pMan, dd, Cudd_Regular(bFunc), pMan->pVars, tVisited ); shRef( gNode );
    if ( Cudd_IsComplement(bFunc) )
        gNode = Sh_Not(gNode);
    // deref the intermediate results
    st_foreach_item( tVisited, gen, (char **)&bTemp, (char **)&gTemp )
        Sh_RecursiveDeref( pMan, gTemp );
    st_free_table( tVisited );
    shDeref( gNode );
    return gNode;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Node_t * Ver_NetworkStrashBdd_rec( Sh_Manager_t * pMan, 
    DdManager * dd, DdNode * bFunc, Sh_Node_t ** pgVars, st_table * tVisited )
{
    Sh_Node_t * gNode1, * gNode2, * gNode;
    assert( !Cudd_IsComplement(bFunc) );
    if ( bFunc == dd->one )
        return pMan->pConst1;
    if ( st_lookup( tVisited, (char *)bFunc, (char **)&gNode ) )
        return gNode;
    gNode1 = Ver_NetworkStrashBdd_rec( pMan, dd, Cudd_Regular(cuddE(bFunc)), pgVars, tVisited );
    if ( Cudd_IsComplement(cuddE(bFunc)) )
        gNode1 = Sh_Not(gNode1);
    gNode2 = Ver_NetworkStrashBdd_rec( pMan, dd, cuddT(bFunc), pgVars, tVisited );
    gNode = Sh_NodeMux( pMan, pgVars[bFunc->index], gNode2, gNode1 );   shRef( gNode );
    st_insert( tVisited, (char *)bFunc, (char *)gNode );
    return gNode;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


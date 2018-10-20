/**CFile****************************************************************

  FileName    [ntkMdd.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Procedures to derive nodes and networks from MDDs.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkMdd.c,v 1.2 2004/01/06 21:02:53 alanmi Exp $]

***********************************************************************/

#include "ntkInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Ntk_Node_t ** Ntk_NetworkCreateFromMddDeriveTerms( DdManager * dd, Vmx_VarMap_t * pVmx, Ntk_Node_t * ppNodes[], int nNodes );
static Ntk_Node_t *  Ntk_NetworkCreateFromMdd_rec( Ntk_Network_t * pNet, DdManager * dd, DdNode * aFunc, Ntk_Node_t ** ppNodesBin, st_table * tNodes );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates the node with the given fanins and given MDD.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NodeCreateFromMdd( Ntk_Network_t * pNet, 
    Ntk_Node_t * pFanins[], int nFanins, DdManager * ddDc, DdNode * bOnSet )
{
    Ntk_Node_t * pNodeNew;
    Vm_VarMap_t * pVm;
    Vmx_VarMap_t * pVmx;
    Mvr_Relation_t * pMvr;
    DdNode * bRelation, * bTemp;
    Mvr_Manager_t * pMvrMan;
    DdManager * ddLoc;
    int nVarsBin, i;

    // create the node with the fanins being CIs and this function
    pNodeNew = Ntk_NodeCreate( pNet, "DC", MV_NODE_INT, 2 );
//    Ntk_NetworkForEachCi( pNet, pNodeCi )
    for ( i = 0; i < nFanins; i++ )
        Ntk_NodeAddFanin( pNodeNew, pFanins[i] );

    // get the variable map
    pVm = Ntk_NodeAssignVm( pNodeNew );
    pVmx = Vmx_VarMapLookup( Ntk_NodeReadManVmx(pNodeNew), pVm, -1, NULL );
    nVarsBin = Vmx_VarMapReadBitsNum(pVmx);

    // reside the manager if necessary
    pMvrMan = Ntk_NodeReadManMvr(pNodeNew);
    ddLoc = Mvr_ManagerReadDdLoc( pMvrMan );
    if ( ddLoc->size < nVarsBin )
    {
        for ( i = ddLoc->size; i < nVarsBin; i++ )
            Cudd_bddNewVar( ddLoc );
        Cudd_zddVarsFromBddVars( ddLoc, 2 );
    }

    // get the BDD of unreachable in the relation manager
//    bRelation = Extra_TransferPermute( ddDc, ddLoc, bUnreach, ddDc->perm );   Cudd_Ref( bRelation ); 
    bRelation = Cudd_bddTransfer( ddDc, ddLoc, bOnSet );                            Cudd_Ref( bRelation ); 

    // create the relation with this BDD
    bRelation = Cudd_bddXnor( ddLoc, bTemp = bRelation, ddLoc->vars[nVarsBin-1] );  Cudd_Ref( bRelation );
    Cudd_RecursiveDeref( ddLoc, bTemp );
    pMvr = Mvr_RelationCreate( pMvrMan, pVmx, bRelation );
    Cudd_RecursiveDeref( ddLoc, bRelation );

    // set the functionality
    Ntk_NodeSetFuncVm( pNodeNew, pVm );
    Ntk_NodeSetFuncMvr( pNodeNew, pMvr );

    // make the node minimum base
    Ntk_NodeOrderFanins( pNodeNew );
    Ntk_NodeMakeMinimumBase( pNodeNew );
    Ntk_NodeReorderMvr( pNodeNew );
    return pNodeNew;
}

/**Function*************************************************************

  Synopsis    [Creates the network isomorphic to the node's MDD.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Network_t * Ntk_NetworkCreateFromMdd( Ntk_Node_t * pNode )
{
    DdManager * dd;
    Ntk_Network_t * pNet = pNode->pNet;
    Ntk_Network_t * pNetNew;
    Mvr_Manager_t * pMvrMan;
    Mvr_Relation_t * pMvr;
    Vmx_VarMap_t * pVmx;
    Ntk_Node_t ** ppNodesNew, ** ppNodesBin;
    Ntk_Node_t * ppNodesConst[2];
    Ntk_Node_t * pNodeRoot, * pFanin;
    Ntk_Pin_t * pPin;
    DdNode * aMdd, * bMdd;
    st_table * tNodes;
    int nNodesIn, nNodesBin, i, n;
    char * pName;

    assert( pNode->nValues == 2 );

    // get the manager
    pMvrMan = Ntk_NodeReadManMvr( pNode );
    dd = Mvr_ManagerReadDdLoc( pMvrMan );

    // get the relation and variable maps
    pMvr = Ntk_NodeGetFuncMvr( pNode );
    assert( !Mvr_RelationIsND( pMvr ) );
    pVmx = Mvr_RelationReadVmx( pMvr );

    // start the network
    pNetNew = Ntk_NetworkAlloc( pNet->pMvsis );

    // copy and add the CI nodes
    nNodesIn = Ntk_NodeReadFaninNum( pNode );
    ppNodesNew = ALLOC( Ntk_Node_t *, nNodesIn );
    Ntk_NodeForEachFaninWithIndex( pNode, pPin, pFanin, i )
    {
        ppNodesNew[i] = Ntk_NodeDup( pNetNew, pFanin );
        ppNodesNew[i]->Subtype = MV_BELONGS_TO_NET;
        Ntk_NetworkAddNode( pNetNew, ppNodesNew[i], 1 );
    }
    assert( i == nNodesIn );

    // create and add the buffer nodes
    nNodesBin = Vmx_VarMapBitsNumRange( pVmx, 0, Vm_VarMapReadVarsInNum( Vmx_VarMapReadVm( pVmx ) ) );
    ppNodesBin = Ntk_NetworkCreateFromMddDeriveTerms( dd, pVmx, ppNodesNew, nNodesIn );
    for ( n = 0; n < nNodesBin + 1; n++ )
        if ( ppNodesBin[n] ) // this is the node corresponding to the input var
            Ntk_NetworkAddNode( pNetNew, ppNodesBin[n], 1 );
    FREE( ppNodesNew );

    // create the constant nodes
    ppNodesConst[0] = Ntk_NodeCreateConstant( pNetNew, 2, 1 );
    ppNodesConst[1] = Ntk_NodeCreateConstant( pNetNew, 2, 2 );

    // add the constant nodes
    Ntk_NetworkAddNode( pNetNew, ppNodesConst[0], 1 );
    Ntk_NetworkAddNode( pNetNew, ppNodesConst[1], 1 );

    // create the internal nodes of the DC network
    bMdd = Mvr_RelationGetIset( pMvr, 1 );        Cudd_Ref( bMdd );
    aMdd = Cudd_BddToAdd( dd, bMdd );             Cudd_Ref( aMdd );
    Cudd_RecursiveDeref( dd, bMdd );
//PRN( aMdd );
    
    // start the table mapping ADD nodes into MVSIS nodes
    tNodes = st_init_table( st_ptrcmp,st_ptrhash );
    st_insert( tNodes, (char *)(a0), (char *)ppNodesConst[0] );
    st_insert( tNodes, (char *)(a1), (char *)ppNodesConst[1] );
    pNodeRoot = Ntk_NetworkCreateFromMdd_rec( pNetNew, dd, aMdd, ppNodesBin, tNodes );
    Cudd_RecursiveDeref( dd, aMdd );
    st_free_table( tNodes );
    FREE( ppNodesBin );

    // create the only CO node
    pName = (pNode->pName? pNode->pName: "DC");
    pName = Ntk_NetworkRegisterNewName( pNetNew, pName );
    Ntk_NodeAssignName( pNodeRoot, pName );
    Ntk_NetworkAddNodeCo( pNetNew, pNodeRoot, 1 );

    // put IDs into a proper order
    Ntk_NetworkReassignIds( pNetNew );
    Ntk_NetworkOrderFanins( pNetNew );

    // check the network
    if ( !Ntk_NetworkCheck( pNetNew ) )
       fprintf( Ntk_NetworkReadMvsisOut(pNetNew), "Ntk_NetworkCreateExdc(): Network check has failed.\n" );

    Ntk_NetworkSweep( pNetNew, 1, 1, 0, 0 );
    return pNetNew;
}

/**Function*************************************************************

  Synopsis    [Derives the network from the MDD.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NetworkCreateFromMdd_rec( Ntk_Network_t * pNet, DdManager * dd, DdNode * aFunc, 
    Ntk_Node_t ** ppNodesBin, st_table * tNodes )
{
    Ntk_Node_t * pNode1, * pNode2, * pNode;
    if ( st_lookup( tNodes, (char *)aFunc, (char **)&pNode ) )
        return pNode;
    pNode1 = Ntk_NetworkCreateFromMdd_rec( pNet, dd, cuddT(aFunc), ppNodesBin, tNodes );
    pNode2 = Ntk_NetworkCreateFromMdd_rec( pNet, dd, cuddE(aFunc), ppNodesBin, tNodes );
    pNode  = Ntk_NodeCreateMuxBinary( pNet, ppNodesBin[aFunc->index], pNode1, pNode2 );
    Ntk_NetworkAddNode( pNet, pNode, 1 );
    st_insert( tNodes, (char *)aFunc, (char *)pNode );
    return pNode;
}

/**Function*************************************************************

  Synopsis    [Derives the MV terminal nodes for the BDD.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t ** Ntk_NetworkCreateFromMddDeriveTerms( DdManager * dd, Vmx_VarMap_t * pVmx, Ntk_Node_t * ppNodes[], int nNodes )
{
    Vm_VarMap_t * pVm;
    Ntk_Node_t ** ppNodesBin;
    DdNode ** pbCodes, ** pbCodesTemp, ** pbCodesOne, * bVar;
    int nNodesBin, nVarsMvIn, nValues, nValuesTotal, iVar;
    int * pValuesFirst;
    int * pBitsOrder, * pBits, * pBitsFirst;
    unsigned Values[2];
    int n, b, v;

    // get the number of binary nodes and their order
    pVm = Vmx_VarMapReadVm( pVmx );
    nVarsMvIn = Vm_VarMapReadVarsInNum( pVm );
    pValuesFirst = Vm_VarMapReadValuesFirstArray( pVm );
    assert( nVarsMvIn == nNodes );
    nNodesBin = Vmx_VarMapBitsNumRange( pVmx, 0, nVarsMvIn );
    pBitsOrder = Vmx_VarMapReadBitsOrder( pVmx );
    pBits = Vmx_VarMapReadBits( pVmx );
    pBitsFirst = Vmx_VarMapReadBitsFirst( pVmx );

    // get the codes of the MV variables
    pbCodesTemp = Vmx_VarMapEncodeMap( dd, pVmx );
    // copy the codes
    nValuesTotal = Vm_VarMapReadValuesNum( pVm );
    pbCodes = ALLOC( DdNode *, nValuesTotal );
    for ( v = 0; v < nValuesTotal; v++ )
    {
        pbCodes[v] = pbCodesTemp[v];  Cudd_Ref( pbCodes[v] );
    }
    // deref the codes
    Vmx_VarMapEncodeDeref( dd, pVmx, pbCodesTemp );

    // allocate place for the new nodes
    ppNodesBin = ALLOC( Ntk_Node_t *, nNodesBin + 1 );
    memset( ppNodesBin, 0, sizeof(Ntk_Node_t *) * (nNodesBin + 1) );

    // create the MV-input binary output nodes for each MV var
    for ( n = 0; n < nNodes; n++ )
    {
        pbCodesOne = pbCodes + pValuesFirst[n];
        nValues = ppNodes[n]->nValues;
        for ( b = 0; b < pBits[n]; b++ )
        {
            // get the binary var corresponding to this bit
            iVar = pBitsOrder[ pBitsFirst[n] + b ];
            bVar = dd->vars[ iVar ];

            // collect the values, whose code has this bit
            Values[0] = Values[1] = 0;
            for ( v = 0; v < nValues; v++ )
            {
                if ( Cudd_bddLeq( dd, pbCodesOne[v], Cudd_Not(bVar) ) )
                    Values[0] |= (1 << v);
                else if ( Cudd_bddLeq( dd, pbCodesOne[v], bVar ) )
                    Values[1] |= (1 << v);
                else
                {
                    Values[0] |= (1 << v);
                    Values[1] |= (1 << v);
                }
            }

            // get the nodes corresponding to this bit
            assert( ppNodesBin[iVar] == NULL );
            ppNodesBin[iVar] = Ntk_NodeCreateOneInputNode( ppNodes[n]->pNet, ppNodes[n], nValues, 2, Values );
        }
    }
    for ( v = 0; v < nValuesTotal; v++ )
        Cudd_RecursiveDeref( dd, pbCodes[v] );
    FREE( pbCodes );

    return ppNodesBin;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



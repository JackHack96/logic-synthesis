/**CFile****************************************************************

  FileName    [ntkiDsd.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Interface to Disjoint Support Decomposition (DSD). 
  This fast BDD-based recursive algorithm for simple 
  (single-output) DSD is based on the following papers:
  (1) V. Bertacco and M. Damiani, "Disjunctive decomposition of 
  logic functions," Proc. ICCAD '97, pp. 78-82.
  (2) Y. Matsunaga, "An exact and efficient algorithm for disjunctive 
  decomposition", Proc. SASIMI '98, pp. 44-50.
  The scope of detected decompositions is the same as in the paper:
  T. Sasao and M. Matsuura, "DECOMPOS: An integrated system for 
  functional decomposition," Proc. IWLS '98, pp. 471-477.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkiDsd.c,v 1.3 2005/07/08 01:01:22 alanmi Exp $]

***********************************************************************/

#include "ntkiInt.h"
#include "dsd.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Dsd_Manager_t * Ntk_NetworkDsdPerform( Ntk_Network_t * pNet, bool fVerbose );
static Ntk_Network_t * Ntk_NetworkDsdInternal( Ntk_Network_t * pNet, bool fUseNand, bool fVerbose );
static Ntk_Node_t *    Ntk_NetworkDsdNode( Ntk_Network_t * pNet, Ntk_Node_t * pNodeGlo, Ntk_Node_t * ppNodes[], DdManager * ddDsd, Dsd_Node_t * pNodeDsd, bool fUseNand );
static void            Ntk_NetworkPrintDsdInternal( Ntk_Network_t * pNet, int Output, bool fShort );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Derives the DSD network.]

  Description [Takes the binary network (pNet), derives global BDDs for
  the primary outputs of this network, and decomposes these BDDs using
  disjoint support decomposition. Finally, constructs and return a new 
  network, which is topologically equivalent to the decomposition tree.
  Allocates and frees a new BDD manager and a new DSD manager.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Network_t * Ntk_NetworkDsd( Ntk_Network_t * pNet, bool fUseNand, bool fVerbose )
{
    Ntk_Network_t * pNetDsd;
    bool fDynEnable = 1;
    bool fLatchOnly = 0;

    // free the old global manager if necessary
    Ntk_NetworkSetDdGlo( pNet, NULL );
    // compute the global functions
    Ntk_NetworkGlobalComputeCo( pNet, fDynEnable, fLatchOnly, fVerbose );
    // create the collapsed network
    pNetDsd = Ntk_NetworkDsdInternal( pNet, fUseNand, fVerbose );
    // undo the global functions
    Ntk_NetworkGlobalDeref( pNet );
    return pNetDsd;
}

/**Function*************************************************************

  Synopsis    [Constructs the decomposed network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Network_t * Ntk_NetworkDsdInternal( Ntk_Network_t * pNet, bool fUseNand, bool fVerbose )
{
    Ntk_Node_t * pNodeGlo;
    Dsd_Manager_t * pManDsd;
    Dsd_Node_t * pNodeDsd, * pNodeDsdR;
    Dsd_Node_t ** ppNodesDsd;
    DdManager * ddMvr;
    int nNodesDsd, nDsdFaninMax, nDecs, Index, i;
    Ntk_Network_t * pNetNew;
    Ntk_Pin_t * pPin;
    Ntk_Node_t * pNode, * pNodeNew, * pNodeNewInv, * pNodeNewCo, * pFanin;
    Ntk_Node_t ** ppNodesNet;
    Ntk_Latch_t * pLatch, * pLatchNew;

    // perform the decomposition
    pManDsd = Ntk_NetworkDsdPerform( pNet, fVerbose );
    if ( pManDsd == NULL )
        return NULL;


    // extend MVR manager if necessary
    Dsd_TreeNodeGetInfo( pManDsd, NULL, &nDsdFaninMax );
    ddMvr = Ntk_NetworkReadManDdLoc( pNet );
    for ( i = ddMvr->size; i < nDsdFaninMax + 1; i++ )
        Cudd_bddIthVar( ddMvr, i );


    // allocate the empty network
    pNetNew = Ntk_NetworkAlloc( pNet->pMvsis );
    // register the name 
    pNetNew->pName = Ntk_NetworkRegisterNewName( pNetNew, pNet->pName );
    // register the network spec file name
    if ( pNet->pSpec )
        pNetNew->pSpec = Ntk_NetworkRegisterNewName( pNetNew, pNet->pSpec );

    // copy and add the CI nodes
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        // copy the node
        pNodeNew = Ntk_NodeDup( pNetNew, pNode );
        // clean the copy field to store the invertor of this node if available
        pNodeNew->pCopy = NULL;
        // add the node to the network
        Ntk_NetworkAddNode( pNetNew, pNodeNew, 1 );
    }


    // create the global node
    pNodeGlo = Ntk_NodeCreateFromNetwork( pNet, NULL );

    // adjust the global node to point to the CIs of the new network
    Ntk_NodeForEachFanin( pNodeGlo, pPin, pFanin )
    {
        pPin->pNode = Ntk_NetworkFindNodeByName( pNetNew, pFanin->pName );
        assert( pPin->pNode );
    }


    // collect DSD nodes in DFS order (leaves and const1 is not collected)
    ppNodesDsd = Dsd_TreeCollectNodesDfs( pManDsd, &nNodesDsd );
    // assign numbers to each DSD node in this order
    for ( i = 0; i < nNodesDsd; i++ )
        Dsd_NodeSetMark( ppNodesDsd[i], i );
    // allocate storage for MVSIS nodes
    ppNodesNet = ALLOC( Ntk_Node_t *, nNodesDsd );
    memset( ppNodesNet, 0, sizeof(Ntk_Node_t *) * nNodesDsd );
   // create MVSIS nodes
    for ( i = 0; i < nNodesDsd; i++ )
    {
        assert( Dsd_NodeReadDecsNum(ppNodesDsd[i]) > 1 );
        ppNodesNet[i] = Ntk_NetworkDsdNode( pNetNew, pNodeGlo, ppNodesNet, 
            Ntk_NetworkReadDdGlo(pNet), ppNodesDsd[i], fUseNand );
        Ntk_NetworkAddNode( pNetNew, ppNodesNet[i], 1 );
    }


    // create and add the internal and CO nodes
    i = 0;
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        // get the corresponding root
        pNodeDsd  = Dsd_ManagerReadRoot( pManDsd, i );
        pNodeDsdR = Dsd_Regular(pNodeDsd);

        // get the corresponding MVSIS node
        nDecs = Dsd_NodeReadDecsNum(pNodeDsdR);
        Index = (Dsd_NodeReadSupp(pNodeDsdR))->index;
        if ( nDecs == 0 )
        {
            pNodeNew = Ntk_NodeCreateConstant( pNet, 2, 2 );
            Ntk_NetworkAddNode( pNetNew, pNodeNew, 1 );
        }
        else if ( nDecs == 1 )
            pNodeNew = Ntk_NodeReadFaninNode( pNodeGlo, Index );
        else
            pNodeNew = ppNodesNet[ Dsd_NodeReadMark(pNodeDsdR) ];
        assert( pNodeNew );

        // add buffer if necessary
        if ( pNodeDsdR != pNodeDsd )
        {
            if ( pNodeNew->pCopy == NULL )
            { // the invertor is not avaiable - create it
                pNodeNewInv = Ntk_NodeCreateOneInputBinary( pNetNew, pNodeNew, 1 );
                Ntk_NetworkAddNode( pNetNew, pNodeNewInv, 1 );
                pNodeNew->pCopy = pNodeNewInv; // remember the intertor
                pNodeNew = pNodeNewInv;
            }
            else 
                pNodeNew = pNodeNew->pCopy; 
        }
        // pNodeNew is already in the network

        // add the CO node
//        Ntk_NetworkAddNodeCo( pNetNew, pNodeNew, 1 );
        // copy the CO node (it is important if latches are present - see below)
        pNodeNewCo = Ntk_NodeDup( pNetNew, pNode );
        // set the only fanin to be equal to the new internal node
        pNodeNewCo->lFanins.pHead->pNode = pNodeNew;
        // add the CO node
        Ntk_NetworkAddNode( pNetNew, pNodeNewCo, 1 );
        i++;
    }
    FREE( ppNodesDsd );
    FREE( ppNodesNet );

    // stop the DSD manager
    Dsd_ManagerStop( pManDsd );

    // delete the global node
    Ntk_NodeDelete( pNodeGlo );


    // copy and add the latches if present
    Ntk_NetworkForEachLatch( pNet, pLatch )
    {
        // get the new latch
        pLatchNew = Ntk_LatchDup( pNetNew, pLatch );
        // set the correct inputs/outputs
        pLatchNew->pInput  = pLatch->pInput->pCopy;
        pLatchNew->pOutput = pLatch->pOutput->pCopy;
        // add the new latch to the network
        Ntk_NetworkAddLatch( pNetNew, pLatchNew );
    }

    // copy the EXDC network
    if ( pNet->pNetExdc )
        pNetNew->pNetExdc = Ntk_NetworkDup( pNet->pNetExdc, pNet->pMan );

    // put IDs into a proper order
//    Ntk_NetworkReassignIds( pNetNew );
    Ntk_NetworkOrderFanins( pNetNew );

    // check the network
    if ( !Ntk_NetworkCheck( pNetNew ) )
       fprintf( Ntk_NetworkReadMvsisOut(pNetNew), "Ntk_NetworkDsdInternal(): Network check has failed.\n" );
    return pNetNew;
}


/**Function*************************************************************

  Synopsis    [Creates the node corresponding to the DSD node.]

  Description [The procedure takes the new network and the array of MVSIS 
  nodes that are already created for other DSD nodes proceding the given
  node in the DFS order.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NetworkDsdNode( Ntk_Network_t * pNet, Ntk_Node_t * pNodeGlo, Ntk_Node_t * ppNodes[], DdManager * ddDsd, Dsd_Node_t * pNodeDsd, bool fUseNand )
{
    Dsd_Node_t * pFaninDsd, * pFaninDsdR;
    Dsd_Type_t Type;
    Ntk_Node_t * pNodeNew, * pFanin, * pFaninInv;
    DdManager * ddMvr;
    DdNode * bVar, * bLocal, * bTemp, * bRel;
    Mvr_Relation_t * pMvr;
    Vm_VarMap_t * pVm;
    Vmx_VarMap_t * pVmx;
    int nDecs, nDecsFanin, Index, i;

    // create the new node
    pNodeNew = Ntk_NodeCreate( pNet, NULL, MV_NODE_INT, 2 );
    // create the fanins
    nDecs = Dsd_NodeReadDecsNum( pNodeDsd );
    assert( nDecs > 1 );
    for ( i = 0; i < nDecs; i++ )
    {
        pFaninDsd = Dsd_NodeReadDec( pNodeDsd, i );
        pFaninDsdR = Dsd_Regular(pFaninDsd);

        // get the corresponding MVSIS node
        nDecsFanin = Dsd_NodeReadDecsNum( pFaninDsdR );
        assert( nDecsFanin > 0 );      
        Index = (Dsd_NodeReadSupp(pFaninDsdR))->index;
        if ( nDecsFanin == 1 )
            pFanin = Ntk_NodeReadFaninNode( pNodeGlo, Index );
        else
            pFanin = ppNodes[ Dsd_NodeReadMark(pFaninDsdR) ];
        assert( pFanin );

        // add buffer if necessary
        if ( (pFaninDsdR != pFaninDsd) ^ (fUseNand && Dsd_NodeReadType(pNodeDsd) == DSD_NODE_OR) )
        {
            if ( pFanin->pCopy == NULL )
            { // the invertor is not avaiable - create it
                pFaninInv = Ntk_NodeCreateOneInputBinary( pNet, pFanin, 1 );
                Ntk_NetworkAddNode( pNet, pFaninInv, 1 );
                pFanin->pCopy = pFaninInv; // remember the intertor
                pFanin = pFaninInv;
            }
            else 
                pFanin = pFanin->pCopy; 
        }
        Ntk_NodeAddFanin( pNodeNew, pFanin );
    }

    // create the variable maps for the new node
    pVm = Vm_VarMapCreateBinary( Ntk_NetworkReadManVm(pNet), nDecs, 1 );
    pVmx = Vmx_VarMapLookup( Ntk_NetworkReadManVmx(pNet), pVm, -1, NULL );

    ddMvr = Ntk_NetworkReadManDdLoc( pNet );
    Type  = Dsd_NodeReadType( pNodeDsd );
    switch ( Type )
    {
        case DSD_NODE_CONST1:
        {
            bLocal = ddMvr->one; Cudd_Ref( bLocal );
            break;
        }
        case DSD_NODE_OR:
        {
            bLocal = Cudd_Not(ddMvr->one); Cudd_Ref( bLocal );
            for ( i = 0; i < nDecs; i++ )
            {
                bVar = Cudd_NotCond( ddMvr->vars[i], fUseNand );
                bLocal = Cudd_bddOr( ddMvr, bTemp = bLocal, bVar );  Cudd_Ref( bLocal );
                Cudd_RecursiveDeref( ddMvr, bTemp );
            }
            break;
        }
        case DSD_NODE_EXOR:
        {
            bLocal = Cudd_Not(ddMvr->one); Cudd_Ref( bLocal );
            for ( i = 0; i < nDecs; i++ )
            {
                bVar = ddMvr->vars[i];
                bLocal = Cudd_bddXor( ddMvr, bTemp = bLocal, bVar );  Cudd_Ref( bLocal );
                Cudd_RecursiveDeref( ddMvr, bTemp );
            }
            break;
        }
        case DSD_NODE_PRIME:
        {
            bLocal = Dsd_TreeGetPrimeFunction( ddDsd, pNodeDsd );    Cudd_Ref( bLocal );
            bLocal = Extra_TransferLevelByLevel( ddDsd, ddMvr, bTemp = bLocal ); Cudd_Ref( bLocal );
            Cudd_RecursiveDeref( ddDsd, bTemp );
            // bLocal is now in the MVR manager
            break;
        }
        default:
        {
            assert( 0 );
            break;
        }
    }

    // create the relation
    bRel = Cudd_bddXnor( ddMvr, bLocal, ddMvr->vars[nDecs] );  Cudd_Ref( bRel );
    Cudd_RecursiveDeref( ddMvr, bLocal );
    // create the relation
    pMvr = Mvr_RelationCreate( Ntk_NetworkReadManMvr(pNet), pVmx, bRel );
    Cudd_RecursiveDeref( ddMvr, bRel );

    // set the var map and the relation
    Ntk_NodeWriteFuncVm( pNodeNew, pVm );
    Ntk_NodeWriteFuncMvr( pNodeNew, pMvr );
    return pNodeNew;
}

/**Function*************************************************************

  Synopsis    [Starts the DSD manager and decomposes global functions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dsd_Manager_t *  Ntk_NetworkDsdPerform( Ntk_Network_t * pNet, bool fVerbose )
{
    DdManager * dd;
    Ntk_Node_t * pNode;
    Dsd_Manager_t * pManDsd;
    DdNode ** pbFuncsGlo;
    int nFuncGlo, i;

    // collect global functions into the array
    nFuncGlo = Ntk_NetworkReadCoNum(pNet);
    pbFuncsGlo = ALLOC( DdNode *, nFuncGlo );
    i = 0;
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        pbFuncsGlo[i++] = (Ntk_NodeReadFuncGlob(pNode))[1];
printf( "Output %3d : Support size = %3d.\n", i,
Cudd_SupportSize( Ntk_NetworkReadDdGlo(pNet), (Ntk_NodeReadFuncGlob(pNode))[1] ) );
    }

    // start the DSD manager
    dd = Ntk_NetworkReadDdGlo( pNet );
    pManDsd = Dsd_ManagerStart( dd, Ntk_NetworkReadCiNum(pNet), fVerbose );
    // decompose global functions
    Dsd_Decompose( pManDsd, pbFuncsGlo, nFuncGlo );
    FREE( pbFuncsGlo );

    return pManDsd;
}


/**Function*************************************************************

  Synopsis    [Procedure to print DSD to the standard output.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkPrintDsd( Ntk_Network_t * pNet, int Output, bool fShort )
{
    bool fDynEnable = 1;
    bool fLatchOnly = 0;
    bool fVerbose = 0;

    // free the old global manager if necessary
    Ntk_NetworkSetDdGlo( pNet, NULL );
    // compute the global functions
    Ntk_NetworkGlobalComputeCo( pNet, fDynEnable, fLatchOnly, fVerbose );
    // create the collapsed network
    Ntk_NetworkPrintDsdInternal( pNet, Output, fShort );
    // undo the global functions
    Ntk_NetworkGlobalDeref( pNet );
}

/**Function*************************************************************

  Synopsis    [Constructs the decomposed network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkPrintDsdInternal( Ntk_Network_t * pNet, int Output, bool fShort )
{
    Dsd_Manager_t * pManDsd;
    Ntk_Node_t * pNode;
    char ** pInputNames, ** pOutputNames;
    int nInputs, nOutputs, i;

    // start the DSD manager and the decomposition
    pManDsd = Ntk_NetworkDsdPerform( pNet, 0 );
    if ( pManDsd == NULL )
        return;

    nInputs = Ntk_NetworkReadCiNum(pNet);
    nOutputs = Ntk_NetworkReadCoNum(pNet);
	pInputNames  = ALLOC( char *, nInputs );
	pOutputNames = ALLOC( char *, nOutputs );

	i = 0;
	Ntk_NetworkForEachCi( pNet, pNode )
		pInputNames[i++] = util_strsav(Ntk_NodeGetNamePrintable(pNode));
	i = 0;
	Ntk_NetworkForEachCo( pNet, pNode )
		pOutputNames[i++] = util_strsav(Ntk_NodeGetNamePrintable(pNode));

    Dsd_TreePrint( Ntk_NetworkReadMvsisOut(pNet), pManDsd, pInputNames, pOutputNames, fShort, Output );

    for ( i = 0; i < nInputs; i++ )
        free( pInputNames[i] );
    for ( i = 0; i < nOutputs; i++ )
        free( pOutputNames[i] );
	free( pOutputNames );
	free( pInputNames );

    // stop the DSD manager
    Dsd_ManagerStop( pManDsd );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



/**CFile****************************************************************

  FileName    [simpCodc.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [MV network simplification using CODC's]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: simpFull.c,v 1.33 2004/01/06 21:02:59 alanmi Exp $]

***********************************************************************/

#include "simpInt.h"
#include <time.h>


/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/


/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static Simp_Node_t *SimpNodeInit(Ntk_Network_t *net,Ntk_Node_t *node);
static void         SimpNodeEnd (Simp_Info_t *pInfo,Ntk_Node_t *node);
static st_table *   SimpNodeSupportCi(Ntk_Network_t *net,Ntk_Node_t *node);
static int          SimpNodeSupportNum(Ntk_Node_t *node);
static int          SimpNodeSupportCompare(char **pn1, char **pn2);
static void         SimpNodeUpdate(Ntk_Node_t *node, Simp_Info_t *pInfo);
static void         SimpNodeFreeGlo ( Ntk_Node_t *pNode, Simp_Info_t *pInfo );
static bool         SimpNodeIsOrphan( Ntk_Node_t *pNode, Simp_Node_t *pSimp );

static void         SimpProcessOrphanRecur(Ntk_Node_t *tnode);
static void         SimpBfsOrderRecur(Simp_Info_t *pInfo, sarray_t *aLevelThis,
                           sarray_t *aLevelAll,st_table *stFoutCount,int iLevels);

static bool         SimpComputeMddGlobal(Ntk_Network_t *n, Simp_Info_t *pInfo);
static bool         SimpSimplifyWithCodc(Ntk_Node_t *n,  Simp_Info_t *pInfo);


/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************
  Synopsis    [Simplify the network using MV-CODC's.]
  Description [Simplify the network using MV-CODC's. Return TRUE if a timeout
  has occured. The CODC computation is programmed to be as modular as
  possible. A separate package can also use the CODC set by simply following
  the following calls:
      pInfo = Simp_InfoInit(network);
      Simp_FullsimpInit(network, pInfo);
      ...compute CODC/SDC/MODC and do you own stuff...
      Simp_FullsimpEnd(network, pInfo);
  ]
  SideEffects []
  SeeAlso     []
******************************************************************************/
bool
Simp_NetworkFullSimplify(
    Ntk_Network_t *pNet,
    Simp_Info_t   *pInfo)
{
    int          i, num_timeouts;
    bool         timeout_occur;
    sarray_t    *listBfs;
    Ntk_Node_t  *pNode;
    ProgressBar * pProgress;
    long         clk, start_time, simplify_time, time_left;
    
    start_time = clock();
    timeout_occur = FALSE;
    
    /* assume sweep has been done */
    
    Simp_FullsimpInit(pNet, pInfo);
    pInfo->time_glob += clock() - start_time;
    
    /* order nodes according to size of their supports */
    listBfs = SimpComputeBfsOrder( pNet, pInfo);
    
    /* start the progress bar */
    if ( pInfo->fProgrs ) {
        pProgress = Extra_ProgressBarStart( stdout, listBfs->num ); 
    }
    
    simplify_time = clock();
    num_timeouts  = 0;
    time_left     = pInfo->timeout_net;
    
    sarrayForEachItem ( Ntk_Node_t *, listBfs, i, pNode ) {
        
        if ( Ntk_NodeIsCo( pNode ) ) continue;
        clk = clock();
        
        if (SimpSimplifyWithCodc( pNode, pInfo )) {
            printf("Node timeout (%d sec) at [%s]\n",
                   (int)(pInfo->timeout_node/CLOCKS_PER_SEC),
                   Ntk_NodeGetNamePrintable(pNode));
            num_timeouts++;
        }
        
        time_left -= clock() - clk;
        if (time_left <= 0) { 
            timeout_occur = 1;
            printf("Total timeout (%d sec) occured at node [%s]\n",
                   (int)(pInfo->timeout_net/CLOCKS_PER_SEC),
                   Ntk_NodeGetNamePrintable(pNode));
            break;
        }
        
        if ( pInfo->fProgrs )
            if ( i % 10 == 0 ) {
                Extra_ProgressBarUpdate( pProgress, i, NULL );
            }
    }
    sarray_free(listBfs);
    if ( pInfo->fProgrs ) {
        Extra_ProgressBarStop( pProgress );
    }
    
    /* print total number of timed out nodes */
    if (!timeout_occur && num_timeouts > 0) {
        printf("Iter %d: timeout at %d node(s)\n",
                pInfo->nIter, num_timeouts);
    }
    
    /* print performance statistics */
    if (pInfo->verbose) {
        printf("\nPerformance summary:\n");
        printf("MINI:\t%s\n", print_time(pInfo->time_mini));
        printf("IMAG:\t%s\n", print_time(pInfo->time_imag));
        printf("CSPF:\t%s\n", print_time(pInfo->time_cspf));
        printf("GLOB:\t%s\n", print_time(pInfo->time_glob));
        printf("TOTL:\t%s\n", print_time(clock()-start_time));
    }
    
    Simp_FullsimpEnd(pNet, pInfo);
    
    return timeout_occur;
}


/**Function********************************************************************
  Synopsis    [Initialize the data structure needed for CODC computation.]
  Description [Return 1 if failed.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
bool
Simp_FullsimpInit(
    Ntk_Network_t *pNet,
    Simp_Info_t   *pInfo) 
{
    int          iSeq;
    DdNode     **pbFuncs;
    Ntk_Node_t  *pNode, *pFanin;
    Simp_Node_t *pSimp;
    
    /* allocate aux_field for mv_simp_type_t */
    Ntk_NetworkForEachCo( pNet, pNode ) {
        Simp_DaemonSetNodeData(pNode, NULL);
    }
    Ntk_NetworkForEachCi( pNet, pNode ) {
        Simp_DaemonSetNodeData(pNode, NULL);
    }
    
    Ntk_NetworkForEachNode( pNet, pNode ) {
        pSimp = SimpNodeInit(pNet, pNode);
        Simp_DaemonSetNodeData(pNode, pSimp);
    }
    
    /* initiate a new Cudd manager for global MDD's */
    pInfo->ddmg = Cudd_Init( 0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    Cudd_AutodynEnable( pInfo->ddmg, CUDD_REORDER_SYMM_SIFT );
    
    /* allocate space for MVF; compute local and global MDD */
    if (SimpComputeMddGlobal(pNet, pInfo)) {
        return 1;
    }
    Cudd_AutodynDisable( pInfo->ddmg );
    
    /* store information for complete flexibility computation */
    if ( pInfo->dc_type == 3 ) {
        int nValues;
        
        pInfo->ppbCoGlo = ALLOC( Mva_Func_t *, Ntk_NetworkReadCoNum(pNet) );
        iSeq = 0;
        Ntk_NetworkForEachCo( pNet, pNode ) {
            
            pFanin  = Ntk_NodeReadFaninNode( pNode, 0 );
            pbFuncs = Ntk_NodeReadFuncGlo( pFanin );
            nValues = Ntk_NodeReadValueNum(pFanin);
            pInfo->ppbCoGlo[iSeq++] = Mva_FuncCreate( pInfo->ddmg,
                                         nValues, pbFuncs );
        }
    }
    return 0;
}


/**Function********************************************************************
  Synopsis    [Clean up the data structure needed for CODC computation.]
  Description [Assume that Simp_FullsimpInit() has been called. Return 1 if failed.]
  SideEffects []
  SeeAlso     [Simp_FullsimpInit]
******************************************************************************/
bool
Simp_FullsimpEnd(
    Ntk_Network_t *network,
    Simp_Info_t   *pInfo) 
{
    int         nValues, i;
    Ntk_Node_t *pNode;
    DdNode    **pbFuncs;
    
    Ntk_NetworkForEachNode( network, pNode ) {
        SimpNodeEnd( pInfo, pNode );
        
        pbFuncs = Ntk_NodeReadFuncGlo( pNode );
        nValues = Ntk_NodeReadValueNum( pNode );
        if ( pbFuncs ) {
            Ntk_NodeWriteFuncGlo( pNode, NULL );
            for ( i = 0; i < nValues; i++ )
                Cudd_RecursiveDeref( pInfo->ddmg, pbFuncs[i] );
            FREE( pbFuncs );
        }
    }
    Ntk_NetworkForEachCi( network, pNode ) {
        pbFuncs = Ntk_NodeReadFuncGlo( pNode );
        nValues = Ntk_NodeReadValueNum( pNode );
        if ( pbFuncs ) {
            Ntk_NodeWriteFuncGlo( pNode, NULL );
            for ( i = 0; i < nValues; i++ )
                Cudd_RecursiveDeref( pInfo->ddmg, pbFuncs[i] );
            FREE( pbFuncs );
        }
    }
    
    if ( pInfo->ppNodes ) {
        FREE( pInfo->ppNodes );
    }
    if ( pInfo->ppbCoGlo ) {
        int nCo = Ntk_NetworkReadCoNum( pInfo->network );
        for ( i=0; i<nCo; ++i ) {
            if ( pInfo->ppbCoGlo[i] != NULL )
                Mva_FuncFree( pInfo->ppbCoGlo[i] );
        }
        FREE( pInfo->ppbCoGlo );
    }
    
    Extra_StopManager( pInfo->ddmg );
    //Cudd_Quit( pInfo->ddmg );
    return 0;
}


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************
  Synopsis    [Compute a reverse topological order]
  Description [Compute a reverse topological order of all nodes in the
  network. A node always appear BEFORE its fanin nodes. Given the same
  topological level, the nodes with large PI support set is ordered first. A
  new array is returned, which should be freed by the user. (could reimplement
  more efficiently with Ntk_NetworkLevelize() TODO).]
  SideEffects []
  SeeAlso     []
******************************************************************************/
sarray_t *
SimpComputeBfsOrder(
    Ntk_Network_t *network,
    Simp_Info_t   *pInfo)
{
    int i,j, nCo, iSeq;
    Ntk_Node_t *pNode;
    sarray_t    *aLevelThis, *aLevelAll, *aTemp1, *aListFinal;
    st_table   *stFoutCount;
    
    iSeq = 0;
    nCo  = Ntk_NetworkReadCoNum( network );
    stFoutCount  = st_init_table(st_ptrcmp, st_ptrhash);
    aLevelThis   = sarray_alloc( Ntk_Node_t *, nCo );
    aLevelAll    = sarray_alloc( Ntk_Node_t *, nCo );
    aListFinal   = sarray_alloc( Ntk_Node_t *, Ntk_NetworkReadNodeTotalNum( network ) );
    
    /* compute combinational outputs */
    Ntk_NetworkForEachCo( network, pNode ) {
        sarray_insert( Ntk_Node_t *, aLevelThis, iSeq++, pNode );
    }
    aLevelThis->num = nCo;
    
    /* recursively compute the BFS order */
    SimpBfsOrderRecur(pInfo, aLevelThis, aLevelAll, stFoutCount, 0);
    st_free_table(stFoutCount);
    
    /* process aLevelAll */
    for (i=0; i < aLevelAll->num; ++i) {
        
        aTemp1 = sarray_fetch(sarray_t *, aLevelAll, i);
        if (pInfo->verbose)
            printf("BFS [%d]:", i);
        
        /* last set consists of all PIs (hassle free) */
        if ( i == aLevelAll->num - 1 ) {
            sarrayForEachItem(Ntk_Node_t *, aTemp1, j, pNode) {
                if (pNode) {
                    sarray_insert_last( Ntk_Node_t *, aListFinal, pNode );
                    if (pInfo->verbose)
                        printf(" %s", Ntk_NodeGetNamePrintable(pNode));
                }
            }
            sarray_free(aTemp1);
            if (pInfo->verbose)
                printf("\n");
            continue;
        }
        
        /* insert into the final list */
        if ( aTemp1->num > 1 ) {
            sarray_sort( aTemp1, SimpNodeSupportCompare );
        }
        
        for ( j = 0; j < aTemp1->num; ++j) {
            pNode = sarray_fetch(Ntk_Node_t *, aTemp1, j);
            if (pNode) {
                sarray_insert_last( Ntk_Node_t *, aListFinal, pNode );
                if (pInfo->verbose)
                    printf(" %s", Ntk_NodeGetNamePrintable(pNode));
            }
        }
        sarray_free(aTemp1);
        if (pInfo->verbose) printf("\n");
    }
    sarray_free(aLevelAll);
    
    return aListFinal;
}



/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************
  Synopsis    [Compute the CODC set and simplify the node.]
  Description [Return true if a timeout has occured.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
bool
SimpSimplifyWithCodc(
    Ntk_Node_t  *pNode,
    Simp_Info_t *pInfo)
{
    bool         timeout_occured, fChanged;
    DdNode      *pCodc;
    Ntk_Node_t  *pNodeDc;
    Simp_Node_t *simp_data;
    
    /* some sanity checks */
    simp_data = Simp_DaemonGetNodeData(pNode);
    if ( simp_data == NULL) return 0;
    
    /* skip constant and CI nodes */
    if (Ntk_NodeIsCi(pNode) ||
        Ntk_NodeReadFaninNum(pNode)<=1 ) {
        simp_data->pCodc = Cudd_ReadLogicZero(pInfo->ddmg);
        Cudd_Ref(simp_data->pCodc);
        return 0;
    }
    if (pInfo->verbose) {
        printf("\nFull-simplify start node [%s]\n",
               Ntk_NodeGetNamePrintable(pNode));
    }
    
    /* special treatment for floating nodes */
    if ( SimpNodeIsOrphan( pNode, simp_data ) ){
        
        simp_data->fBadnode = TRUE;
        if ( simp_data->pCodc ) {
            Cudd_RecursiveDeref( pInfo->ddmg, simp_data->pCodc );
            simp_data->pCodc = NULL;
        }
        
        SimpProcessOrphanRecur(pNode);
        if (pInfo->verbose) {
            printf("warning: node %s has no fanout\n",
                   Ntk_NodeGetNamePrintable(pNode));
        }
        return 0;
    }
    
    /* start recording time budget */
    timeout_occured = 0;
    fChanged = FALSE;
    pNodeDc  = NULL;
    pCodc    = NULL;
    SimpTimeOutStartNode(pInfo, pInfo->timeout_node);
    
    /* compute CODC in MDDs */
    if ( pInfo->dc_type == 1 ) {      /* SDC */
        pCodc = NULL;
    }
    else if ( pInfo->dc_type == 3 ) { /* Complete */
        pCodc = Simp_ComputeFlex( pInfo, pNode );
    }
    else {                            /* MODC/CODC */
        pCodc = Simp_ComputeCodc(pInfo, pNode);
    }
    pInfo->time_cspf += clock() - pInfo->time_start_node;
    timeout_occured = SimpTimeOutCheckNode(pInfo);
    
    
    /* compute image in the local support */
    if (!timeout_occured) {
        pNodeDc = Simp_ComputeImage(pInfo, pNode, pCodc);
    }
    pInfo->time_imag += clock() - pInfo->time_start_node;
    timeout_occured = SimpTimeOutCheckNode(pInfo);
    
    /* dispose of the CODC */
    if (pCodc) Cudd_RecursiveDeref(pInfo->ddmg, pCodc);
    
    if ( pInfo->verbose ) {
        Vm_VarMap_t *pVm;
        Cvr_Cover_t *pCf, *pCd;
        pVm = Ntk_NodeReadFuncVm( pNode );
        pCf = Ntk_NodeGetFuncCvr( pNode );
        pCd = (pNodeDc)?Ntk_NodeReadFuncCvr( pNodeDc ):NULL;
        
        printf("Simplifying: nValsIn[%3d] nValsOut[%3d] nCubesOn[%3d] nCubesDc[%3d]\n",
               Vm_VarMapReadValuesInNum( pVm ),
               Vm_VarMapReadValuesOutNum( pVm ),
               Cvr_CoverReadCubeNum( pCf ),
               (pCd) ? Cvr_CoverReadCubeNum( pCd ) : 0 );
    }
    
    /* minimize the node */
    fChanged = Simp_NodeSimplifyDc(pNode, pNodeDc,
                                   pInfo->method,  pInfo->accept,
                                   pInfo->fSparse, pInfo->fConser,
                                   pInfo->fPhase,  pInfo->fRelatn);
    
    if ( pInfo->verbose && fChanged ) {
        printf( "Simplified!\n" );
    }
    
    pInfo->time_mini += clock() - pInfo->time_start_node;
    timeout_occured = SimpTimeOutCheckNode(pInfo);
    
    /* dispose of the local CODC node */
    if ( pNodeDc ) {
        Ntk_NodeDelete( pNodeDc );
    }
    
    /* rebuild the global BDD only when the old node is replaced;
       also, CO nodes always have the same (or less) global function
    */
    if (fChanged) {
        SimpNodeUpdate( pNode, pInfo );
        pInfo->time_glob += clock() - pInfo->time_start_node;
        timeout_occured = SimpTimeOutCheckNode(pInfo);
    }
    
    assert( Ntk_NetworkCheckNode(pInfo->network, pNode) );
    
    /* compute MODC for fanin edges (codc|modc) */
    if ( !timeout_occured &&
         (pInfo->dc_type == 0 || pInfo->dc_type == 2) ) {
        SimpComputeMspf(pNode, pInfo);
    }
    
    return timeout_occured;
}

/**Function********************************************************************
  Synopsis    [Compute the MDD array (local and global) for all nodes.]
  Description [Should allow giving-up gracefully if the MDD size explode.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
bool
SimpComputeMddGlobal(
    Ntk_Network_t * pNet,
    Simp_Info_t   * pInfo) 
{
    int            i, k, nBits, nLeaves, nValues, iStick, iIndex;
    int          * pValuesFirst;
    int          * pBits, *pBitsFirst, *pBitsOrder;
    char        ** psLeavesByName;
    Ntk_Node_t   * pNode, * pNodeCi;
    Vmx_VarMap_t * pVmx;
    DdNode      ** pbCodes, ** pbFuncs;
    DdManager    * ddmg;

    ddmg = pInfo->ddmg;
    
    // get the CI variable order
    psLeavesByName = Ntk_NetworkOrderArrayByName( pNet, 0 );
    // get the variable map
    pVmx = Ntk_NetworkGlobalGetVmx( pNet, psLeavesByName );
    pInfo->pVmx = pVmx;
    
    // extend the global manager if necessary
    nBits = Vmx_VarMapReadBitsNum( pVmx );
    for ( i = ddmg->size; i < nBits; i++ )
        Cudd_bddIthVar( ddmg, i );
    
    // encode the CI variables
    pbCodes = Vmx_VarMapEncodeMap( ddmg, pVmx );
    // get the pointer to the first values
    pValuesFirst = Vm_VarMapReadValuesFirstArray( Vmx_VarMapReadVm(pVmx) );
    // assign the elementary BDDs to the values of the CIs
    nLeaves = Ntk_NetworkReadCiNum( pNet );
    
    /* store relationship between BDD ID and node */
    pInfo->ppNodes = ALLOC( Ntk_Node_t *, Cudd_ReadSize( ddmg ) );
    memset( pInfo->ppNodes, 0, Cudd_ReadSize(ddmg) * sizeof(Ntk_Node_t *));
    pBits      = Vmx_VarMapReadBits(pVmx);
    pBitsFirst = Vmx_VarMapReadBitsFirst(pVmx);
    pBitsOrder = Vmx_VarMapReadBitsOrder(pVmx);
    
    for ( i = 0; i < nLeaves; i++ )
    {
        pNodeCi = Ntk_NetworkFindNodeByName( pNet, psLeavesByName[i] );
        assert( pNodeCi );
        // copy the elementary MDDs into the CIs
        nValues = Ntk_NodeReadValueNum( pNodeCi );
        pbFuncs = ALLOC( DdNode *, nValues );
        k = 0;
        do {
            pbFuncs[k] = (pbCodes + pValuesFirst[i])[k];
            Cudd_Ref( pbFuncs[k++] );
        }
        while ( k < nValues );
        Ntk_NodeWriteFuncGlo( pNodeCi, pbFuncs );
        
        /* store the BDD ID info */
        iStick = 0;
        for (k=0; k<pBits[i]; ++k) {
            
            iIndex = pBitsOrder[pBitsFirst[i] + k];
            if ( iStick != iIndex ) {
                pInfo->ppNodes[iIndex] = pNodeCi;
                iStick = iIndex;
            }
        }
    }
    // deref the codes 
    Vmx_VarMapEncodeDeref( ddmg, pVmx, pbCodes );
    
    Ntk_NetworkDfs(pNet, 1);
    
    Ntk_NetworkForEachNodeSpecial(pNet, pNode) {
        
        /* compute global function only if neccessary */
        if ( Ntk_NodeIsCi(pNode) || Ntk_NodeIsCo(pNode) )
            /* SimpNodeIsSubset(pNode)*/
            continue;
        pbFuncs = Ntk_NodeGlobalMdd( ddmg, pNode);
        Ntk_NodeWriteFuncGlo(pNode, pbFuncs);
    }
    
    FREE( psLeavesByName );
    return 0;
}


/**Function********************************************************************
  Synopsis    [Initialize the simplify related data structure for a node.]
  Description [Initialize the simplify related data structure for a node. A
  new structure is returned, which should be freed by the user. ]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Simp_Node_t *
SimpNodeInit(
    Ntk_Network_t *pNet,
    Ntk_Node_t    *pNode) 
{
    Simp_Node_t *pSimp;
    
    pSimp = ALLOC(Simp_Node_t, 1);
    memset(pSimp, 0, sizeof(Simp_Node_t));
    
    pSimp->stCi  = SimpNodeSupportCi(pNet, pNode);
    return pSimp;
}

/**Function********************************************************************
  Synopsis    [Free the simplify related data structure for a node.]
  Description [Free the simplify related data structure for a node.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
SimpNodeEnd(
    Simp_Info_t *pInfo,
    Ntk_Node_t  *pNode) 
{
    int i, nFanins;
    Simp_Node_t *pSimp;
    DdManager   *dd;
    
    pSimp = Simp_DaemonGetNodeData(pNode);
    
    if (pSimp->stCi != NULL) {
        st_free_table(pSimp->stCi);
    }
    if (pSimp->pCodc != NULL) {
        Cudd_RecursiveDeref(pInfo->ddmg, pSimp->pCodc);
    }
    /* Mspf belongs to the local manager! */
    nFanins = Ntk_NodeReadFaninNum(pNode);
    dd = Ntk_NetworkReadManDdLoc ( pInfo->network );
    if ((nFanins > 0) && (!Ntk_NodeIsCi(pNode))) {
        if (pSimp->lMspf != NULL) {
            for (i=0; i<nFanins; ++i) {
                if (pSimp->lMspf[i]) {
                    Cudd_RecursiveDeref(dd, pSimp->lMspf[i]);
                }
            }
            FREE(pSimp->lMspf);
        }
    }
    FREE(pSimp);
    Simp_DaemonSetNodeData(pNode, NULL);
    
    /* release global BDD */
    SimpNodeFreeGlo( pNode, pInfo );
    
    return;
}

/**Function********************************************************************
  Synopsis    [Free global BDD]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
SimpNodeFreeGlo ( Ntk_Node_t *pNode, Simp_Info_t *pInfo )
{
    int i,nValues;
    DdNode **pbFuncs;
    pbFuncs = Ntk_NodeReadFuncGlo( pNode );
    nValues = Ntk_NodeReadValueNum( pNode );
    if ( pbFuncs ) {
        
        Ntk_NodeWriteFuncGlo( pNode, NULL );
        for ( i = 0; i < nValues; i++ )
            Cudd_RecursiveDeref( pInfo->ddmg, pbFuncs[i] );
        FREE( pbFuncs );
    }
}


/**Function********************************************************************
  Synopsis    [Recursively compute a reverse topological order.]
  Description [Given the same topological level, the nodes with large PI
  support set is ordered first. A new array is returned, which should be freed
  by the user.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
SimpBfsOrderRecur(
    Simp_Info_t *pInfo,
    sarray_t *   aLevelThis,
    sarray_t *   aLevelAll,
    st_table *   stFoutCount,
    int          iLevels) 
{
    int i,j,num;
    sarray_t     *aLevelNext;
    Ntk_Node_t  *pTmp1, *pTmp2;
    Ntk_Pin_t   *pPin;
    Simp_Node_t *pSimp;
    char        *dummy;
    
    aLevelNext = sarray_alloc( Ntk_Node_t *, aLevelThis->size );
    sarray_insert_last_safe( Ntk_Node_t *, aLevelAll, aLevelThis );
    
    for (i=0; i < aLevelThis->num; i++) {
        
        pTmp1 = sarray_fetch(Ntk_Node_t *, aLevelThis, i);
        if (Ntk_NodeIsCi(pTmp1)) continue;
        
        pSimp = Simp_DaemonGetNodeData(pTmp1);
        
        Ntk_NodeForEachFaninWithIndex( pTmp1, pPin, pTmp2, j ) {
            if (Ntk_NodeReadFanoutNum(pTmp2) == 1) {
                sarray_insert_last_safe( Ntk_Node_t *, aLevelNext, pTmp2 );
                continue;
            }
            
            if (!st_lookup(stFoutCount, (char *) pTmp2, &dummy)) {
                /*printf("{%3s}\tfanout[%5s(%2d/%2d)]\tnout[%2d]\tcount[ 1]\n",
                       Ntk_NodeGetNamePrintable(pTmp2),
                       Ntk_NodeGetNamePrintable(pTmp1),j,
                       Ntk_NodeReadFaninNum(pTmp1),
                       Ntk_NodeReadFanoutNum(pTmp2)); */
                (void) st_insert(stFoutCount, (char*)pTmp2, (char*) 1);
            }
            else {
                num = (int)dummy; num++;
                /* printf("{%3s}\tfanout[%5s(%2d/%2d)]\tnout[%2d]\tcount[%2d]\n",
                       Ntk_NodeGetNamePrintable(pTmp2),
                       Ntk_NodeGetNamePrintable(pTmp1),j,
                       Ntk_NodeReadFaninNum(pTmp1),
                       Ntk_NodeReadFanoutNum(pTmp2),num); */
                if (num != Ntk_NodeReadFanoutNum(pTmp2)) {
                    (void) st_delete(stFoutCount, (char**)&pTmp2, &dummy);
                    (void) st_insert(stFoutCount, (char*)pTmp2, (char*) num);
                    /* printf("\n"); */
                }
                else {
                    /* all fanout nodes have been visited */
                    (void) st_delete(stFoutCount, (char**)&pTmp2, &dummy);
                    sarray_insert_last_safe( Ntk_Node_t *, aLevelNext, pTmp2 );
                    /* printf("\t{%s} next level\n",
                       Ntk_NodeGetNamePrintable(pTmp2)); */
                }
            }
        }
    }
    
    iLevels++;
    if ( aLevelNext->num > 0) {
        SimpBfsOrderRecur( pInfo, aLevelNext, aLevelAll, stFoutCount, iLevels );
    } else {
        sarray_free( aLevelNext );
    }
    
    return;
}

/**Function********************************************************************
  Synopsis    [Propagate the badness down until no bad node is discovered.]
  Description [Assume the node given is a badnode.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
SimpProcessOrphanRecur(
    Ntk_Node_t *tnode) 
{
    int flag;
    Ntk_Node_t  *fanin, *fanout;
    Ntk_Pin_t   *pPin1, *pPin2;
    Simp_Node_t *pSimp, *pSimp2;
    
    if (Ntk_NodeIsCi(tnode)) return;
    
    pSimp = Simp_DaemonGetNodeData(tnode);
    if (!pSimp->fBadnode) return;
    
    /* recursively propagate orphaness */
    Ntk_NodeForEachFanin(tnode, pPin1, fanin) {
        if ( Ntk_NodeIsCi( fanin )) continue;
        
        /* if all parents are orphans, it becomes an orphan */
        flag = -1;
        Ntk_NodeForEachFanout( fanin, pPin2, fanout) {
            
            /* if fanout to CO, for sure not an orphan */
            if ( Ntk_NodeIsCo( fanout ) ) {
                flag=1; break;
            }
            
            /* otherwise check orphanness of fanouts */
            pSimp2 = Simp_DaemonGetNodeData(fanout);
            if ( !(pSimp2->fBadnode) ) {
                flag=1; break;
            }
        }
        if (flag<0) {
            pSimp2 = Simp_DaemonGetNodeData(fanin);
            pSimp2->fBadnode = TRUE;
            SimpProcessOrphanRecur(fanin);
        }
    }
    return;
}

/**Function********************************************************************
  Synopsis    [return 1 if a node is orphan]
  Description [a node is orphan if it has not fanout or all fanouts are orphans]
  SideEffects []
  SeeAlso     []
******************************************************************************/
bool
SimpNodeIsOrphan( Ntk_Node_t *pNode, Simp_Node_t *pSimp ) 
{
    Ntk_Node_t  *pFanout;
    Ntk_Pin_t   *pPin;
    Simp_Node_t *pSimpFanout;
    
    if ( pSimp->fBadnode || Ntk_NodeReadFanoutNum(pNode) < 1 )
        return TRUE;
    
    Ntk_NodeForEachFanout( pNode, pPin, pFanout ) {
        if ( Ntk_NodeIsCo( pFanout ) )
            return FALSE;
        pSimpFanout = Simp_DaemonGetNodeData( pFanout );
        if ( !pSimpFanout->fBadnode )
            return FALSE;
    }
    return TRUE;
}


/**Function********************************************************************
  Synopsis    [Compare the support size of two nodes.]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
SimpNodeSupportCompare(
    char **pn1,
    char **pn2) 
{
    if (pn1==NULL) {
        if (pn2==NULL) return 0;
        else return 1;
    }
    if (pn2==NULL) return -1;
    if ( SimpNodeSupportNum((Ntk_Node_t *)(*pn1)) <
         SimpNodeSupportNum((Ntk_Node_t *)(*pn2))) return 1;
    else if ( SimpNodeSupportNum((Ntk_Node_t*)(*pn1)) >
              SimpNodeSupportNum((Ntk_Node_t*)(*pn2))) return -1;
    else return 0;
}


/**Function********************************************************************
  Synopsis    [Update the MDD with the new logic function]
  Description [This is called after minimization is performed on the
  node. If complete flexibility is used in the minimization, one would need to
  update all the nodes in the transitive fanout cone to reflect the change,
  because their ODC set is no longer valid.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
SimpNodeUpdate(
    Ntk_Node_t *pNode,
    Simp_Info_t *pInfo)
{
    DdNode ** pbFuncs;
    
    /* rebuild the global BDD only when the old node is replaced;
       also, CO nodes always have the same (or less) global function
    */
    if ( !Ntk_NodeIsCoFanin( pNode ) ) {
        
        SimpNodeFreeGlo( pNode, pInfo );
        pbFuncs = Ntk_NodeGlobalMdd( pInfo->ddmg, pNode );
        Ntk_NodeWriteFuncGlo( pNode, pbFuncs );
    }
    return;
}

/**Function********************************************************************
  Synopsis    [Compute the support in terms of combinational input nodes]
  Description [return NULL if the support is null]
  SideEffects []
  SeeAlso     []
******************************************************************************/
st_table *
SimpNodeSupportCi(
    Ntk_Network_t *pNet,
    Ntk_Node_t    *pNode) 
{
    int        nSup;
    st_table   *stCi;
    Ntk_Node_t *tnode;
    
    if (pNode==NULL) return NULL;
    if (Ntk_NodeReadNetwork(pNode)==NULL) return NULL;
    
    nSup = Ntk_NetworkComputeNodeSupport(pNet, &pNode, 1);
    if (nSup < 1) {
        return NULL;
    }
    
    stCi = st_init_table(st_ptrcmp, st_ptrhash);
    Ntk_NetworkForEachNodeSpecial(pNet, tnode) {
        st_insert(stCi, (char *)tnode, NULL);
    }
    return stCi;
}


/**Function********************************************************************
  Synopsis    [Return the number of PI support nodes.]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
SimpNodeSupportNum(
    Ntk_Node_t *node) 
{
    Simp_Node_t *pSimp;
    if ( Ntk_NodeIsCi( node ) ) return 0;
    pSimp = Simp_DaemonGetNodeData(node);
    if (pSimp && pSimp->stCi) 
        return st_count(pSimp->stCi);
    return 0;
}

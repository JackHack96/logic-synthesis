/**CFile****************************************************************

  FileName    [simpDc.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Compute don't cares for a node.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: simpSdc.c,v 1.15 2003/05/27 23:16:17 alanmi Exp $]

***********************************************************************/

#include "simpInt.h"
#include "ntkInt.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static int         SimpNodeArrayFindIndex (sarray_t *nlist, Ntk_Node_t *keynode);
static bool        SimpTestSubsetSupport  (Ntk_Node_t *node, st_table *support);
static Mvc_Cover_t *SimpComputeSdcInternal(Ntk_Node_t *node, Vm_VarMap_t *pVm,
                                           sarray_t *lFanin, sarray_t *lResub);
static sarray_t    *SimpComputeSubsetSupportIntern(Ntk_Node_t *pNode);
static sarray_t    *SimpComputeSubsetSupportExtern(Ntk_Node_t *pNode);
static void        Mvc_CoverBitSet(Mvc_Cover_t *pMvc, Vm_VarMap_t *pVm, int iVar,
                                   int iValue);

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Compute the SDC set from the local fanins.]

  Description [Compute the SDC set from the local fanins. The don't care node
  returned will depend on exactly the same set fanins of the original node. No
  subset support node will be include. There is no need to set up the
  full-simp data structure and initialize the MDDs for each node. It can be
  directly called by the simplify command.]

  SideEffects []

  SeeAlso     [SimpComputeSdcNewBase SimpComputeSdcInternal]

******************************************************************************/
Ntk_Node_t *
Simp_ComputeSdcLocal(
    Ntk_Node_t *pNode) 
{
    Ntk_Node_t  *pNodeSdc;
    Mvc_Cover_t *pMvcSdc;
    Vm_VarMap_t *pVm;
    sarray_t    *lResub, *lFanin;
    
    lResub = SimpComputeSubsetSupportIntern(pNode);
    
    /* if subset node exists then call internal sdc function */
    if ( lResub->num > 0) {
        
        lFanin = sarray_alloc( Ntk_Node_t *, Ntk_NodeReadFaninNum(pNode) );
        Ntk_NodeReadFanins(pNode, (Ntk_Node_t **)(lFanin->space));
        lFanin->num = Ntk_NodeReadFaninNum(pNode);
        pVm = Ntk_NodeReadFuncVm(pNode);
        
        pMvcSdc  = SimpComputeSdcInternal(pNode, pVm, lFanin, lResub);
        pNodeSdc = SimpNodeCreateFromFanins(Ntk_NodeReadNetwork(pNode),lFanin);
        SimpNodeAssignMvc(pNodeSdc, pMvcSdc);
        sarray_free(lFanin);
    }
    else {
        pNodeSdc = NULL;
    }
    
    sarray_free(lResub);
    
    return pNodeSdc;
}


/**Function********************************************************************

  Synopsis    [Compute the satisfiability don't care (SDC) set with boolean
  resub of subset support nodes.]
  
  Description [Compute the satisfiability don't care (SDC) set with boolean
  resub of subset support nodes. A new node is returned, which contains the
  don't care and the fanins it dependes on. There is no need to set up the
  full-simp data structure and initialize the MDDs for each node. It can be
  directly called by the simplify command.]

  SideEffects []

  SeeAlso     [SimpComputeSdcLocal SimpComputeSdcInternal]

******************************************************************************/
Ntk_Node_t *
Simp_ComputeSdcNewBase(
    Ntk_Node_t  *pNode) 
{
    sarray_t    *lResub, *lSubset, *lFanin, *lNew;
    Ntk_Node_t  *pNodeSdc;
    
    Mvc_Cover_t *pMvcSdc;
    Vm_VarMap_t *pVmNew;
    
    lResub = SimpComputeSubsetSupportExtern(pNode);
    
    /* no subset support nodes, call sdc-local */
    if ( lResub->num==0 ) {
        sarray_free( lResub );
        return Simp_ComputeSdcLocal( pNode );
    }
    
    lFanin = sarray_alloc( Ntk_Node_t *, Ntk_NodeReadFaninNum(pNode)+lResub->num );
    Ntk_NodeReadFanins(pNode, (Ntk_Node_t **)(lFanin->space));
    lFanin->num = Ntk_NodeReadFaninNum(pNode);
    (void)sarray_append(lFanin, lResub);
    
    pNodeSdc = SimpNodeCreateFromFanins(Ntk_NodeReadNetwork(pNode),lFanin);
    pVmNew = Ntk_NodeGetFuncVm(pNodeSdc);
    
    /* find the internal subset support nodes */
    lSubset = SimpComputeSubsetSupportIntern(pNode);
    lNew    = sarray_join(lSubset, lResub);
    
    pMvcSdc  = SimpComputeSdcInternal(pNode, pVmNew, lFanin, lNew);
    SimpNodeAssignMvc(pNodeSdc, pMvcSdc);
    
    sarray_free(lFanin);
    sarray_free(lResub);
    sarray_free(lSubset);
    sarray_free(lNew);
    return pNodeSdc;
}

/**Function********************************************************************

  Synopsis    [Compute the SDC set with given subset support nodes]
  
  Description [Compute the SDC set with given subset support nodes. Array
  fanin_list should contain existing inputs and possibly additional new nodes
  as boolean resub candidates; resub_list should contain all the subset
  support nodes, which may or may not be the current fanin nodes. 
      fanin_list : [pnode fanins][resub nodes]
      resub_list :          [nodes]
  ]

  SideEffects []
  SeeAlso     [SimpComputeSdcInternal]

******************************************************************************/
Ntk_Node_t *
Simp_ComputeSdcResub (Ntk_Node_t *pNode, sarray_t *lFanin, sarray_t *lResub) 
{
    Ntk_Node_t  *pNodeSdc;
    Vm_VarMap_t *pVm;
    Mvc_Cover_t *pMvcSdc;
    
    if ( lResub==NULL || sarray_n(lResub)==0 ||
         lFanin==NULL || sarray_n(lFanin)==0 )
        return NULL;
    
    pNodeSdc = SimpNodeCreateFromFanins(Ntk_NodeReadNetwork(pNode),lFanin);
    pVm = Ntk_NodeGetFuncVm(pNodeSdc);
    
    pMvcSdc  = SimpComputeSdcInternal(pNode, pVm, lFanin, lResub);
    SimpNodeAssignMvc(pNodeSdc, pMvcSdc);
    
    return pNodeSdc;
}


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis    [Return the list of subset support nodes.]
  
  Description [Return the list of subset support nodes. A node g is a subset
  support node of f, iff the fanins of g is a subset of the fanins of f.
  Returns a new array structure, which may be empty. Note that this routine
  can be called by command simplify, which does not allocate simp_data for
  each node.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
sarray_t *
SimpComputeSubsetSupportExtern(
    Ntk_Node_t *pNode)
{
    char  *dummy;
    bool          is_subset_sup;
    int            iTrav;
    Ntk_Pin_t     *pPin, *pPin2, *pPin3;
    Ntk_Node_t    *pFanin, *pFaninout, *pTmp;
    Ntk_Network_t *pNet;
    Simp_Node_t   *pSimp;
    st_table      *stFanin;
    sarray_t      *listResub;
    
    pNet = Ntk_NodeReadNetwork(pNode);
    
    /* avoid nodes in the transitive fanout cone */
    Ntk_NetworkComputeNodeTfo( pNet, &pNode, 1, 500, 0, 1 );
    iTrav = Ntk_NetworkReadTravId( pNet );
    
    /* initialize hash table for fanin nodes */
    stFanin = st_init_table(st_ptrcmp, st_ptrhash);
    Ntk_NodeForEachFanin( pNode, pPin, pFanin ) {
        st_insert(stFanin, (char *)pFanin, (char *)1);
        Ntk_NodeSetTravId( pFanin, iTrav );
    }
    
    /* check all fanouts of all fanins */
    listResub = sarray_alloc( Ntk_Node_t *, Ntk_NodeReadFaninNum(pNode) );
    
    Ntk_NodeForEachFanin(pNode, pPin, pFanin) {
        Ntk_NodeForEachFanout(pFanin, pPin2, pFaninout) {
            
            if ( pFaninout == pNode || Ntk_NodeIsCo(pFaninout))
                continue;
            if ( Ntk_NodeReadTravId( pFaninout) == iTrav ) /* Tfo traversal I.D. */
                continue;
            
            /* skip orphan nodes if we are inside fullsimp */
            pSimp = Simp_DaemonGetNodeData( pFaninout );
            if ( pSimp && pSimp->fBadnode ) {
                continue;
            }
            
            /* set traversal i.d. to the current one; avoid hash */
            Ntk_NodeSetTravId( pFaninout, iTrav );
            
            is_subset_sup = TRUE;
            Ntk_NodeForEachFanin( pFaninout, pPin3, pTmp ) {
                if(!st_lookup(stFanin, (char *)pTmp, &dummy)) {
                    is_subset_sup = FALSE;
                    break;
                }
            }
            if (is_subset_sup && Ntk_NodeReadFaninNum( pFaninout ) > 1) {
                sarray_insert_last_safe( Ntk_Node_t *, listResub, pFaninout );
            }
        }
    }
    
    st_free_table(stFanin);
    
    return listResub;
}


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Compute the SDC set with given subset support nodes]
  
  Description [Compute the SDC set with given subset support nodes. Array
  fanin_list should contain existing inputs and possibly additional new nodes
  as boolean resub candidates; resub_list should contain all the subset
  support nodes, which may or may not be the current fanin nodes. 

   fanin_list : [pnode fanins][resub nodes]
   resub_list :          [nodes]

  Note:  explicit SDC computation should avoid non-deterministic part
         (used to) onset SDC = [onset(0)][~0] + [onset(1)][~1]
         (should)  onset SDC = [compl(0)][0]  + [compl(1)][1]
   ]

  SideEffects []
   
  SeeAlso     []

******************************************************************************/
Mvc_Cover_t *
SimpComputeSdcInternal(
    Ntk_Node_t  *node,
    Vm_VarMap_t *pVm,         /* var map of the new fanin list */
    sarray_t    *fanin_list,  /* new fanin list, may not be same as node */
    sarray_t    *resub_list) 
{
    Mvc_Cover_t *pDomain, *pDef, *pSdcAll, *pSdc, *pTmp;
    Mvc_Cover_t *pImage, *isetj, *isetj_adj, *isetj_new, **pIsets;
    int         dvalue, iResubVar, nValues;
    int         i,j,index;
    int         *pnPos;
    Ntk_Node_t  *pFanin, *pFinout;
    Ntk_Pin_t   *pPin;
    Cvr_Cover_t *pCvr;
    Vm_VarMap_t *pVmTmp;
    
    dvalue = -1;
    iResubVar = -1;
    
    pSdcAll = NULL;
    pnPos = ALLOC(int, fanin_list->num);
    memset(pnPos, 0, sizeof(int)*fanin_list->num);
    
    for (i=0; i<sarray_n(resub_list); i++) {
        pFinout = sarray_fetch(Ntk_Node_t *, resub_list, i);
        
        dvalue = -1; iResubVar = -1;
        iResubVar = SimpNodeArrayFindIndex(fanin_list, pFinout);
        if (iResubVar<0) {
            iResubVar = Ntk_NodeReadFaninNum(node)+i;
        }
        
        /* compute new column indices */
        Ntk_NodeForEachFaninWithIndex(pFinout, pPin, pFanin, j) {
            index = Ntk_NodeReadFaninIndex(node, pFanin);
            if (index >= 0) {
                pnPos[j] = index;
            } else {
                fail("SimpComputeSdcInternal: resub node is not subset support");
            }
        }
        
        pImage = pDomain = NULL;
        
        /* compute the complement of care set */
        pCvr = Ntk_NodeGetFuncCvr(pFinout);
        pVmTmp  = Cvr_CoverReadVm(pCvr);
        nValues = Vm_VarMapReadValuesOutNum(pVmTmp);
        pIsets  = Cvr_CoverReadIsets(pCvr);
        for (j=0; j<nValues; ++j) {
            if (pIsets[j]==NULL) {
                dvalue = j;
                continue;
            }
            
            isetj = pIsets[j];
            isetj_adj = Cvr_IsetAdjust(isetj, pVm, pVmTmp, pnPos);
            if (isetj_adj==NULL) {
                assert(0);
                continue;
            }
            
            if (!Mvc_CoverIsEmpty(isetj_adj)) {
                if (pDomain) {
                    isetj_new = Mvc_CoverBooleanOr(pDomain, isetj_adj);
                    Mvc_CoverFree(pDomain);
                    pDomain = isetj_new;
                }
                else {
                    pDomain = Mvc_CoverDup(isetj_adj);
                }
                
                /* set the output bit */
                Mvc_CoverBitSet(isetj_adj, pVm, iResubVar, j);
                
                if (pImage) {
                    isetj_new = Mvc_CoverBooleanOr(pImage, isetj_adj);
                    Mvc_CoverFree(pImage);
                    Mvc_CoverFree(isetj_adj);
                    pImage = isetj_new;
                }
                else {
                    pImage = isetj_adj;
                }
            }
            else {
                Mvc_CoverFree(isetj_adj);
            }
        }
        
        /* compute SDC for the default cover as well */
        if (dvalue >= 0) {
            pDef = Cvr_CoverComplement( pVm, pDomain );
            Mvc_CoverBitSet( pDef, pVm, iResubVar, dvalue );
            
            pTmp = Mvc_CoverBooleanOr( pImage, pDef );
            Mvc_CoverFree( pImage );
            Mvc_CoverFree( pDef );
            pImage = pTmp;
        }
        
        pSdc = Cvr_CoverComplement(pVm, pImage);
        Mvc_CoverFree(pDomain);
        Mvc_CoverFree(pImage);
        
        if (pSdcAll) {
            pTmp = Mvc_CoverBooleanOr(pSdcAll, pSdc);
            Mvc_CoverFree(pSdcAll);
            Mvc_CoverFree(pSdc);
            pSdcAll = pTmp;
        } else {
            pSdcAll = pSdc;
        }
    }
    
    FREE(pnPos);
    return pSdcAll;
}

/**Function********************************************************************

  Synopsis    [Return true if all fanins of the node is contained in the set.]
  
  Description [Return true if all fanins of the node is contained in the
  set. Return false if the node is a combinational input, or the node has only
  one fanin (will not contribute to resub).]
  
  SideEffects []

  SeeAlso     []

******************************************************************************/
bool
SimpTestSubsetSupport(
    Ntk_Node_t *node,
    st_table   *support) 
{

    Ntk_Node_t *fanin;
    Ntk_Pin_t *pPin;
    char *dummy;
    
    if (Ntk_NodeIsCi(node)) return FALSE;
    if (Ntk_NodeReadFaninNum(node) <= 1) return FALSE;
    
    Ntk_NodeForEachFanin(node, pPin, fanin) {
        if(!st_lookup(support, (char *)fanin, &dummy)) {
            return FALSE;
        }
    }
    return TRUE;
}

/**Function********************************************************************
  Synopsis    [Find the index of a node in an array.]
  Description [Find the index of a node in an array.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
SimpNodeArrayFindIndex(
    sarray_t   *node_list,
    Ntk_Node_t *keynode) 
{
    int i;
    Ntk_Node_t *node;
    if (node_list==NULL) return -1;
    sarrayForEachItem(Ntk_Node_t *, node_list, i, node) {
        if (node == keynode) return i;
    }
    return -1;
}

/**Function********************************************************************
  Synopsis    [Compute the array of internal subset support nodes.]
  Description [Compute the array of internal subset support nodes, which are
  already in the fanin list.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
sarray_t *
SimpComputeSubsetSupportIntern(
    Ntk_Node_t *node) 
{

    Ntk_Node_t  *pFanin;
    Ntk_Pin_t   *pPin;

    st_table    *fnin_st;
    sarray_t    *subset_list;
    
    /* hash table for fanin list */
    fnin_st = st_init_table(st_ptrcmp, st_ptrhash);
    Ntk_NodeForEachFanin(node, pPin, pFanin) {
        st_insert(fnin_st, (char *)pFanin, (char *)1);
    }
    
    /* find the internal subset support nodes */
    subset_list = sarray_alloc( Ntk_Node_t *, Ntk_NodeReadFaninNum( node ) );
    Ntk_NodeForEachFanin( node, pPin, pFanin ) {
        
        if (SimpTestSubsetSupport( pFanin, fnin_st )) {
            sarray_insert_last( Ntk_Node_t *, subset_list, pFanin );
        }
    }
    
    st_free_table(fnin_st);
    return subset_list;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ntk_Node_t *
SimpNodeCreateFromFanins(Ntk_Network_t *pNet, sarray_t *lFanin) 
{
    int i;
    Ntk_Node_t  *pNodeNew, *pFanin;
    
    pNodeNew = Ntk_NodeCreate( pNet, NULL, MV_NODE_INT, 2 );
    sarrayForEachItem(Ntk_Node_t *, lFanin, i, pFanin) {
        
        Ntk_NodeAddFanin(pNodeNew, pFanin);
    }
    Ntk_NodeSetValueNum(pNodeNew, 2);
    Ntk_NodeAssignVm(pNodeNew);
    return pNodeNew;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
SimpNodeAssignMvc(Ntk_Node_t *pNode, Mvc_Cover_t *pMvc)
{
    Cvr_Cover_t *pCvrNew;
    Mvc_Cover_t **pIsets;
    
    pIsets    = ALLOC(Mvc_Cover_t *, 2);
    pIsets[0] = NULL;
    pIsets[1] = pMvc;
    pCvrNew   = Cvr_CoverCreate( Ntk_NodeReadFuncVm(pNode), pIsets );
    Ntk_NodeWriteFuncCvr( pNode, pCvrNew );
    return;
}



//assume all columns are filled; remove unnecessary ones.
void
Mvc_CoverBitSet(Mvc_Cover_t *pMvc, Vm_VarMap_t *pVm, int iVar, int iValue)
{
    int iBase, iSize, i;
    Mvc_Cube_t *pCube;
    
    iSize = Vm_VarMapReadValues(pVm, iVar);
    iBase = Vm_VarMapReadValuesFirst(pVm, iVar);
    Mvc_CoverForEachCube(pMvc, pCube) {
        for ( i = 0; i < iSize; ++i ) {
            if ( i != iValue) 
                Mvc_CubeBitRemove(pCube, iBase + i);
        }
    }
    return;
}


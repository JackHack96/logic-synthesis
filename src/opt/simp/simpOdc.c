/**CFile****************************************************************

  FileName    [simpOdc.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Compute don't cares for a node.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: simpOdc.c,v 1.25 2003/09/01 18:15:46 wjiang Exp $]

***********************************************************************/

#include "simpInt.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/
#define SIMP_CODC_TOO_BIG 50000
#define SIMP_MODC_TOO_BIG 60000

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static bool     SimpComputeCspf(Simp_Info_t *pInfo, DdNode **pbMspf, DdNode **pbCodc,    
                                Mvr_Relation_t *pMvrFout, DdManager *mgLoc,
                                Ntk_Node_t *pNode, Ntk_Node_t *pOther, int iOther);
static DdNode * SimpCleanNondet(DdManager *mg, Ntk_Node_t *pNode,
                                Mvr_Relation_t *pMvrFunc, int iFanin,
                                DdNode *bMspf, DdNode *bNondet);
static DdNode * SimpAdjustMspf( DdManager *ddmg, DdNode *pMspf, DdNode *pCodcFout );


/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Compute the compatible observability don't cares (CODC).]
  
  Description [Compute the compatible observability don't cares (CODC) for a
  node. Visit each fanout, obtain the maximal observability don't cares (MODC)
  first and make it compatible with the other fanins of this fanout; the
  results from all fanouts are intersected together to give the CODC set of
  this node. The result is represented in MDDs with CI variables. The full
  preparation routine Simp_FullsimpInit() has to be called to set up the MDD
  for each node. (ref: Y. Jiang, R. Brayton, ICCAD'00.)]
  
  SideEffects [the CODC field in the simp data structure of this node will be
  updated; a duplicate copy is return.]

  SeeAlso     []

******************************************************************************/
DdNode *
Simp_ComputeCodc(
    Simp_Info_t *pInfo,
    Ntk_Node_t  *pNode)
{
    bool           fCmptbl;
    int            k, index;
    DdNode         *pCspf, *pDc, *pTemp, *pMspf, *pGlob;
    DdManager      *ddmg, *mgLoc;
    Simp_Node_t    *pSimp, *pSimpFout, *pSimpFin;
    Ntk_Node_t     *pFanout, *pFaninOth;
    Ntk_Pin_t      *pPin1, *pPin2;
    Ntk_Network_t  *pNet;
    Mvr_Relation_t *pMvrFout;
    st_table       *stCmptbl;
    char           *dummy;
    
    if (pNode==NULL) return NULL;
    pSimp = Simp_DaemonGetNodeData(pNode);
    if (pSimp==NULL) return NULL;
    
    pSimp->fVisited = TRUE;
    if (Ntk_NodeIsCoFanin( pNode )) {
        pCspf = Cudd_ReadLogicZero(pInfo->ddmg);
        Cudd_Ref( pCspf );
        return pCspf;
    }
    
    /* retrieve information */
    pNet = pInfo->network;
    ddmg = pInfo->ddmg;
    
    /* initiate the CSPF computation */
    pCspf    = NULL;
    stCmptbl = st_init_table(st_ptrcmp, st_ptrhash);
    
    /* compute the CODC set from each fanout */
    Ntk_NodeForEachFanout( pNode, pPin1, pFanout) {
        
        pSimpFout = Simp_DaemonGetNodeData(pFanout);
        if (pSimpFout->fBadnode) continue;
        
        pMvrFout = Ntk_NodeGetFuncMvr( pFanout );
        mgLoc    = Mvr_RelationReadDd( pMvrFout );
        index    = Ntk_NodeReadFaninIndex( pFanout, pNode );
        fCmptbl  = FALSE;
        
        if ( pSimpFout->lMspf==NULL ||
             pSimpFout->lMspf[index] == NULL ) {
            pDc = Cudd_ReadLogicZero(ddmg);
            Cudd_Ref( pDc );
        }
        else {
            pMspf = pSimpFout->lMspf[index];
            Cudd_Ref( pMspf );
            
            if (pInfo->verbose) {
                printf("Fanout [%s] MODC: size=%d ",Ntk_NodeGetNamePrintable(pFanout),
                        Cudd_DagSize(pMspf));
            }
            
            /* collapse into the global MDD space */
            pTemp = Ntk_NodeGlobalMddCollapse( ddmg, pMspf, pFanout );
            Cudd_Ref( pTemp );
            
            /* implementation independent CODC (Brayton, ICCAD'01) */
            if ( pSimpFout->pCodc != NULL ) {
                pDc = SimpAdjustMspf( ddmg, pTemp, pSimpFout->pCodc );
                Cudd_RecursiveDeref( ddmg, pTemp );
            }
            else {
                pDc = pTemp;
            }
            
            if (pInfo->verbose) {
               printf(" MODC(PI): size=%d\n", Cudd_DagSize(pDc));
            }
            
            /* make the mspf compatible with previous edges. */
            if ( pInfo->dc_type == 0 ) {
            Ntk_NodeForEachFaninWithIndex (pFanout, pPin2, pFaninOth, k) {
                if (k == index || Ntk_NodeIsCi(pFaninOth))
                    continue;
                
                pSimpFin = Simp_DaemonGetNodeData(pFaninOth);
                
                /* compatibility check for visited node;
                   even if it hasn't used its CODC, its fanins may;
                   so we have to make compatible anyway
                */
                if ( !pSimpFin->fVisited /*|| !pSimpFin->fChanged */ ) {
                    continue;
                }
                
                /* skip if already compatible */
                if (st_lookup(stCmptbl, (char *)pFaninOth, &dummy)) {
                    continue;
                }
                st_insert(stCmptbl, (char *)pFaninOth, dummy);
                
                /* ruggedness control */
                if (Cudd_DagSize(pDc) > SIMP_MODC_TOO_BIG) {
                    if (pInfo->verbose) {
                        printf("CODC size > %d; stopped\n", SIMP_MODC_TOO_BIG);
                    }
                    Cudd_RecursiveDeref( ddmg, pDc );
                    pDc = Cudd_ReadLogicZero( ddmg ); Cudd_Ref( pDc );
                    fCmptbl = FALSE;
                    break;
                }
                
                SIMP_EXEC(fCmptbl = SimpComputeCspf(pInfo,&pMspf,&pDc,pMvrFout,mgLoc,pNode,pFaninOth,k),
                          (pInfo->time_cspf) );
                
                if (pDc == Cudd_Not(DD_ONE(ddmg)))
                    break;
                
            }
            } /* if (dc_type == 0) */
            
            /* combine MSPF(local) and CODC(global) */
            if ( fCmptbl ) {
                
                pTemp = Ntk_NodeGlobalMddCollapse( ddmg, pMspf, pFanout ); Cudd_Ref( pTemp );
                pGlob = Cudd_bddOr( ddmg, pTemp, pDc ); Cudd_Ref( pGlob );
                Cudd_RecursiveDeref( ddmg, pTemp );
                Cudd_RecursiveDeref( ddmg, pDc );
                pDc = pGlob;
            }
            
            Cudd_RecursiveDeref( mgLoc, pMspf );
        }
        
        /* do the filtering early to speed up CSPF;
           but doing so also reduce possible DC's
        */
        if ( pInfo->fFilter ) {
            pDc = SimpFilterGlobal( pInfo, pDc, pSimp->stCi );
        }
        
        /* OR the computed CSPF with the CSPF of the fanout node.
         * directly OR since they both depend on CI's
         */
        if ( pSimpFout->pCodc != NULL ) {
            pTemp = Cudd_bddOr( ddmg, pSimpFout->pCodc, pDc ); Cudd_Ref(pTemp);
            Cudd_RecursiveDeref( ddmg, pDc );
            pDc = pTemp;
        }
        
        /* Do the intersection over all fanout edges to find the CSPF of this node */
        if (pCspf == NULL) {
            pCspf = pDc;
        }
        else {
            pTemp = Cudd_bddAnd( ddmg, pCspf, pDc); Cudd_Ref( pTemp );
            Cudd_RecursiveDeref( ddmg, pDc );
            Cudd_RecursiveDeref( ddmg, pCspf );
            pCspf = pTemp;
        }
        
        if ( pCspf == Cudd_Not(DD_ONE(ddmg)) ) {
              break;  /* no sense of continuing, empty anyways */
        }
        if (Cudd_DagSize(pCspf) > SIMP_CODC_TOO_BIG) {
              if (pInfo->verbose) {
                  printf("CODC size > %d; stopped\n", SIMP_CODC_TOO_BIG);
              }
              Cudd_RecursiveDeref( ddmg, pCspf );
              pCspf = NULL;
              break;
        }
    }  /* foreach fanout */
    
    st_free_table(stCmptbl);
    
    /* CSPF for this node completed */
    if (pCspf != NULL) {
        
        if (pSimp->pCodc == NULL) {
            pSimp->pCodc = pCspf;
        }
        else {
            pTemp = Cudd_bddOr( ddmg, pCspf, pSimp->pCodc );
            Cudd_Ref( pTemp );
            Cudd_RecursiveDeref( ddmg, pCspf );
            Cudd_RecursiveDeref( ddmg, pSimp->pCodc );
            pSimp->pCodc = pTemp;
        }
    }
    
    /* collapse node that are not in the TFI for node minimization;
       this may speed up image computation, but also reduces its CODC;
       however may also allow larger CODC for neighbors.
     */
    if ( pSimp->pCodc && !Cudd_IsConstant( pSimp->pCodc )
         && pInfo->fFilter) {
        pSimp->pCodc = SimpFilterGlobal( pInfo, pSimp->pCodc, pSimp->stCi );
    }
    
    
    /* return a fresh copy */
    pCspf = pSimp->pCodc;
    if ( pCspf) Cudd_Ref( pCspf );
    
    return (pCspf);
}


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************
  Synopsis    [Computes the maximal set of permissible functions (MSPF)]
  Description [For all input edges of node, using mdd_consensus(). Results are
  store in the MDD array of simp_node_t data strusture. The resulting MSPF
  BDD's are in the local manager space.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
SimpComputeMspf(
    Ntk_Node_t  *pNode,
    Simp_Info_t *pInfo) 
{
    int i, nInputs, iFanin, nValues;
    Simp_Node_t    *pSimp;
    Ntk_Node_t     *pFanin;
    Ntk_Pin_t      *pPin;
    Mvr_Relation_t *pMvrFunc;
    Vm_VarMap_t    *pVm;
    Vmx_VarMap_t   *pVmx;
    DdNode         *bMspf, *bQuan, *bRel, *bCube, *bNondet, *bTemp, *bSum;
    DdNode        ** pbIsets;
    DdManager      *mgLoc;
    
    
    /* nothing to do for constant/CI nodes */
    nInputs = Ntk_NodeReadFaninNum(pNode);
    if (nInputs == 0) return;
    
    pSimp = Simp_DaemonGetNodeData(pNode);
    assert( pSimp && (pSimp->lMspf==NULL) );
    
    /* MODC=empty for single fanin nodes */
    pSimp->lMspf = ALLOC(DdNode *, nInputs);
    memset( pSimp->lMspf, 0, nInputs * sizeof(DdNode *) );
    
    if (nInputs == 1) {
        return;
    }
    
    /* MDD manager for the local space */
    pMvrFunc = Ntk_NodeGetFuncMvr(pNode);
    mgLoc = Mvr_RelationReadDd(pMvrFunc);
    
    /* obtain the MDD array for the isets */
    pVm     = Mvr_RelationReadVm( pMvrFunc );
    pVmx    = Mvr_RelationReadVmx( pMvrFunc );
    bRel    = Mvr_RelationReadRel( pMvrFunc );
    bCube   = Vmx_VarMapCharCube( mgLoc, pVmx, nInputs );
    Cudd_Ref( bCube );
    nValues = Vm_VarMapReadValuesOutNum( pVm );
    
    /* issue to study:
       MODC formulea (ICCAD'00) works only for deterministic MV functions;
       need to remove the non-det part from MODC;
       (\exist f) (\forany y) R(x,y,f)
    */
    
    bNondet = NULL;
    if ( Mvr_RelationIsND( pMvrFunc ) ) {
        
        bNondet = Cudd_ReadLogicZero( mgLoc );
        Cudd_Ref( bNondet );
        
        pbIsets = ALLOC( DdNode *, nValues );
        Mvr_RelationCofactorsDerive( pMvrFunc, pbIsets, nInputs, nValues );
        
        /* initialize the sum to be 0-set */
        bSum = pbIsets[0];
        Cudd_Ref( bSum );
        
        /* accumulate the overlapping sets */
        for ( i = 1; i < nValues; i++ ) {
            
            /* intersect with the sum */
            bQuan = Cudd_bddAnd( mgLoc, bSum, pbIsets[i] );
            Cudd_Ref( bQuan );
            
            /* include the overlap into the nondet part */
            bTemp = Cudd_bddOr( mgLoc, bNondet, bQuan );
            Cudd_Ref( bTemp );
            Cudd_RecursiveDeref( mgLoc, bQuan );
            Cudd_RecursiveDeref( mgLoc, bNondet );
            bNondet = bTemp;
            
            /* include iset[i] into the sum */
            if ( i != (nValues - 1) ) {
                bTemp = Cudd_bddOr( mgLoc, bSum, pbIsets[i] );
                Cudd_Ref( bTemp );
                Cudd_RecursiveDeref( mgLoc, bSum );
                bSum = bTemp;
            }
        }
        Cudd_RecursiveDeref( mgLoc, bSum );
        Mvr_RelationCofactorsDeref( pMvrFunc, pbIsets, nInputs, nValues );
        FREE( pbIsets );
        
        /* remove the part that is truly don't cares */
        bQuan = Mvr_RelationQuantifyUniv( pMvrFunc, bRel, nInputs );
        Cudd_Ref( bQuan );
        
        bTemp = Cudd_bddAnd( mgLoc, bNondet, Cudd_Not( bQuan ) );
        Cudd_Ref( bTemp );
        Cudd_RecursiveDeref( mgLoc, bQuan );
        Cudd_RecursiveDeref( mgLoc, bNondet );
        bNondet = bTemp;
    }
    
    /* compute MSPF for each fanin */
    Ntk_NodeForEachFaninWithIndex( pNode, pPin, pFanin, iFanin ) {
        
        if ( Ntk_NodeIsCi( pFanin ) )
            continue;
        
        bQuan = Mvr_RelationQuantifyUniv( pMvrFunc, bRel, iFanin );
        Cudd_Ref( bQuan );
        
        bMspf = Cudd_bddExistAbstract( mgLoc, bQuan, bCube );
        Cudd_Ref( bMspf );
        Cudd_RecursiveDeref( mgLoc, bQuan );
        
        if ( bNondet ) {
            bMspf = SimpCleanNondet( mgLoc, pNode, pMvrFunc, iFanin, bMspf, bNondet );
        }
        
        pSimp->lMspf[iFanin] = bMspf;
    }
    
    Cudd_RecursiveDeref( mgLoc, bCube );
    
    /* deallocate memory of MDD isets */
    if ( bNondet ) {
       Cudd_RecursiveDeref( mgLoc, bNondet );
    }
    return;
}


/**Function********************************************************************

  Synopsis    [Filter out variables that are not in the hash table list.]
  
  Description [Given a MDD, which represents a don't care set. Assume all
  variables in the support are already CI's. All variables not in the TFI set
  are exitentially quantified out.]

  SideEffects [the original MDD F is freed afterwards]

  SeeAlso     []

******************************************************************************/
DdNode *
SimpFilterGlobal(
    Simp_Info_t *pInfo,
    DdNode       *bFunc,
    st_table     *stNodes)
{
    int iSeq, *pSup, nVars;
    char          *dummy;
    Ntk_Node_t    *pStick;
    DdManager     *mg;
    DdNode        *bTemp, *bResult, *bCube;
    
    mg    = pInfo->ddmg;
    if ( bFunc == Cudd_ReadOne( mg ) ||
         bFunc == Cudd_ReadLogicZero( mg ) ) {
        return bFunc;
    }
    
    /* access information */
    pStick = NULL;
    nVars = Cudd_ReadSize( mg );
    pSup  = Cudd_SupportIndex( mg, bFunc );
    bCube = Cudd_ReadOne( mg );
    Cudd_Ref( bCube );
    
    for ( iSeq = 0; iSeq < nVars; ++iSeq ) {
        
        if ( pSup[iSeq] > 0 ) {
            
            /* new test if it is in stNodes as well */
            if ( pStick == pInfo->ppNodes[iSeq] ||
                 !st_lookup(stNodes, (char *)pInfo->ppNodes[iSeq], &dummy)) {
                
                /* accumulate the variables in a cube */
                bTemp = Cudd_bddAnd( mg, bCube, Cudd_bddIthVar(mg, iSeq) );
                Cudd_Ref( bTemp );
                Cudd_RecursiveDeref( mg, bCube );
                bCube  = bTemp;
                
                pStick = pInfo->ppNodes[iSeq];
            }
        }
    }
    
    bResult = Cudd_bddUnivAbstract( mg, bFunc, bCube );
    Cudd_Ref( bResult );
    Cudd_RecursiveDeref( mg, bFunc );
    Cudd_RecursiveDeref( mg, bCube );
    
    FREE(pSup);
    return bResult;
}


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Makes the MSPF compatible with a previous visited edge.]
  
  Description [Makes the MSPF compatible with a previous visited edge. Given
  the current CODC of a node, and another previously visited node, compute its
  compatible subset. Assume both are in the global MDD manager space.]

  SideEffects [The original codc set is freed after wards.]

  SeeAlso     []

******************************************************************************/
bool
SimpComputeCspf(
    Simp_Info_t    * pInfo,
    DdNode        ** pbMspf,   /* boolean difference in local DD domain   */
    DdNode        ** pbCodc,   /* current CODC in global DD domain        */
    Mvr_Relation_t * pMvrFout, /* fanout relation where pMspf was derived */
    DdManager      * mgLoc,    /* local DD manager for pMspf              */
    Ntk_Node_t     * pNode,    /* current node being worked on            */
    Ntk_Node_t     * pOther,   /* neighbor node being compatiblized       */
    int              iOther )
{
    DdNode         *bQuan, *bTemp, *bOthDc;
    DdNode         *bCodc, *bMspf;
    DdManager      *mg;
    Simp_Node_t    *pSimpThi, *pSimpOth;
    
    /* global MDD manager */
    mg = pInfo->ddmg;
    bCodc = *pbCodc;
    bMspf = *pbMspf;
    
    /* retrieve the computed codc from the other node */
    pSimpThi = Simp_DaemonGetNodeData( pNode );
    pSimpOth = Simp_DaemonGetNodeData( pOther );
    bOthDc   = pSimpOth->pCodc;
    if ( bOthDc==NIL(DdNode) || bOthDc == Cudd_ReadLogicZero(mg) ) {
        return FALSE;
    }
    
    if (pInfo->verbose) {
        printf("Compatability: %s(%d) ", Ntk_NodeGetNamePrintable(pOther),
               Cudd_DagSize(bOthDc) );
        printf("<- %s(%d)\n", Ntk_NodeGetNamePrintable(pNode),
               bCodc?Cudd_DagSize(bCodc):0 );
    }
    
    /* compute (ANYx)CODC - universal quantification (in the local domain) */
    if ( bMspf && bMspf != Cudd_ReadLogicZero( mgLoc ) ) {
        
        bQuan = Mvr_RelationQuantifyUniv( pMvrFout, bMspf, iOther );
        Cudd_Ref( bQuan );
        Cudd_RecursiveDeref( mgLoc, bMspf );
        *pbMspf = bQuan;
    }
    
    /* compute Care(x) * CODC: (in the global domain ok) */
    if ( bCodc && bCodc != Cudd_ReadLogicZero( mg ) ) {
        
        bTemp = Cudd_bddAnd( mg, Cudd_Not(bOthDc), bCodc ); Cudd_Ref( bTemp );
        Cudd_RecursiveDeref( mg, bCodc );
        *pbCodc = bTemp;
    }
    
    return TRUE;
}

/**Function********************************************************************

  Synopsis    [Remove non-deterministic parts from MODC]
  
  Description [Remove non-deterministic parts from MODC, using equation :
  ( \forall x \not{NonDet()} \cdot MSPF )]
  
  SideEffects []
  SeeAlso     []

******************************************************************************/
DdNode *
SimpCleanNondet(
    DdManager      *mg,
    Ntk_Node_t     *pNode,
    Mvr_Relation_t *pMvrFunc,
    int             iFanin,
    DdNode         *bMspf,
    DdNode         *bNondet)
{
    DdNode *bTemp, *bQuan;
    
    if ( bMspf == NIL(DdNode) ) return NIL(DdNode);
    
    bQuan = Mvr_RelationQuantifyUniv( pMvrFunc, Cudd_Not(bNondet), iFanin );
    Cudd_Ref( bQuan );
    
    bTemp = Cudd_bddAnd( mg, bMspf, bQuan);
    Cudd_Ref( bTemp );
    
    Cudd_RecursiveDeref( mg, bMspf );
    Cudd_RecursiveDeref( mg, bQuan );
    
    return bTemp;
}


/**Function********************************************************************

  Synopsis    [Adjust the MSPF with the fanout's CODC]
  
  Description [Compute MSPF that is independent of the fanout's
  implementation, using formulea: (MSPF + \exist x_i CODC) given by
  Brayton, ICCAD'01.]

  SideEffects []
  SeeAlso     []

******************************************************************************/
DdNode *
SimpAdjustMspf( DdManager * ddmg, DdNode * pMspf, DdNode * pCodcFout )
{
    DdNode *pMspfNew;
    
    /* since we are in the global space already,
       thus don't need to quantify out x_i
    */
    pMspfNew = Cudd_bddOr( ddmg, pMspf, pCodcFout );
    Cudd_Ref( pMspfNew );
    
    return pMspfNew;
}

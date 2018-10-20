/**CFile****************************************************************

  FileName    [simpSimplify.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [MV network simplification]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: simpSimplify.c,v 1.29 2004/05/12 04:30:14 alanmi Exp $]

***********************************************************************/

#include "simpInt.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Simplify each node without CODC computation]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void
Simp_NetworkSimplify(
    Ntk_Network_t *pNet,
    Simp_Info_t   *pInfo)
{
    ProgressBar *pProgress = NULL;
    Ntk_Node_t  *pNode, *pNsdc=NULL;
    Simp_Method_t acceptType;
    int iNode;
    
    pNsdc = NULL;
    acceptType = pInfo->accept;
    
    /* clean up the pData field */
    Ntk_NetworkForEachCo( pNet, pNode ) {
        Simp_DaemonSetNodeData(pNode, NULL);
    }
    Ntk_NetworkForEachCi( pNet, pNode ) {
        Simp_DaemonSetNodeData(pNode, NULL);
    }
    Ntk_NetworkForEachNode( pNet, pNode ) {
        Simp_DaemonSetNodeData(pNode, NULL);
    }
    
    pProgress = Extra_ProgressBarStart( stdout, Ntk_NetworkReadNodeIntNum(pNet) );

    iNode = 0;
    Ntk_NetworkForEachNode( pNet, pNode ) {
        iNode++;        
        if (Ntk_NodeReadFaninNum(pNode) <= 1)
            continue;
        if (Ntk_NodeReadFaninNum(pNode) >= pInfo->fanin_max)
            continue;
        
        if (pInfo->verbose) {
            printf("simplify %s\n", Ntk_NodeGetNamePrintable(pNode));
        }
        
        /* do not compute SDC */
        if (pInfo->dc_type == 0) {
            (void) Simp_NodeSimplify(pNode, pInfo->method, acceptType,
                                     pInfo->fSparse, pInfo->fConser,
                                     pInfo->fPhase,  pInfo->fRelatn);
        }
        else if (pInfo->dc_type == 1) {
            
            if (pInfo->use_bres) {
                pNsdc = Simp_ComputeSdcNewBase(pNode);
            }
            else {
                pNsdc = Simp_ComputeSdcLocal(pNode);
            }
            (void) Simp_NodeSimplifyDc(pNode, pNsdc, pInfo->method, acceptType,
                                       pInfo->fSparse, pInfo->fConser,
                                       pInfo->fPhase,  pInfo->fRelatn);
        }
        if (pNsdc) {
            Ntk_NodeDelete(pNsdc); pNsdc = NULL;
        }

        // update the progress bar
        if ( pProgress && iNode % 50 == 0 )
            Extra_ProgressBarUpdate( pProgress, iNode, NULL );
    }
    if ( pProgress )
        Extra_ProgressBarStop( pProgress );
    
    return;
}


/**Function********************************************************************

  Synopsis    [Simplify a perticular node using don't cares provided.]

  Description [Simplify a perticular node using don't cares provided.  The
  default algorithm is espresso. Note: DC set gets freed in the end.  This
  procedure can not be timed out. Return TRUE if the logic function of the
  node is changed.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
bool
Simp_NodeSimplifyDc(
    Ntk_Node_t *      pnode,
    Ntk_Node_t *      dcnode,
    Simp_Method_t     method,
    Simp_AcceptType_t acceptType,
    bool              fMakeSparse,
    bool              fConserve,
    bool              fPhase,
    bool              fRelation)
{
    bool              fAccepted, fCommonBase;
    Cvr_Cover_t      *pCf, *pCd, *pCg;
    Mvc_Cover_t      *pMd;
    Ntk_Node_t       *pNodeNew;
    Vm_VarMap_t      *pVm, *pVmNew;
    Mvr_Relation_t   *pMvr;
    int              *pMapF, *pMapD;
    
    
    /* don't care set is empty */
    if ( dcnode == NIL(Ntk_Node_t) ) {
        fAccepted = Simp_NodeSimplify(pnode, method, acceptType,
                                      fMakeSparse, fConserve, fPhase, fRelation);
        return fAccepted;
    }
    if ( SimpNodeIsHardCase( pnode ) ) {
        
        (void) Simp_NodeSimplify(pnode, SIMP_SIMPLE, acceptType,
                                 fMakeSparse, 1, 0, 0);
        return FALSE;
    }
    
    pVm = Ntk_NodeReadFuncVm(pnode);
    
    /* don't care set is tautology */
    if (Ntk_NodeIsConstant(dcnode)) {
        
        unsigned iPos;
        pMvr = Ntk_NodeGetFuncMvr( dcnode );
        iPos = Mvr_RelationGetConstant( pMvr );
        
        if ( iPos & 1 ) {
            fAccepted = Simp_NodeSimplify(pnode, method, acceptType,
                                          fMakeSparse, fConserve, fPhase, fRelation);
        }
        else if ( iPos & 2 ) {
            
            Ntk_Node_t  *pNconst;
            
            /* should keep the relations here, but determinize for now (11/4)*/
            pNconst = Ntk_NodeCreateConstant(Ntk_NodeReadNetwork(pnode),
                                             Vm_VarMapReadValuesOutNum(pVm), 2);
            SimpNodeReplace( pnode, pNconst );
            fAccepted = 1;
        }
        else {
            assert(0);
        }
        return fAccepted;
    }
    
    /* pnode should have been ordered */
    //Ntk_NodeOrderFanins( pnode );
    Ntk_NodeOrderFanins( dcnode );  //for commonbase to work
    pCg = Ntk_NodeGetFuncCvr(pnode);
    pCd = Ntk_NodeGetFuncCvr(dcnode);
    
    
    /* derive a new node for minimization when not common base */
    fCommonBase = SimpNodeCompareFanin( pnode, dcnode );
    if ( fCommonBase ) {
        
        pCf = Cvr_CoverDup( pCg );
        pMd = Mvc_CoverDup( Cvr_CoverReadIsetByIndex(pCd,1) );
    }
    else {
        
        pMapF = ALLOC(int, Ntk_NodeReadFaninNum(pnode));
        pMapD = ALLOC(int, Ntk_NodeReadFaninNum(dcnode));
        
        pNodeNew = Ntk_NodeMakeCommonBase(pnode, dcnode, pMapF, pMapD);
        pVmNew   = Ntk_NodeGetFuncVm(pNodeNew);
        pCf = Cvr_CoverCreateAdjust(pCg, pVmNew, pMapF);
        pMd = Cvr_IsetAdjust( Cvr_CoverReadIsetByIndex(pCd,1),pVmNew,
                              Cvr_CoverReadVm(pCd), pMapD);
    }
    
    if ( fRelation && (Vm_VarMapReadValuesOutNum( pVm ) > 2 ||
                       Cvr_CoverReadDefault( pCf ) < 0 ) ) {
        
        Cvr_CoverEspressoRelation(pCf, pMd, (int)method, fMakeSparse, fConserve, fPhase);
    }
    else {
        
        Cvr_CoverEspresso(pCf, pMd, (int)method, fMakeSparse, fConserve, fPhase);
    }
    
    fAccepted = Simp_AcceptResult(pCg, pCf, acceptType, 0);
    
    /* replace the whole node if accepted */
    if (fAccepted) {
        
        if ( fCommonBase ) 
            pNodeNew = SimpNodeClone( pnode );
        Ntk_NodeSetValueNum( pNodeNew, Ntk_NodeReadValueNum( pnode ) );
        Ntk_NodeWriteFuncCvr( pNodeNew, pCf );
        SimpNodeReplace( pnode, pNodeNew );
    }
    else {
        Cvr_CoverFree(pCf);
        if (!fCommonBase) Ntk_NodeDelete( pNodeNew );
    }
    
    Mvc_CoverFree(pMd);
    if ( !fCommonBase ){
        FREE(pMapF);
        FREE(pMapD); 
    }
    
    (void) Ntk_NodeMakeMinimumBase( pnode );
    
    return fAccepted;
}



/**Function********************************************************************

  Synopsis    [Simplify a perticular node without don't cares.]

  Description [Simplify a perticular node without don't cares. The
  default algorithm is espresso.  Return TRUE if the logic function of the
  node is changed.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
bool
Simp_NodeSimplify(
    Ntk_Node_t *      pnode,
    Simp_Method_t     method,
    Simp_AcceptType_t acceptType,
    bool              fMakeSparse,
    bool              fConserve,
    bool              fPhase,
    bool              fRelation)
{
    bool         fAccepted;
    int          nVals, iDef;
    Cvr_Cover_t *pCold, *pCnew;
    Vm_VarMap_t *pVm;
    
    if (pnode==NULL)                    return FALSE;
    if (Ntk_NodeReadFaninNum(pnode)<=1) return FALSE;
    if (Ntk_NodeIsConstant( pnode ) )   return FALSE;
    
    fAccepted = FALSE;
    pVm      = Ntk_NodeReadFuncVm(pnode);
    pCold    = Ntk_NodeGetFuncCvr(pnode);
    pCnew    = Cvr_CoverDup(pCold);
    nVals    = Vm_VarMapReadValuesOutNum( pVm );
    iDef     = Cvr_CoverReadDefault( pCnew );
    
    /* detect hard cases */
    if ( method == SIMP_SIMPLE || SimpNodeIsHardCase( pnode ) ) {
        
        Cvr_CoverEspresso(pCnew, NULL, (int)SIMP_SIMPLE, fMakeSparse, 1, 0);
    }
    else if ( fRelation && (nVals > 2 || iDef < 0 )
              || ( nVals == 2 && iDef < 0 )  )  {
        
        Cvr_CoverEspressoRelation(pCnew, NULL, (int)method, fMakeSparse, fConserve, fPhase );
    }
    else {
        
        Cvr_CoverEspresso(pCnew, NULL, (int)method, fMakeSparse, fConserve, fPhase);
    }
    
    fAccepted = Simp_AcceptResult(pCold, pCnew, acceptType, 0);
    
    if (fAccepted) {
        Ntk_NodeSetFuncCvr(pnode, pCnew);
    }
    else {
        Cvr_CoverFree(pCnew);
    }
    
    (void) Ntk_NodeMakeMinimumBase( pnode );
    
    return fAccepted;
}

/**Function********************************************************************

  Synopsis    [Return true if the new node has lower cost]

  Description [Return true if the new node has lower cost, base on criterion
  given by acceptType.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
bool
Simp_AcceptResult(
    Cvr_Cover_t *    old_cover,
    Cvr_Cover_t *    new_cover,
    Simp_AcceptType_t acceptType,
    bool verbose)
{
    int  nOld, nNew;
    bool fAccepted=FALSE;
    
    if (new_cover==NULL || old_cover==NULL) {
        fail("Simp_AcceptResult: empty covers.\n");
    }
    
    nOld = nNew = 0;
    if (acceptType == SIMP_CUBE) {
        nOld = Cvr_CoverReadCubeNum(old_cover);
        nNew = Cvr_CoverReadCubeNum(new_cover);
        if (nOld == nNew) {
            nOld = Cvr_CoverReadLitSopNum(old_cover);
            nNew = Cvr_CoverReadLitSopNum(new_cover);
        }
    }
    else if (acceptType == SIMP_SOP_LIT) {
        nOld = Cvr_CoverReadLitSopNum(old_cover);
        nNew = Cvr_CoverReadLitSopNum(new_cover);
    }
    else if (acceptType == SIMP_FCT_LIT) {
        nOld = Cvr_CoverReadLitFacNum(old_cover);
        nNew = Cvr_CoverReadLitFacNum(new_cover);
    }
    else if (acceptType == SIMP_ALWAYS) {
        fAccepted = TRUE;
    }
    else {
        fail("Simp_AcceptResult: Wrong acceptance criterion\n");
    }
    
    /* break tie by favoring larger number of values in the SOP;
       larger number of values is likely to consist of primes */
    if ( !fAccepted && nNew == nOld ) {
        nOld = - Cvr_CoverReadLitSopValueNum(old_cover);
        nNew = - Cvr_CoverReadLitSopValueNum(new_cover);
    }
    
    if ( fAccepted || nNew<nOld ) {
        if (verbose) {
            printf("-->simplified<--[%3d/%3d]\n", nOld, nNew);
        }
        return TRUE;
    } else {
        if (verbose) {
            printf("-->not simplified<--[%3d/%3d]\n", nOld, nNew);
        }
        return FALSE;
    }
}


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/



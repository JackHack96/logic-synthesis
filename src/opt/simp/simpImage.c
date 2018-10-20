/**CFile****************************************************************

  FileName    [simpImage.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Image computation]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: simpImage.c,v 1.21 2003/05/27 23:16:15 alanmi Exp $]

***********************************************************************/

#include "simpInt.h"
#include "var_set.h"


/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static Ntk_Node_t  *SimpComputeImageIntern(Simp_Info_t *i,Ntk_Node_t *n,DdNode *codc);
static Mvc_Cover_t *SimpComputeRangeRecur(Simp_Info_t *pInfo, sarray_t **flist,
                                          sarray_t **aNodes, Vm_VarMap_t *pVm,
                                          Mvc_Data_t *pMvcData, int start_index);
static Mvc_Cover_t *SimpRangeOne   ( Simp_Info_t *pInfo, Vm_VarMap_t *pVm,
                                     int i, var_set_t *s,sarray_t *f );
static Mvc_Cover_t *SimpRangeTwo   ( Simp_Info_t *pInfo, Vm_VarMap_t *pVm,
                                     Mvc_Data_t *pMvcData,int i, var_set_t *s,
                                     sarray_t *f );
static Mvc_Cover_t *SimpRangeCofact( Simp_Info_t *info, Vm_VarMap_t *pVm,
                                     Mvc_Data_t *pMvcData, int i, var_set_t *s,
                                     sarray_t *f, sarray_t **n );

static Mvc_Cover_t *SimpCubeLiteral     (Mvc_Manager_t *pMem,Vm_VarMap_t *pVm, int iIndex, int iConst);
static Mvc_Cover_t *SimpCubeLiteralRange(Mvc_Manager_t *pMem,Vm_VarMap_t *pVm, int iIndex, sarray_t *r);
static Mvc_Cover_t *SimpCubeLiteralImage(Mvc_Manager_t *pMem,Vm_VarMap_t *pVm, DdManager *mg,
                                         int index, Mva_Func_t *pMva );


static sarray_t    *SimpDisjointSupports(sarray_t *nodes, sarray_t *funcs,
                                         int start_index, Simp_Info_t *pInfo);
static bool         SimpNodeCheckSupport(Ntk_Node_t *node);
static Mvc_Data_t * SimpAllocateMvcData( Ntk_Node_t *pNode );
static void         SimpFreeMvcData( Ntk_Node_t *pNode, Mvc_Data_t *pMvcData );


/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Compute the image of a don't care set.]

  Description [Compute the image of a don't care set. Given a don't care set
  represented in MDDs with CI variables, this routine uses recursively image
  computation to map the care-set to the local support of the node; and finaly
  complement again to give the local don't care set, returned as a separate
  node (may have different fanin nodes vs. the original one.) 
  If the image computation failed, then the SDC set of
  the local fanin support will be computed for substitute.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
Ntk_Node_t *
Simp_ComputeImage(
    Simp_Info_t *pInfo,
    Ntk_Node_t  *pNode,
    DdNode      *bCodc)
{
    DdManager   *mg;
    DdNode      *bCare;
    Simp_Node_t *simp_data;
    Ntk_Node_t  *pNodeDc, *pNodeCare, *pNodeSdc, *pNodeTmp;
    
    if (pNode==NULL) return NULL;
    
    simp_data = Simp_DaemonGetNodeData(pNode);
    if (simp_data==NULL) return NULL;
    
    mg = pInfo->ddmg;
    
    /* special cases */
    if (bCodc == NULL) {
        bCodc = Cudd_ReadLogicZero(mg);
        Cudd_Ref( bCodc );
    }
    else if (bCodc == Cudd_ReadOne(mg)) {
        /* whole space is don't care, return constant one node */
        pNodeDc = Ntk_NodeCreateConstant( Ntk_NodeReadNetwork(pNode), 2, 2 );
        if (pInfo->verbose) {
            printf("CODC(PI): tautology\n");
        }
        return pNodeDc;
    }
    else {
        Cudd_Ref(bCodc);
        //bCodc = SimpFilterGlobal(pInfo, bCodc, simp_data->stCi);
    }
    
    if (pInfo->verbose) {
        printf("CODC(PI):\t[%d]\n", Cudd_DagSize(bCodc));
    }
    
    /* if all fanins are PI's and DC set is empty then return SDC */
    if (SimpNodeCheckSupport(pNode) && bCodc==Cudd_ReadLogicZero(mg)) {
        
        if (pInfo->verbose) {
            printf("SimpComputeImage: avoid image computation by SDC.\n");
        }
        Cudd_RecursiveDeref( mg, bCodc );
        if (pInfo->use_bres) {
            return Simp_ComputeSdcNewBase(pNode); /* boolean resub */
        } else {
            return Simp_ComputeSdcLocal(pNode);
        }
    }
    
    bCare = Cudd_Not(bCodc);
    
    /* the image would include SDC with boolean resub */
    
    SIMP_EXEC(pNodeCare=SimpComputeImageIntern(pInfo, pNode, bCare),
              pInfo->time_imag);
    Cudd_RecursiveDeref( mg, bCare );
    
    /* if image failed then return SDC */
    if (pNodeCare == NULL) {
        if ( pInfo->use_bres ) {
            pNodeDc = Simp_ComputeSdcNewBase(pNode);
        }
        else {
            pNodeDc = Simp_ComputeSdcLocal(pNode);
        }
    }
    else {
        pNodeDc  = SimpNodeComplement( pNodeCare );
        Ntk_NodeDelete( pNodeCare );
        Ntk_NodeOrderFanins( pNodeDc );
        
        /* supplement the image with subset support SDC */
        if (pInfo->use_bres) {
            
            pNodeSdc = Simp_ComputeSdcNewBase(pNode);
            if ( pNodeSdc ) {
                Ntk_NodeOrderFanins( pNodeSdc );
                pNodeTmp = SimpNodeOr( pNodeDc, pNodeSdc );
                Ntk_NodeDelete( pNodeDc );
                Ntk_NodeDelete( pNodeSdc );
                pNodeDc = pNodeTmp;
            }
        }
    }
    
    return pNodeDc;
}



/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Partition the array of function into disjoint support sets.]

  Description [Partition the array of function into disjoint support
  sets. Returns an array of var_set's; each var_set is a set of nodes with
  common supports; the var_set contains a set of indexes (int) into the
  original array.]]

  SideEffects []
  SeeAlso     []
******************************************************************************/
sarray_t *
SimpDisjointSupports(
    sarray_t *nodes,
    sarray_t *funcs,
    int      start_index,
    Simp_Info_t *pInfo) 
{
    int i,j,index, nFuncs, iFirst;
    char       *dummy;
    sarray_t   *listSupp, *listVars, *listInsect;
    var_set_t  *vsTemp, *vsCurr;
    
    Ntk_Node_t *pn, *tmpn;
    Mva_Func_t *pMva;
    
    st_table     *stCurr, *stTemp, *stSupp;
    st_generator *gen;
    
    nFuncs     = sarray_n(funcs);
    listSupp   = sarray_alloc( st_table *,  nFuncs); /* common support nodes   */
    listVars   = sarray_alloc( var_set_t *, nFuncs); /* indices of these nodes */
    listInsect = sarray_alloc( int,         nFuncs); /* intersection nodes     */
    
    for ( i=start_index; i<nFuncs; ++i) {
        
        pn = sarray_fetch(Ntk_Node_t *, nodes, i);
        if ( pn == NIL(Ntk_Node_t) ) continue;
        
        pMva = sarray_fetch( Mva_Func_t *, funcs, i);
        if ( pMva == NULL ) continue;
        
        stSupp = SimpMvaComputeSupport( pMva, pInfo );
        
        if ( sarray_n(listSupp)==0 ) {
            /* the first set */
            sarray_insert_last( st_table *, listSupp, stSupp );
            
            vsTemp = var_set_new( nFuncs );
            var_set_set_elt( vsTemp, i );
            sarray_insert_last( var_set_t *, listVars, vsTemp );
            continue;
        }
        
        listInsect->num = 0;
        /* record all disjoint groups that intersect */
        for (j=0; j<sarray_n(listSupp); ++j) {
            
            stCurr = sarray_fetch(st_table *, listSupp, j);
            if (stCurr==NIL(st_table)) {
                continue;
            }
            st_foreach_item(stSupp, gen, (char **)&tmpn, NIL(char*)) {
                if (st_lookup(stCurr, (char *)tmpn, &dummy)) {
                    sarray_insert_last( int, listInsect, j );
                    st_free_gen( gen );  /* pre-emption: memory leak */
                    break;
                }
            }
        }
        if (sarray_n(listInsect) == 0) {
            /* no intersection; create a new support set */
            sarray_insert_last( st_table *, listSupp, stSupp );
            
            vsTemp = var_set_new(nFuncs);
            var_set_set_elt(vsTemp, i);
            sarray_insert_last( var_set_t *, listVars, vsTemp );
            continue;
        }
        else {
            
            /* combine all st_tables that intersects this *pn */
            iFirst = -1;
            for (j=0; j<sarray_n(listInsect); ++j) {
                
                index  = sarray_fetch(int, listInsect, j);
                stTemp = sarray_fetch(st_table *, listSupp, index);
                vsTemp = sarray_fetch(var_set_t *, listVars, index);
                
                if ( iFirst < 0 ) iFirst = index;
                
                /* reuse the first existing table and var_set */
                if ( j==0 ) {
                    stCurr = stTemp;
                    vsCurr = vsTemp;
                }
                else {
                    st_foreach_item(stTemp, gen, (char **)&tmpn, NIL(char*)) {
                        st_insert(stCurr, (char *)tmpn, NIL(char));
                    }
                    st_free_table(stTemp);
                    
                    /* combine all sets into one */
                    var_set_or(vsCurr, vsCurr, vsTemp);
                    var_set_free(vsTemp);
                }
                sarray_insert( st_table *,  listSupp, index, NIL(st_table) );
                sarray_insert( var_set_t *, listVars, index, NIL(var_set_t));
            }
            
            /* combine with the current support set */
            st_foreach_item(stSupp, gen, (char **)&tmpn, NIL(char *)) {
                st_insert(stCurr, (char *)tmpn, NIL(char));
            }
            var_set_set_elt(vsCurr, i);
            
            sarray_insert( st_table *,  listSupp, iFirst, stCurr);
            sarray_insert( var_set_t *, listVars, iFirst, vsCurr);
            
            st_free_table(stSupp); // new
        }
    }
    sarray_free(listInsect);
    
    /* wrap up listSupp array of arrays */
    for (i=0; i<sarray_n(listSupp); ++i) {
        
        stCurr = sarray_fetch(st_table *, listSupp, i);
        if (stCurr==NIL(st_table)) continue;
        st_free_table(stCurr);
    }
    sarray_free( listSupp );
    sarray_compact( listVars );
    
    return listVars;
}



/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Compute the image of a set from primary inputs to a local
  support.]

  Description [Compute the image of a set from primary inputs to a local
  support. Given an MDD (assume support variables are combinational inputs),
  compute the image in local input of node. Result is in multi-valued SOP
  format as a separate node. ]

  SideEffects []

  SeeAlso     []

******************************************************************************/
Ntk_Node_t *
SimpComputeImageIntern(
    Simp_Info_t *pInfo,
    Ntk_Node_t *pNode,
    DdNode     *bCare)
{
    int i,k,nValues, nFanins;
    sarray_t       *aFuncs, *aNodes;
    Mva_Func_t     *pMva;
    
    Ntk_Node_t     *pResult, *pFanin;
    Ntk_Pin_t      *pPin;
    Simp_Node_t    *simp_data;
    Mvc_Cover_t    *pMvcCare;
    Mvc_Data_t     *pMvcData;
    Vm_VarMap_t    *pVm;
    DdNode         **pbGlo, *bTemp;
    DdManager      *mg;
    
    mg = pInfo->ddmg;
    
    /* compute output cofactors */
    nFanins = Ntk_NodeReadFaninNum(pNode);
    aFuncs = sarray_alloc( Mva_Func_t *, nFanins );
    aNodes = sarray_alloc( Ntk_Node_t *, nFanins );
    Ntk_NodeReadFanins(pNode, (Ntk_Node_t **)(aNodes->space));
    aNodes->num = nFanins;
    
    Ntk_NodeForEachFaninWithIndex( pNode, pPin, pFanin, i ) {
        
        simp_data = Simp_DaemonGetNodeData(pFanin);
        nValues   = Ntk_NodeReadValueNum( pFanin );
        pMva      = Mva_FuncAlloc( mg, nValues );
        
        /* have to use global function here */
        pbGlo = Ntk_NodeGetFuncGlo(pFanin);
        
        for (k=0; k<nValues; ++k) {
            bTemp = Cudd_bddConstrain(mg, pbGlo[k], bCare);
            Cudd_Ref(bTemp);
            Mva_FuncReplaceIset( pMva, k, bTemp );
        }
        
        sarray_insert_last( Mva_Func_t *, aFuncs, pMva );
    }
    
    /* reset the start time */
    SimpTimeOutStartNode(pInfo, pInfo->time_left_node);
    
    /* allocate cover information (var mask etc.) */
    pVm = Ntk_NodeReadFuncVm( pNode );
    pMvcData = SimpAllocateMvcData( pNode );
    
    /* start recursive range computation */
    pMvcCare = SimpComputeRangeRecur(pInfo, &aFuncs, &aNodes, pVm, pMvcData, 0);
    
    /* de-allocate cover information */
    SimpFreeMvcData( pNode, pMvcData );
    
    
    /* NULL means full image */
    if (pMvcCare == NULL) {
        pResult = NULL;
    }
    else {
        pResult = SimpNodeCreateFromFanins(Ntk_NodeReadNetwork(pNode),aNodes);
        SimpNodeAssignMvc( pResult, pMvcCare );
    }
    
    sarray_free(aNodes);
    if (aFuncs != NIL(sarray_t)) {
        SimpMvaArrayFree( aFuncs, 0 );
    }
    
    return pResult;
}


/**Function********************************************************************

  Synopsis    [Recursive range computation.]

  Description [Recursive range computation. The function list and node list
  are given in terms of addresses of the pointers, because the pointer may get
  freed or changed.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
Mvc_Cover_t *
SimpComputeRangeRecur(
    Simp_Info_t   *pInfo,
    sarray_t     **func_list,
    sarray_t     **local_base,
    Vm_VarMap_t   *pVm,
    Mvc_Data_t    *pMvcData,
    int            start_index)
{
    DdManager       *mg;
    Mvc_Cover_t     *pImage, *pSuper, *pLit, *pTemp, *pTemp1;
    Mva_Func_t      *pMva;
    sarray_t        *listFuncs, *listNodes, *listSupp;
    var_set_t       *vsSupp;
    int             i, k, nConst, thisone, count, iSet;
    
    if (SimpTimeOutCheckNode(pInfo)) {
        return NULL;
    }

    mg = pInfo->ddmg;
    listFuncs = *func_list;
    listNodes = *local_base;
    
    /* Constant functions are removed from the list;
     * the corresponding mv variable is ANDed to the image cube.
     */
    pImage  = NULL;
    thisone = -1;
    count   = 0;
    
    for (i=start_index; i<sarray_n(listFuncs); ++i) {
        
        pMva = sarray_fetch( Mva_Func_t *, listFuncs, i);
        if ( pMva == NULL )
            continue;
        
        if ( Mva_FuncIsConstant( pMva, &nConst ) ) {
            
            Mva_FuncFree( pMva );
            sarray_insert( Mva_Func_t *, listFuncs, i, NULL );
            pLit  = SimpCubeLiteral( pInfo->pMvcMem, pVm, i, nConst );
            if (pImage==NULL) {
                pImage = pLit;
            }
            else {
                pTemp = Mvc_CoverBooleanAnd( pMvcData, pImage, pLit );
                Mvc_CoverFree(pImage);
                Mvc_CoverFree(pLit);
                pImage = pTemp;
            }
            continue;
        }
        count++;
        if (thisone == -1 ) {
            thisone = i; /* the first non-NIL function */
        }
    }
    
    /* if only one function left and not constant,
     * terminate the recursion and return the full image
     */
    if (count <= 1) {
        if (thisone>=0) {
            
            /* compute actual set of values, may not be whole set */
            pMva = sarray_fetch( Mva_Func_t *, listFuncs, thisone);
            pLit = SimpCubeLiteralImage ( pInfo->pMvcMem, pVm, mg, thisone, pMva);
            
            if (pLit) { /* Null means fullset and no need to intersect */
                pTemp = Mvc_CoverBooleanAnd( pMvcData, pImage, pLit );
                Mvc_CoverFree( pImage );
                Mvc_CoverFree( pLit );
                pImage = pTemp;
            }
            Mva_FuncFree( pMva ); pMva = NULL;
        }
        /* if nothing is left then return empty set */
        
        sarray_free(listFuncs); /* recursively generated */
        *func_list = NIL(sarray_t);
        return pImage;
    }
    
    /* partition the list of functions into disjoint supports;
       If there is only one function, no need to process; if there are two
       functions, call a special routine; else do output cofactoring with
       respect to the first function and recurse.
    */
    
    listSupp = SimpDisjointSupports(listNodes, listFuncs, start_index, pInfo);
    
    pSuper = NULL;
    for (iSet=0; iSet<sarray_n(listSupp); ++iSet) {
        
        vsSupp = sarray_fetch(var_set_t *, listSupp, iSet);
        k = var_set_n_elts(vsSupp);
        if (k==1) {
            /* stand alone node:
             *  still need to check if all phases of this node are reachable;
             *  if not, apply restrictions to the super_image.
             */
            pTemp = SimpRangeOne( pInfo, pVm, start_index, vsSupp, listFuncs);
        }
        else if (k==2) {
            pTemp = SimpRangeTwo( pInfo, pVm, pMvcData, start_index, vsSupp, listFuncs);
        }
        else {
            pTemp = SimpRangeCofact(pInfo, pVm, pMvcData, start_index, vsSupp,
                                    listFuncs, local_base );
        }
        var_set_free(vsSupp);
        if (pTemp) {
            if (pSuper) {
                pTemp1 = Mvc_CoverBooleanAnd( pMvcData, pSuper, pTemp );
                Mvc_CoverFree(pSuper);
                Mvc_CoverFree(pTemp);
                pSuper = pTemp1;
            } else {
                pSuper = pTemp;
            }
        }
    }
    sarray_free(listSupp);
    
    /* quick simplification to avoid huge image */
    if ( pSuper && Mvc_CoverReadCubeNum(pSuper) >= 300 ) {
        
        //printf("warning: image size larger than 300 cubes.\n");
        Mvc_CoverDist1Merge( pMvcData, pSuper );
    }
    
    if (pImage) {
        if (pSuper) {
            pTemp = Mvc_CoverBooleanAnd( pMvcData, pSuper, pImage );
            Mvc_CoverFree(pSuper);
            Mvc_CoverFree(pImage);
            pSuper = pTemp;
        }
        else {
            pSuper = pImage;
        }
    }
    
    return pSuper;
}


/**Function********************************************************************

  Synopsis    [Return the range of one function.]

  Description [Return the range of one function. The support set contains
  disjoint functional supports, in which only one function is left. Check if
  all output values of this node are reachable; if not, apply restrictions to
  the super_image]

  SideEffects []
  SeeAlso     []

******************************************************************************/
Mvc_Cover_t *
SimpRangeOne(
    Simp_Info_t *pInfo,
    Vm_VarMap_t *pVm,
    int         start_index,
    var_set_t   *supset,
    sarray_t     *listFuncs )
{
    int i;
    Mva_Func_t  *pMva = NULL;
    
    /* retrieve the one and only left */
    for ( i = start_index; i < sarray_n(listFuncs); ++i ) {
        if (!var_set_get_elt(supset,i)) continue;
        pMva = sarray_fetch( Mva_Func_t *, listFuncs, i );
        break;
    }
    
    assert( pMva );
    return ( SimpCubeLiteralImage ( pInfo->pMvcMem, pVm, pInfo->ddmg, i, pMva ) );
    /* NULL means full-set here */
}

/**Function********************************************************************

  Synopsis    [Return the range of two function.]

  Description [Return the range of two function. The support set contains
  disjoint functional supports, in which only two functions are left. Perform
  output cofactoring on these two functions.]

  SideEffects []
  SeeAlso     []

******************************************************************************/
Mvc_Cover_t *
SimpRangeTwo(
    Simp_Info_t *pInfo,
    Vm_VarMap_t *pVm,
    Mvc_Data_t  *pMvcData,
    int         start_index,
    var_set_t   *supset,
    sarray_t    *listFuncs )
{
    int i, flag;
    int iVar1, iVar2, iValue, count;
    Mva_Func_t  *pMva, *pFun1, *pFun2, *pCof;
    DdNode      *bFunc;
    DdManager   *mg;
    Mvc_Cover_t *lit1, *lit2;
    Mvc_Cover_t *temp, *temp2, *image;
    
    flag = -1;
    pFun1 = NULL;
    pFun2 = NULL;
    iVar1 = iVar2 = 0;
    mg = pInfo->ddmg;
    
    /* retrieve the two functions according to var_set */
    for (i=start_index; i<sarray_n(listFuncs); ++i) {
        if (!var_set_get_elt(supset, i)) continue;
        
        pMva = sarray_fetch( Mva_Func_t *, listFuncs, i);
        if (pMva == NULL) continue;
        if (flag==-1) {
            pFun1=pMva; iVar1=i; flag=i;
        } else {
            pFun2=pMva; iVar2=i;
        }
    }
    
    /* do output cofactoring of f2 wrt. f1 */
    count = Mva_FuncReadIsetNum( pFun1 );
    image = NULL;
    
    for ( iValue=0; iValue<count; ++iValue ) {
        
        /* iVar1'th var taking iValue'th value */
        bFunc = pFun1->pbFuncs[iValue];
        
        /* if this component is Null, it means this value is
           not reachable and no need to explore further */
        if ( bFunc == NULL || bFunc == Cudd_ReadLogicZero(mg) ) {
            continue;
        }
        
        lit1 = SimpCubeLiteral( pInfo->pMvcMem, pVm, iVar1, iValue );
        pCof = Mva_FuncCofactor( pFun2, bFunc );
        
        /* in case of non-constant, need to test multiple values */
        lit2 = SimpCubeLiteralImage ( pInfo->pMvcMem, pVm, mg, iVar2, pCof );
        
        if (lit2) {
            temp = Mvc_CoverBooleanAnd( pMvcData, lit1, lit2 );
            Mvc_CoverFree(lit1);
            Mvc_CoverFree(lit2);
        }
        else { /* Full set for lit2 */
            temp = lit1;
        }
        
        Mva_FuncFree( pCof );
        if (image) {
            temp2 = Mvc_CoverBooleanOr(image, temp);
            Mvc_CoverFree(image);
            Mvc_CoverFree(temp);
            image = temp2;
        }
        else {
            image = temp;
        }
    }
    
    return image;
}

/**Function********************************************************************
  Synopsis    []
  Description [Return the range of a list of functions by output cofacting.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Mvc_Cover_t *
SimpRangeCofact(
    Simp_Info_t *pInfo,
    Vm_VarMap_t *pVm,
    Mvc_Data_t  *pMvcData, 
    int         start_index,
    var_set_t   *supset,
    sarray_t    *listFuncs,
    sarray_t    **base )
{
    int           i, k;
    int           iStart, iFlag, iAccl, nComp;
    Mvc_Cover_t  *pMvcImage, *pMvcPart, *pMvcTemp;
    DdManager    *mg;
    Mva_Func_t   *aMvfThis, *aMvfFunc, *aMvfConst, *aMvfCof;
    sarray_t     **llRange;
    
    mg = pInfo->ddmg;
    iStart = iFlag = -1;
    iAccl = 0;
    
    /* compute output cofactoring */
    for (i=start_index; i<sarray_n(listFuncs); ++i) {
        
        aMvfThis = sarray_fetch( Mva_Func_t *, listFuncs, i);
        
        if (!var_set_get_elt(supset,i) || aMvfThis == NULL) {
            if (iFlag==-1) {
                iAccl++;
            }
            else {
                /* fill empty slots with NIL Mvf pointers
                 *  nComp has already been set by now.
                 */
                for (k=0; k<nComp; ++k) {
                    if (llRange[k]) {
                        sarray_insert_last( Mva_Func_t *, llRange[k], NULL);
                    }
                }
            }
            continue;
        }
        
        /* 'i' is in this partition */
        if (iFlag==-1) {
            /* this is the first special one */
            iFlag = i;
            aMvfFunc = aMvfThis;
            nComp = Mva_FuncReadIsetNum( aMvfThis );
            llRange = ALLOC(sarray_t *, nComp);
            
            if (iStart == -1) {
                iStart = start_index+iAccl;
            }
            for (k=0; k<nComp; ++k) {
                
                /* if this component is Null, it means this value is
                   not reachable and no need to explore further (1/16/2002) */
                if ( (aMvfFunc->pbFuncs[k] == NULL) ||
                      aMvfFunc->pbFuncs[k] == Cudd_ReadLogicZero(mg) ) {
                    llRange[k] = NULL;
                }
                else {
                    llRange[k] = sarray_alloc( Mva_Func_t *, array_n(listFuncs));
                    /* Hack some fake array elements */
                    llRange[k]->num = iStart;
                    
                    /* insert constant MDD's [0-k] */
                    aMvfConst = Mva_FuncAllocConstant( mg, nComp, k );
                    sarray_insert_last( Mva_Func_t *, llRange[k], aMvfConst );
                }
            }
        }
        else {
            /* Cofactoring is done here (wrt. the first) */
            for (k=0; k<nComp; ++k) {
                
                /* Check for only Non-Null components (1/16/2002) */
                if ( llRange[k] ) {
                    
                    aMvfCof = Mva_FuncCofactor( aMvfThis, aMvfFunc->pbFuncs[k] );
                    sarray_insert_last( Mva_Func_t *, llRange[k], aMvfCof );
                }
            }
        }
    }
    
    /* OR every thing together */
    pMvcImage = NULL;
    for (i=0; i<nComp; ++i) {
        
        if (llRange[i]) {
            
            pMvcPart = SimpComputeRangeRecur(pInfo,&llRange[i],base,pVm,pMvcData,iStart);
            if (pMvcPart) {
                
                if (pMvcImage) {
                    pMvcTemp = Mvc_CoverBooleanOr(pMvcImage, pMvcPart);
                    Mvc_CoverFree(pMvcPart);
                    Mvc_CoverFree(pMvcImage);
                    pMvcImage = pMvcTemp;
                }
                else {
                    pMvcImage = pMvcPart;
                }
            }
        }
    }
    
    SimpMvaArrayListFree( llRange, nComp, start_index);
    
    return pMvcImage;
}

/**Function********************************************************************
  Synopsis    [Returns a literal cube]
  Description [Given a var map, find the variable/column with `index', and
  return a single literal cover with a single value `value'.]

  SideEffects []
  SeeAlso     []
******************************************************************************/
Mvc_Cover_t *
SimpCubeLiteral(
    Mvc_Manager_t * pMem,
    Vm_VarMap_t   * pVm,
    int             index,
    int             value) 
{
    int  base,i,size;
    Mvc_Cover_t * pMvcLit;
    Mvc_Cube_t  * pMvcCube;
    
    pMvcLit = Mvc_CoverAlloc( pMem, Vm_VarMapReadValuesInNum(pVm) );
    pMvcCube = Mvc_CubeAlloc( pMvcLit );

    base = Vm_VarMapReadValuesFirst( pVm,index );
    size = Vm_VarMapReadValues( pVm,index );
    Mvc_CubeBitFill( pMvcCube );
    
    for (i=0; i<size; ++i) {
        if (i != value)
            Mvc_CubeBitRemove( pMvcCube, (base+i) );
    }
    Mvc_CoverAddCubeTail ( pMvcLit, pMvcCube );
    
    return pMvcLit;
}

/**Function********************************************************************

  Synopsis    [Returns a literal cube]
  Description [Given a var map, find the variable/column with `index', and
  return a single literal cover with a a range of values contained in the
  array `value_range'.]

  SideEffects []
  SeeAlso     []

******************************************************************************/
Mvc_Cover_t *
SimpCubeLiteralRange(
    Mvc_Manager_t * pMem,
    Vm_VarMap_t   * pVm,
    int             index,
    sarray_t      * aRange) 
{
    int  base,i,size,val;
    Mvc_Cover_t * pMvcLit;
    Mvc_Cube_t  * pMvcCube;
    
    /* empty range */
    if (sarray_n(aRange)==0) {
        pMvcLit = Mvc_CoverAlloc( pMem, Vm_VarMapReadValuesInNum(pVm) );
        return pMvcLit;
    }
    
    pMvcLit = Mvc_CoverAlloc( pMem, Vm_VarMapReadValuesInNum(pVm) );
    pMvcCube = Mvc_CubeAlloc( pMvcLit );

    base = Vm_VarMapReadValuesFirst( pVm,index );
    size = Vm_VarMapReadValues( pVm,index );
    Mvc_CubeBitFill( pMvcCube );
    
    for (i=0; i<size; ++i) {
        Mvc_CubeBitRemove( pMvcCube, base+i );
    }
    for (i=0; i<sarray_n(aRange); ++i) {
        val = sarray_fetch(int, aRange, i);
        Mvc_CubeBitInsert( pMvcCube, (base+val) );
    }
    Mvc_CoverAddCubeTail ( pMvcLit, pMvcCube );
    
    return pMvcLit;
}

/**Function********************************************************************

  Synopsis    [Compute the literal cube from a Mvf MDD array.]

  Description [The values in the literal cube corresponds to the non-empty
  components (parts) in the Mvf MDD array. Returns Mvc_Cover_t * of the
  literal if there is at least one empty part; returns NULL if no empty part
  (full-set).]

  SideEffects []
  SeeAlso     []

******************************************************************************/
Mvc_Cover_t *
SimpCubeLiteralImage(
    Mvc_Manager_t * pMem,
    Vm_VarMap_t   * pVm,
    DdManager     * ddmg,
    int             index,
    Mva_Func_t    * pMva) 
{
    int i, empty_flag;
    sarray_t     * aRanges;
    Mvc_Cover_t * pMvcLit;
    
    empty_flag = 0;
    aRanges = sarray_alloc( int, pMva->nIsets );
    
    for (i=0; i < pMva->nIsets; ++i) {
        
        if ( pMva->pbFuncs[i] &&
             pMva->pbFuncs[i] != Cudd_ReadLogicZero(ddmg) ) {
            sarray_insert_last( int, aRanges, i );
        }
        else {
            empty_flag = 1;
        }
    }
    if (empty_flag) {
        pMvcLit = SimpCubeLiteralRange( pMem, pVm, index, aRanges );
    }
    else {
        pMvcLit = NULL;
    }
    
    sarray_free(aRanges);
    return pMvcLit;
}

/**Function********************************************************************
  Synopsis    [Return TRUE if the direct fanins are all CI's.]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
bool
SimpNodeCheckSupport (
    Ntk_Node_t *node) 
{

    Ntk_Node_t *pFanin;
    Ntk_Pin_t  *pPin;
    Ntk_NodeForEachFanin(node, pPin, pFanin) {
        if (!Ntk_NodeIsCi(pFanin)) {
            return FALSE;
        }
    }
    return TRUE;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Mvc_Data_t *
SimpAllocateMvcData( Ntk_Node_t *pNode )
{
    int           i, nVals;
    Vm_VarMap_t  *pVm;
    Cvr_Cover_t  *pCvr;
    Mvc_Cover_t  **pIsets;
    
    pVm   = Ntk_NodeReadFuncVm( pNode );
    pCvr  = Ntk_NodeReadFuncCvr( pNode );
    nVals = Vm_VarMapReadValuesOutNum( pVm );
    pIsets = Cvr_CoverReadIsets( pCvr );
    
    for ( i = 0; i < nVals; ++i ) {
        if ( pIsets[i] )
            return Mvc_CoverDataAlloc( pVm, pIsets[i] );
    }
    
    return NULL;
}

void
SimpFreeMvcData( Ntk_Node_t *pNode, Mvc_Data_t *pMvcData )
{
    int           i, nVals;
    Vm_VarMap_t  *pVm;
    Cvr_Cover_t  *pCvr;
    Mvc_Cover_t  **pIsets;
    
    pVm   = Ntk_NodeReadFuncVm( pNode );
    pCvr  = Ntk_NodeReadFuncCvr( pNode );
    nVals = Vm_VarMapReadValuesOutNum( pVm );
    pIsets = Cvr_CoverReadIsets( pCvr );
    
    for ( i = 0; i < nVals; ++i ) {
        if ( pIsets[i] ) {
            Mvc_CoverDataFree( pMvcData, pIsets[i] );
            break;
        }
    }
    return;
}


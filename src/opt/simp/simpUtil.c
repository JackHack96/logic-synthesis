/**CFile****************************************************************

  FileName    [simpUtil.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Utilities for the simplify package]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: simpUtil.c,v 1.10 2003/05/27 23:16:18 alanmi Exp $]

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

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
bool
SimpNodeIsHardCase( Ntk_Node_t *pNode ) 
{
    int          nVars, nVals;
    Vm_VarMap_t *pVm;
    Cvr_Cover_t *pCf;
    
    pVm = Ntk_NodeReadFuncVm( pNode );
    pCf = Ntk_NodeGetFuncCvr( pNode );
    nVars = Vm_VarMapReadVarsInNum( pVm );
    nVals = Vm_VarMapReadValuesOutNum( pVm );
    
    /* number of inputs more than the literal count,
       indicates it is likely an Ahille's hill function */
    return ( nVars >= 3 && ((nVals-1) * nVars) >= Cvr_CoverReadLitFacNum( pCf ) );
}


/**Function********************************************************************
  Synopsis    [return a new node whose cvr is the complement of the input one]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ntk_Node_t  *SimpNodeComplement( Ntk_Node_t *pNode )
{
    int i;

    Ntk_Node_t  *pCmpl;
    Cvr_Cover_t *pCvr, *pCvrCmpl;
    Mvc_Cover_t **pIsets, **pIsetsCmpl;
    Vm_VarMap_t *pVm;
    
    if ( pNode==NULL ) return NULL;
    pCvr = Ntk_NodeGetFuncCvr( pNode );
    pVm  = Ntk_NodeGetFuncVm( pNode );
    assert ( Vm_VarMapReadValuesOutNum(pVm) == 2 );
    
    /* clone the node structure */
    pCmpl = SimpNodeClone( pNode );
    
    /* create the complement cover */
    pIsets = Cvr_CoverReadIsets( pCvr );
    pIsetsCmpl = ALLOC( Mvc_Cover_t *, 2 );
    memset( pIsetsCmpl, 0, 2 * sizeof(Mvc_Cover_t *));
    for ( i=0; i<2; ++i ) {
        if ( pIsets[i] )
            pIsetsCmpl[i] = Cvr_CoverComplement( pVm, pIsets[i] );
    }
    
    /* insert into the new node */
    pCvrCmpl = Cvr_CoverCreate(Ntk_NodeReadFuncVm(pNode), pIsetsCmpl);
    Ntk_NodeWriteFuncCvr( pCmpl, pCvrCmpl );
    
    return pCmpl;
}


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ntk_Node_t  *SimpNodeOr( Ntk_Node_t *pNode1, Ntk_Node_t *pNode2 ) 
{
    int *pMap1, *pMap2;
    Ntk_Node_t  *pNodeNew;
    Cvr_Cover_t *pCvr1, *pCvr2, *pCvrNew1, *pCvrNew2, *pCvrRes;

    Vm_VarMap_t *pVm1, *pVm2, *pVmNew;
    
    if ( pNode1==NULL || pNode2==NULL ) return NULL;
    
    pVm1 = Ntk_NodeReadFuncVm( pNode1 );
    pVm2 = Ntk_NodeReadFuncVm( pNode1 );
    assert( Vm_VarMapReadValuesOutNum( pVm1 ) == 2 &&
            Vm_VarMapReadValuesOutNum( pVm2 ) == 2 );
    
    /* derive the new node */
    pMap1 = ALLOC(int, Ntk_NodeReadFaninNum(pNode1));
    pMap2 = ALLOC(int, Ntk_NodeReadFaninNum(pNode2));
    pNodeNew = Ntk_NodeMakeCommonBase(pNode1, pNode2, pMap1, pMap2);
    pVmNew   = Ntk_NodeGetFuncVm(pNodeNew);
    
    /* adjust the covers */
    pCvr1    = Ntk_NodeGetFuncCvr(pNode1);
    pCvrNew1 = Cvr_CoverCreateAdjust(pCvr1, pVmNew, pMap1);
    pCvr2    = Ntk_NodeGetFuncCvr(pNode2);
    pCvrNew2 = Cvr_CoverCreateAdjust(pCvr2, pVmNew, pMap2);
    FREE(pMap1);
    FREE(pMap2);
    
    /* performing disjunction */
    pCvrRes = Cvr_CoverOr( pCvrNew1, pCvrNew2 );
    Cvr_CoverFree( pCvrNew1 );
    Cvr_CoverFree( pCvrNew2 );
    
    /* clone the node structure */
    Ntk_NodeWriteFuncCvr( pNodeNew, pCvrRes );
    
    return pNodeNew;
}


/**Function********************************************************************
  Synopsis    [return a new node whose cvr is the complement of the input one]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Ntk_Node_t  *SimpNodeClone( Ntk_Node_t *pNode ) 
{
    Ntk_Node_t  *pNodeNew, *pFanin;
    Ntk_Pin_t   *pPin;
    Vm_VarMap_t *pVm;
    
    pVm  = Ntk_NodeGetFuncVm( pNode );
    pNodeNew = Ntk_NodeCreate( Ntk_NodeReadNetwork(pNode), NULL, MV_NODE_INT,
                               Ntk_NodeReadValueNum( pNode ) );

    Ntk_NodeWriteFuncVm ( pNodeNew, pVm );  
    Ntk_NodeForEachFanin ( pNode, pPin, pFanin ) {
        Ntk_NodeAddFanin ( pNodeNew, pFanin );
    }
    return pNodeNew;
}


/**Function********************************************************************
  Synopsis    [replace old node with new one]
  Description [take care of the global BDD if there are any]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
SimpNodeReplace( Ntk_Node_t *pNode, Ntk_Node_t *pNodeNew) 
{
    DdNode **pbFuncs;
    pbFuncs = Ntk_NodeReadFuncGlo( pNode );
    if ( pbFuncs ) {
        Ntk_NodeWriteFuncGlo( pNode, NULL );
        Ntk_NodeReplace( pNode, pNodeNew );
        Ntk_NodeWriteFuncGlo( pNode, pbFuncs );
    }
    else {
        Ntk_NodeReplace( pNode, pNodeNew );
    }
}


/**Function********************************************************************
  Synopsis    [return 1 if the two nodes have the same fanins]
  Description [assume both fanins are ordered]
  SideEffects []
  SeeAlso     []
******************************************************************************/
bool
SimpNodeCompareFanin( Ntk_Node_t *pNode1, Ntk_Node_t *pNode2 )
{
    Ntk_Pin_t *pPin1, *pPin2;
    
    if ( Ntk_NodeReadFaninNum( pNode1 ) !=
         Ntk_NodeReadFaninNum( pNode2 ) )
        return FALSE;
    
    pPin1 = Ntk_NodeReadFaninPinHead( pNode1 );
    pPin2 = Ntk_NodeReadFaninPinHead( pNode2 );
    
    do {
        if ( Ntk_PinReadNode(pPin1) !=
             Ntk_PinReadNode(pPin2) ) {
            return FALSE;
        }
        pPin1 = Ntk_PinReadNext(pPin1);
        pPin2 = Ntk_PinReadNext(pPin2);
    }
    while ( pPin1 && pPin2 );
    
    /* if one list rans out earlier return 0 */
    if ( (pPin1 && Ntk_PinReadNode(pPin1) ) ||
         (pPin2 && Ntk_PinReadNode(pPin2) ) ) {
        return FALSE;
    }
    
    return TRUE;
}

/**Function********************************************************************

  Synopsis    [Start time recording for a node.]

  Description [Start time recording for a node. Store the current time in
  pInfo->timeout_start_node.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
void
SimpTimeOutStartNode(
    Simp_Info_t *pInfo,
    long         time_left)
{
    pInfo->time_left_node  = time_left;
    pInfo->time_start_node = clock();
    return;
}


/**Function********************************************************************

  Synopsis    [Check if timeout has occured since last time this routine is
  called.]

  Description [Check if timeout has occured since last time this routine is
  called. Only checks the time left for each node.]

  SideEffects [Update the new time_left.]

  SeeAlso     []

******************************************************************************/
bool
SimpTimeOutCheckNode(
    Simp_Info_t *pInfo)
{
    pInfo->time_left_node -= clock() - pInfo->time_start_node;
    pInfo->time_start_node = clock();
    return (pInfo->time_left_node <=0);
}


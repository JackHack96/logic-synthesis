/**CFile****************************************************************

  FileName    [ntkApi.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    []

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkApi.c,v 1.39 2005/01/23 07:11:41 alanmi Exp $]

***********************************************************************/

#include "ntkInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Basic pin APIs]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Pin_t *  Ntk_PinReadNext( Ntk_Pin_t * pPin ) { return pPin->pNext; }
Ntk_Pin_t *  Ntk_PinReadPrev( Ntk_Pin_t * pPin ) { return pPin->pPrev; }
Ntk_Pin_t *  Ntk_PinReadLink( Ntk_Pin_t * pPin ) { return pPin->pLink; }
Ntk_Node_t * Ntk_PinReadNode( Ntk_Pin_t * pPin ) { return pPin->pNode; }

/**Function*************************************************************

  Synopsis    [Basic latch APIs]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Latch_t *   Ntk_LatchReadNext  ( Ntk_Latch_t * pLatch ) { return pLatch->pNext;  }
Ntk_Latch_t *   Ntk_LatchReadPrev  ( Ntk_Latch_t * pLatch ) { return pLatch->pPrev;  }
int             Ntk_LatchReadReset ( Ntk_Latch_t * pLatch ) { return pLatch->Reset;  }
Ntk_Node_t *    Ntk_LatchReadInput ( Ntk_Latch_t * pLatch ) { return pLatch->pInput; }
Ntk_Node_t *    Ntk_LatchReadOutput( Ntk_Latch_t * pLatch ) { return pLatch->pOutput;}
Ntk_Node_t *    Ntk_LatchReadNode  ( Ntk_Latch_t * pLatch ) { return pLatch->pNode;  }
Ntk_Network_t * Ntk_LatchReadNet   ( Ntk_Latch_t * pLatch ) { return pLatch->pNet;   }
char *          Ntk_LatchReadData  ( Ntk_Latch_t * pLatch ) { return pLatch->pData;  }

/**Function*************************************************************

  Synopsis    [Basic node APIs]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t *      Ntk_NodeReadNext         ( Ntk_Node_t * pNode ) { return pNode->pNext;  }
Ntk_Node_t *      Ntk_NodeReadPrev         ( Ntk_Node_t * pNode ) { return pNode->pPrev;  }
Ntk_Node_t *      Ntk_NodeReadOrder        ( Ntk_Node_t * pNode ) { return pNode->pOrder; }
int               Ntk_NodeReadId           ( Ntk_Node_t * pNode ) { return pNode->Id;     }
char *            Ntk_NodeReadName         ( Ntk_Node_t * pNode ) { return pNode->pName;  }
Ntk_NodeType_t    Ntk_NodeReadType         ( Ntk_Node_t * pNode ) { return pNode->Type;   }
Ntk_NodeSubtype_t Ntk_NodeReadSubtype      ( Ntk_Node_t * pNode ) { return pNode->Subtype;}
int               Ntk_NodeReadLevel        ( Ntk_Node_t * pNode ) { return pNode->Level;  }
int               Ntk_NodeReadValueNum     ( Ntk_Node_t * pNode ) { return pNode->nValues;}
Ntk_Pin_t *       Ntk_NodeReadFaninPinHead ( Ntk_Node_t * pNode ) { return pNode->lFanins.pHead;  }
Ntk_Pin_t *       Ntk_NodeReadFaninPinTail ( Ntk_Node_t * pNode ) { return pNode->lFanins.pTail;  }
int               Ntk_NodeReadFaninNum     ( Ntk_Node_t * pNode ) { return pNode->lFanins.nItems; }
Ntk_Pin_t *       Ntk_NodeReadFanoutPinHead( Ntk_Node_t * pNode ) { return pNode->lFanouts.pHead; }
Ntk_Pin_t *       Ntk_NodeReadFanoutPinTail( Ntk_Node_t * pNode ) { return pNode->lFanouts.pTail; }
int               Ntk_NodeReadFanoutNum    ( Ntk_Node_t * pNode ) { return pNode->lFanouts.nItems;}
char *            Ntk_NodeReadData         ( Ntk_Node_t * pNode ) { return pNode->pData;  }
Ntk_NodeBinding_t*Ntk_NodeReadMap          ( Ntk_Node_t * pNode ) { return pNode->pMap;   }
Ntk_Network_t *   Ntk_NodeReadNetwork      ( Ntk_Node_t * pNode ) { return pNode->pNet;   }
Fnc_Function_t *  Ntk_NodeReadFunc         ( Ntk_Node_t * pNode ) { return pNode->pF;     }
Fnc_Global_t *    Ntk_NodeReadFuncGlobal   ( Ntk_Node_t * pNode ) { return pNode->pG? pNode->pG: (pNode->pG = Fnc_GlobalAlloc(pNode->nValues)); }
double            Ntk_NodeReadArrTimeRise  ( Ntk_Node_t * pNode ) { return pNode->dArrTimeRise;  }
double            Ntk_NodeReadArrTimeFall  ( Ntk_Node_t * pNode ) { return pNode->dArrTimeFall;  }
FILE *            Ntk_NodeReadMvsisErr     ( Ntk_Node_t * pNode ) { return Ntk_NetworkReadMvsisErr(pNode->pNet); }
FILE *            Ntk_NodeReadMvsisOut     ( Ntk_Node_t * pNode ) { return Ntk_NetworkReadMvsisOut(pNode->pNet); }

/**Function*************************************************************

  Synopsis    [Node functionality APIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vm_VarMap_t *     Ntk_NodeReadFuncVm       ( Ntk_Node_t * pNode ) { return Fnc_FunctionReadVm( pNode->pF ); }
Cvr_Cover_t *     Ntk_NodeReadFuncCvr      ( Ntk_Node_t * pNode ) { return Fnc_FunctionReadCvr( pNode->pF ); }
Mvr_Relation_t *  Ntk_NodeReadFuncMvr      ( Ntk_Node_t * pNode ) { return Fnc_FunctionReadMvr( pNode->pF ); }
DdNode **         Ntk_NodeReadFuncGlo      ( Ntk_Node_t * pNode ) { return Fnc_FunctionReadGlo( pNode->pF ); }

Vm_VarMap_t *     Ntk_NodeGetFuncVm        ( Ntk_Node_t * pNode ) { return Fnc_FunctionGetVm( pNode->pF ); }
Cvr_Cover_t *     Ntk_NodeGetFuncCvr       ( Ntk_Node_t * pNode ) { return Fnc_FunctionGetCvr( pNode->pNet->pMan, pNode->pF ); }
Mvr_Relation_t *  Ntk_NodeGetFuncMvr       ( Ntk_Node_t * pNode ) { return Fnc_FunctionGetMvr( pNode->pNet->pMan, pNode->pF ); }
DdNode **         Ntk_NodeGetFuncGlo       ( Ntk_Node_t * pNode ) { return Fnc_FunctionGetGlo( pNode->pNet->pMan, pNode->pF ); }

void              Ntk_NodeWriteFuncVm      ( Ntk_Node_t * pNode, Vm_VarMap_t * pVm )     { Fnc_FunctionWriteVm( pNode->pF, pVm ); }
void              Ntk_NodeWriteFuncCvr     ( Ntk_Node_t * pNode, Cvr_Cover_t * pCvr )    { Fnc_FunctionWriteCvr( pNode->pF, pCvr ); }
void              Ntk_NodeWriteFuncMvr     ( Ntk_Node_t * pNode, Mvr_Relation_t * pMvr ) { Fnc_FunctionWriteMvr( pNode->pF, pMvr ); }
void              Ntk_NodeWriteFuncGlo     ( Ntk_Node_t * pNode, DdNode ** pGlo )        { Fnc_FunctionWriteGlo( pNode->pF, pGlo ); }

void              Ntk_NodeSetFuncVm        ( Ntk_Node_t * pNode, Vm_VarMap_t * pVm )     { Fnc_FunctionSetVm( pNode->pF, pVm ); }
void              Ntk_NodeSetFuncCvr       ( Ntk_Node_t * pNode, Cvr_Cover_t * pCvr )    { Fnc_FunctionSetCvr( pNode->pF, pCvr ); }
void              Ntk_NodeSetFuncMvr       ( Ntk_Node_t * pNode, Mvr_Relation_t * pMvr ) { Fnc_FunctionSetMvr( pNode->pF, pMvr ); }
void              Ntk_NodeSetFuncGlo       ( Ntk_Node_t * pNode, DdNode ** pGlo )        { Fnc_FunctionSetGlo( pNode->pF, pGlo ); }

void              Ntk_NodeFreeFuncVm ( Ntk_Node_t * pNode ) { Fnc_FunctionFreeVm( pNode->pF ); };
void              Ntk_NodeFreeFuncCvr( Ntk_Node_t * pNode ) { Fnc_FunctionFreeCvr( pNode->pF ); };
void              Ntk_NodeFreeFuncMvr( Ntk_Node_t * pNode ) { Fnc_FunctionFreeMvr( pNode->pF ); };
void              Ntk_NodeFreeFuncGlo( Ntk_Node_t * pNode ) { Fnc_FunctionFreeGlo( pNode->pF ); };

Fnc_Manager_t *   Ntk_NodeReadMan   ( Ntk_Node_t * pNode ) { return Ntk_NetworkReadMan   ( pNode->pNet ); };
Mvr_Manager_t *   Ntk_NodeReadManMvr( Ntk_Node_t * pNode ) { return Ntk_NetworkReadManMvr( pNode->pNet ); };
Vm_Manager_t *    Ntk_NodeReadManVm ( Ntk_Node_t * pNode ) { return Ntk_NetworkReadManVm ( pNode->pNet ); };
Vmx_Manager_t *   Ntk_NodeReadManVmx( Ntk_Node_t * pNode ) { return Ntk_NetworkReadManVmx( pNode->pNet ); };
Mvc_Manager_t *   Ntk_NodeReadManMvc( Ntk_Node_t * pNode ) { return Ntk_NetworkReadManMvc( pNode->pNet ); };

/**Function*************************************************************

  Synopsis    [Global functionality APIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdManager *       Ntk_NodeReadFuncDd( Ntk_Node_t * pNode )                      { return Fnc_GlobalReadDd( Ntk_NodeReadFuncGlobal(pNode) ); }
void              Ntk_NodeWriteFuncDd( Ntk_Node_t * pNode, DdManager * dd )     {        Fnc_GlobalWriteDd( Ntk_NodeReadFuncGlobal(pNode), dd ); }
DdNode **         Ntk_NodeReadFuncGlob( Ntk_Node_t * pNode )                     { return Fnc_GlobalReadGlo( Ntk_NodeReadFuncGlobal(pNode) ); }
void              Ntk_NodeWriteFuncGlob( Ntk_Node_t * pNode, DdNode ** pbGlo )   {        Fnc_GlobalWriteGlo( Ntk_NodeReadFuncGlobal(pNode), pbGlo ); }
void              Ntk_NodeDerefFuncGlob( Ntk_Node_t * pNode )                    {        Fnc_GlobalDerefGlo( Ntk_NodeReadFuncGlobal(pNode) ); }
DdNode **         Ntk_NodeReadFuncGlobZ( Ntk_Node_t * pNode )                    { return Fnc_GlobalReadGloZ( Ntk_NodeReadFuncGlobal(pNode) ); }
void              Ntk_NodeWriteFuncGlobZ( Ntk_Node_t * pNode, DdNode ** pbGloZ ) {        Fnc_GlobalWriteGloZ( Ntk_NodeReadFuncGlobal(pNode), pbGloZ ); }
void              Ntk_NodeDerefFuncGlobZ( Ntk_Node_t * pNode )                   {        Fnc_GlobalDerefGloZ( Ntk_NodeReadFuncGlobal(pNode) ); }
DdNode **         Ntk_NodeReadFuncGlobS( Ntk_Node_t * pNode )                    { return Fnc_GlobalReadGloS( Ntk_NodeReadFuncGlobal(pNode) ); }
void              Ntk_NodeWriteFuncGlobS( Ntk_Node_t * pNode, DdNode ** pbGloS ) {        Fnc_GlobalWriteGloS( Ntk_NodeReadFuncGlobal(pNode), pbGloS ); }
void              Ntk_NodeDerefFuncGlobS( Ntk_Node_t * pNode )                   {        Fnc_GlobalDerefGloS( Ntk_NodeReadFuncGlobal(pNode) ); }
bool              Ntk_NodeReadFuncNonDet( Ntk_Node_t * pNode )                  { return Fnc_GlobalReadNonDet( Ntk_NodeReadFuncGlobal(pNode) ); }
void              Ntk_NodeWriteFuncNonDet( Ntk_Node_t * pNode, bool fNonDet )   {        Fnc_GlobalWriteNonDet( Ntk_NodeReadFuncGlobal(pNode), fNonDet ); }
int               Ntk_NodeReadFuncNumber( Ntk_Node_t * pNode )                  { return Fnc_GlobalReadNumber( Ntk_NodeReadFuncGlobal(pNode) ); }
void              Ntk_NodeWriteFuncNumber( Ntk_Node_t * pNode, int nNumber )    {        Fnc_GlobalWriteNumber( Ntk_NodeReadFuncGlobal(pNode), nNumber ); }

/**Function*************************************************************

  Synopsis    [Global functionality APIs (binary).]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode *          Ntk_NodeReadFuncBinGlo( Ntk_Node_t * pNode )                     { return Fnc_GlobalReadBinGlo( Ntk_NodeReadFuncGlobal(pNode) ); }
void              Ntk_NodeWriteFuncBinGlo( Ntk_Node_t * pNode, DdNode * bGlo )     {        Fnc_GlobalWriteBinGlo( Ntk_NodeReadFuncGlobal(pNode), bGlo ); }
void              Ntk_NodeDerefFuncBinGlo( Ntk_Node_t * pNode )                    {        Fnc_GlobalDerefBinGlo( Ntk_NodeReadFuncGlobal(pNode) ); }
DdNode *          Ntk_NodeReadFuncBinGloZ( Ntk_Node_t * pNode )                    { return Fnc_GlobalReadBinGloZ( Ntk_NodeReadFuncGlobal(pNode) ); }
void              Ntk_NodeWriteFuncBinGloZ( Ntk_Node_t * pNode, DdNode * bGloZ )   {        Fnc_GlobalWriteBinGloZ( Ntk_NodeReadFuncGlobal(pNode), bGloZ ); }
void              Ntk_NodeDerefFuncBinGloZ( Ntk_Node_t * pNode )                   {        Fnc_GlobalDerefBinGloZ( Ntk_NodeReadFuncGlobal(pNode) ); }
DdNode *          Ntk_NodeReadFuncBinGloS( Ntk_Node_t * pNode )                    { return Fnc_GlobalReadBinGloS( Ntk_NodeReadFuncGlobal(pNode) ); }
void              Ntk_NodeWriteFuncBinGloS( Ntk_Node_t * pNode, DdNode * bGloS )   {        Fnc_GlobalWriteBinGloS( Ntk_NodeReadFuncGlobal(pNode), bGloS ); }
void              Ntk_NodeDerefFuncBinGloS( Ntk_Node_t * pNode )                   {        Fnc_GlobalDerefBinGloS( Ntk_NodeReadFuncGlobal(pNode) ); }


/**Function*************************************************************

  Synopsis    [Basic network APIs]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char *           Ntk_NetworkReadName     ( Ntk_Network_t * pNet ) { return pNet->pName;         }
char *           Ntk_NetworkReadSpec     ( Ntk_Network_t * pNet ) { return pNet->pSpec;         }
Mv_Frame_t *     Ntk_NetworkReadMvsis    ( Ntk_Network_t * pNet ) { return pNet->pMvsis;        }
Ntk_Node_t *     Ntk_NetworkReadCiHead   ( Ntk_Network_t * pNet ) { return pNet->lCis.pHead;    }
Ntk_Node_t *     Ntk_NetworkReadCiTail   ( Ntk_Network_t * pNet ) { return pNet->lCis.pTail;    }
int              Ntk_NetworkReadCiNum    ( Ntk_Network_t * pNet ) { return pNet->lCis.nItems;   }
Ntk_Node_t *     Ntk_NetworkReadCoHead   ( Ntk_Network_t * pNet ) { return pNet->lCos.pHead;    }
Ntk_Node_t *     Ntk_NetworkReadCoTail   ( Ntk_Network_t * pNet ) { return pNet->lCos.pTail;    }
int              Ntk_NetworkReadCoNum    ( Ntk_Network_t * pNet ) { return pNet->lCos.nItems;   }
Ntk_Node_t *     Ntk_NetworkReadNodeHead ( Ntk_Network_t * pNet ) { return pNet->lNodes.pHead;  }
Ntk_Node_t *     Ntk_NetworkReadNodeTail ( Ntk_Network_t * pNet ) { return pNet->lNodes.pTail;  }
Ntk_Node_t *     Ntk_NetworkReadNodeHead2( Ntk_Network_t * pNet ) { return pNet->lNodes2.pHead;  }
Ntk_Node_t *     Ntk_NetworkReadNodeTail2( Ntk_Network_t * pNet ) { return pNet->lNodes2.pTail;  }
int              Ntk_NetworkReadNodeIntNum( Ntk_Network_t * pNet ) { return pNet->lNodes.nItems; }
Ntk_Latch_t *    Ntk_NetworkReadLatchHead( Ntk_Network_t * pNet ) { return pNet->lLatches.pHead;}
Ntk_Latch_t *    Ntk_NetworkReadLatchTail( Ntk_Network_t * pNet ) { return pNet->lLatches.pTail;}
int              Ntk_NetworkReadLatchNum ( Ntk_Network_t * pNet ) { return pNet->lLatches.nItems;}
Ntk_Node_t *     Ntk_NetworkReadOrder    ( Ntk_Network_t * pNet ) { return pNet->pOrder;        }
Ntk_Node_t *     Ntk_NetworkReadOrderByLevel ( Ntk_Network_t * pNet, int Level ) { return pNet->ppLevels[Level]; }
Fnc_Manager_t *  Ntk_NetworkReadMan      ( Ntk_Network_t * pNet ) { return pNet->pMan;          }
Mvr_Manager_t *  Ntk_NetworkReadManMvr   ( Ntk_Network_t * pNet ) { return Fnc_ManagerReadManMvr(pNet->pMan); }
Vm_Manager_t *   Ntk_NetworkReadManVm    ( Ntk_Network_t * pNet ) { return Fnc_ManagerReadManVm(pNet->pMan);  }
Vmx_Manager_t *  Ntk_NetworkReadManVmx   ( Ntk_Network_t * pNet ) { return Fnc_ManagerReadManVmx(pNet->pMan); }
Mvc_Manager_t *  Ntk_NetworkReadManMvc   ( Ntk_Network_t * pNet ) { return Fnc_ManagerReadManMvc(pNet->pMan); }
DdManager *      Ntk_NetworkReadManDdLoc ( Ntk_Network_t * pNet ) { return Mvr_ManagerReadDdLoc( Fnc_ManagerReadManMvr(pNet->pMan) );  }
DdManager *      Ntk_NetworkReadDdGlo    ( Ntk_Network_t * pNet ) { return pNet->pDdGlo;   }
Vmx_VarMap_t *   Ntk_NetworkReadVmxGlo   ( Ntk_Network_t * pNet ) { return pNet->pVmxGlo;  }
Ntk_Network_t *  Ntk_NetworkReadNetExdc  ( Ntk_Network_t * pNet ) { return pNet->pNetExdc; }
FILE *           Ntk_NetworkReadMvsisErr ( Ntk_Network_t * pNet ) { return Mv_FrameReadErr( pNet->pMvsis); }
FILE *           Ntk_NetworkReadMvsisOut ( Ntk_Network_t * pNet ) { return Mv_FrameReadOut( pNet->pMvsis); }
Ntk_Network_t *  Ntk_NetworkReadBackup   ( Ntk_Network_t * pNet ) { return pNet->pNetBackup; }
int              Ntk_NetworkReadStep     ( Ntk_Network_t * pNet ) { return pNet->iStep; }
double           Ntk_NetworkReadDefaultArrTimeRise ( Ntk_Network_t * pNet ) { return pNet->dDefaultArrTimeRise; }
double           Ntk_NetworkReadDefaultArrTimeFall ( Ntk_Network_t * pNet ) { return pNet->dDefaultArrTimeFall; }

/**Function*************************************************************

  Synopsis    [Returns the total number of nodes in the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkReadNodeTotalNum( Ntk_Network_t * pNet )
{
    return pNet->lNodes.nItems + pNet->lCis.nItems + pNet->lCos.nItems;
}

/**Function*************************************************************

  Synopsis    [Returns the number of true COs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkReadCiTrueNum( Ntk_Network_t * pNet )
{
    Ntk_Node_t * pNode;
    int Counter = 0;
    Ntk_NetworkForEachCi( pNet, pNode )
        if ( pNode->Subtype != MV_BELONGS_TO_LATCH )
            Counter++;
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Returns the number of true COs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkReadCoTrueNum( Ntk_Network_t * pNet )
{
    Ntk_Node_t * pNode;
    int Counter = 0;
    Ntk_NetworkForEachCo( pNet, pNode )
        if ( pNode->Subtype != MV_BELONGS_TO_LATCH )
            Counter++;
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Returns the number of nodes that actually will be written into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkReadNodeWritableNum( Ntk_Network_t * pNet )
{
    Ntk_Node_t * pNode;
    int nNodes;
    nNodes = Ntk_NetworkReadCiNum( pNet ) + Ntk_NetworkReadCoNum( pNet );
    Ntk_NetworkForEachNode( pNet, pNode )
        if ( !Ntk_NodeHasCoName(pNode) )
            nNodes++;
    return nNodes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkSetNetExdc( Ntk_Network_t * pNet, Ntk_Network_t * pNetExdc ) 
{ 
    assert( pNet->pNetExdc == NULL );
    pNet->pNetExdc = pNetExdc;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkSetName( Ntk_Network_t * pNet, char * pName )
{
    assert( pNet->pName == NULL );
    pNet->pName = pName;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkSetSpec( Ntk_Network_t * pNet, char * pSpec )
{
    assert( pNet->pSpec == NULL );
    pNet->pSpec = pSpec;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkSetMvsis( Ntk_Network_t * pNet, Mv_Frame_t * pMvsis )
{
    assert( pNet->pMvsis == NULL );
    pNet->pMvsis = pMvsis;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkSetBackup( Ntk_Network_t * pNet, Ntk_Network_t * pNetBackup )
{
//    assert( pNetBackup == NULL || pNet->pNetBackup == NULL );
    pNet->pNetBackup = pNetBackup;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkSetStep( Ntk_Network_t * pNet, int iStep )
{
//    assert( pNet->iStep == 0 );
    pNet->iStep = iStep;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkWriteDdGlo( Ntk_Network_t * pNet, DdManager * ddGlo )
{
    pNet->pDdGlo = ddGlo;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkSetDdGlo( Ntk_Network_t * pNet, DdManager * ddGlo )
{
    if ( pNet->pDdGlo == ddGlo )
        return;
    if ( pNet->pDdGlo )
        Extra_StopManager( pNet->pDdGlo );
    pNet->pDdGlo = ddGlo;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkSetVmxGlo( Ntk_Network_t * pNet, Vmx_VarMap_t * pVmxGlo )
{
    pNet->pVmxGlo = pVmxGlo;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkSetDefaultArrTimeRise ( Ntk_Network_t * pNet, double dTime )
{
	pNet->dDefaultArrTimeRise = dTime;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkSetDefaultArrTimeFall ( Ntk_Network_t * pNet, double dTime )
{
	pNet->dDefaultArrTimeFall = dTime;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeSetName( Ntk_Node_t * pNode, char * pName )
{
    assert( pName == NULL || pNode->pName == NULL );
    pNode->pName = pName;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeSetSubtype( Ntk_Node_t * pNode, int Subtype )
{
//    assert( pNode->Subtype == 0 );
    pNode->Subtype = Subtype;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeSetValueNum( Ntk_Node_t * pNode, int nValues )
{
//    assert( pNode->nValues == -1 );
    pNode->nValues = nValues;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeSetData( Ntk_Node_t * pNode, char * pData )
{
    pNode->pData = pData;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeSetMap( Ntk_Node_t * pNode, Ntk_NodeBinding_t * pBinding )
{
    Ntk_NodeFreeBinding( pNode );
    pNode->pMap = pBinding;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeSetOrder( Ntk_Node_t * pNode, Ntk_Node_t * pNext )
{
    pNode->pOrder = pNext;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeSetLevel( Ntk_Node_t * pNode, int Level )
{
    pNode->Level = Level;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeSetArrTimeRise  ( Ntk_Node_t * pNode, double dTime ) 
{ 
    pNode->dArrTimeRise = dTime;  
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeSetArrTimeFall  ( Ntk_Node_t * pNode, double dTime ) 
{ 
    pNode->dArrTimeFall = dTime;  
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeSetNetwork( Ntk_Node_t * pNode, Ntk_Network_t * pNet )
{
    assert( pNode->pNet == NULL );
    pNode->pNet = pNet;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Ntk_NodeIsCi( Ntk_Node_t * pNode )
{
    return (bool)(pNode->Type == MV_NODE_CI);
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Ntk_NodeIsCo( Ntk_Node_t * pNode )
{
    return (bool)(pNode->Type == MV_NODE_CO);
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the node is a "CO driver".]

  Description [A CO driver is an internal node, whose only fanout is a CO.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Ntk_NodeIsCoDriver( Ntk_Node_t * pNode )
{
    Ntk_Node_t * pFanout;
    if ( Ntk_NodeReadFanoutNum(pNode) == 1 )
    {
        pFanout = Ntk_NodeReadFanoutNode( pNode, 0 );
        if ( pFanout->Type == MV_NODE_CO )
            return 1;
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the has one or more CO fanouts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Ntk_NodeIsCoFanin( Ntk_Node_t * pNode )
{
    Ntk_Node_t * pFanout;
    Ntk_Pin_t * pPin;
    Ntk_NodeForEachFanout( pNode, pPin, pFanout )
    {
        if ( Ntk_NodeIsCo(pFanout) )
            return 1;
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [If the intenal node has exactly one CO fanout, returns this CO.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NodeHasCoName( Ntk_Node_t * pNode )
{
    Ntk_Node_t * pFanout, * pFanoutCo = NULL;
    Ntk_Pin_t * pPin;
    int Counter;
    // the node cannot have CO name if it is CI
    if ( pNode->Type != MV_NODE_INT )
        return NULL;
    // the node ahs a CO name if it is an internal node,
    // whose set of fanouts contains exactly one CO
    Counter = 0;
    Ntk_NodeForEachFanout( pNode, pPin, pFanout )
        if ( pFanout->Type == MV_NODE_CO )
        {
            pFanoutCo = pFanout;
            Counter++;
        }
    if ( Counter == 1 )
        return pFanoutCo;
    return NULL;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Ntk_NodeIsInternal( Ntk_Node_t * pNode )
{
    return (bool)(pNode->Type == MV_NODE_INT);
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Ntk_NodeIsConstant( Ntk_Node_t * pNode )
{
    return (Ntk_NodeReadFaninNum(pNode) == 0);
}

/**Function*************************************************************

  Synopsis    [Returns the constant represented by the node.]

  Description [Returns the constant represented by the node, or -1 if
  the node is not a constant node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NodeReadConstant( Ntk_Node_t * pNode )
{
    Cvr_Cover_t * pCvr;
    Mvc_Cover_t ** pIsets;
    int v;

    if ( Ntk_NodeReadFaninNum(pNode) > 0 )
        return -1;

    pCvr = Ntk_NodeGetFuncCvr(pNode);
    pIsets = Cvr_CoverReadIsets(pCvr);
    for ( v = 0; v < pNode->nValues; v++ )
        if ( pIsets[v] && Mvc_CoverIsTautology(pIsets[v]) )
        {
            // what if the node is a non-deterministic constant???
            return v;
        }
    return -1;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Ntk_NodeBelongsToLatch( Ntk_Node_t * pNode )
{
    return (bool)(pNode->Subtype == MV_BELONGS_TO_LATCH);
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NodeReadDefault( Ntk_Node_t * pNode )
{
    Cvr_Cover_t * pCvr;
    assert( pNode->Type == MV_NODE_INT );
    pCvr = Ntk_NodeReadFuncCvr( pNode );
    if ( pCvr == NULL )
        return -1;
    return Cvr_CoverReadDefault(pCvr);
}


/**Function*************************************************************

  Synopsis    [Returns the int node feeding into the first CO node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NetworkReadFirstCo( Ntk_Network_t * pNet )
{
    Ntk_Node_t * pNode;
    Ntk_NetworkForEachCo( pNet, pNode )
        return Ntk_NodeReadFaninNode( pNode, 0 );
    return NULL;
}
////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



/**CFile****************************************************************

  FileName    [autApi.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Contains the basic procedures to support the Autmaton datastructure.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: autApi.c,v 1.2 2004/04/08 04:48:24 alanmi Exp $]

***********************************************************************/

#include "autInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Reading the contents of automaton.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char *           Aut_AutoReadName    ( Aut_Auto_t * pAut )        { return pAut->pName; }
int              Aut_AutoReadVarNum  ( Aut_Auto_t * pAut )        { return pAut->nVars; }
Vmx_VarMap_t *   Aut_AutoReadVmx     ( Aut_Auto_t * pAut )        { return pAut->pVmx;  }
Vm_VarMap_t *    Aut_AutoReadVm      ( Aut_Auto_t * pAut )        { return Vmx_VarMapReadVm(pAut->pVmx); }
Aut_Var_t **     Aut_AutoReadVars    ( Aut_Auto_t * pAut )        { return pAut->pVars; }
int              Aut_AutoReadStateNum( Aut_Auto_t * pAut )        { return pAut->nStates; }
Aut_State_t *    Aut_AutoReadHead    ( Aut_Auto_t * pAut )        { return pAut->pHead; }
Aut_State_t *    Aut_AutoReadTail    ( Aut_Auto_t * pAut )        { return pAut->pTail; }
Aut_State_t *    Aut_AutoReadOrder   ( Aut_Auto_t * pAut )        { return pAut->pOrder; }
int              Aut_AutoReadInitNum ( Aut_Auto_t * pAut )        { return pAut->nInit; }
Aut_State_t *    Aut_AutoReadInit    ( Aut_Auto_t * pAut )        { return pAut->pInit[0]; }
Aut_State_t **   Aut_AutoReadInitAll ( Aut_Auto_t * pAut )        { return pAut->pInit; }
Aut_Man_t *      Aut_AutoReadMan     ( Aut_Auto_t * pAut )        { return pAut->pMan; }
DdManager *      Aut_AutoReadDd      ( Aut_Auto_t * pAut )        { return pAut->pMan->dd; }
int              Aut_AutoReadInputs  ( Aut_Auto_t * pAut )        { return Vm_VarMapReadVarsInNum( Vmx_VarMapReadVm(pAut->pVmx) ); }
int              Aut_AutoReadOutputs ( Aut_Auto_t * pAut )        { return Vm_VarMapReadVarsOutNum( Vmx_VarMapReadVm(pAut->pVmx) ); }
char *           Aut_AutoReadData    ( Aut_Auto_t * pAut )        { return pAut->pData; }

/**Function*************************************************************

  Synopsis    [Setting the contents of automaton.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void             Aut_AutoSetName   ( Aut_Auto_t * pAut, char * pName ) { pAut->pName = pName; }
void             Aut_AutoSetInitNum( Aut_Auto_t * pAut, int nInit )    { pAut->nInit = nInit; }
void             Aut_AutoSetData   ( Aut_Auto_t * pAut, char * pData ) { pAut->pData = pData; }
void             Aut_AutoSetVarNum ( Aut_Auto_t * pAut, int nVars )    { pAut->nVars = nVars; }
void             Aut_AutoSetVars   ( Aut_Auto_t * pAut, Aut_Var_t ** pVars )    { FREE( pAut->pVars ); pAut->pVars = pVars; }
void             Aut_AutoSetVarLNum( Aut_Auto_t * pAut, int nVarsL )   { pAut->nVarsL = nVarsL; }
void             Aut_AutoSetVarsL  ( Aut_Auto_t * pAut, Aut_Var_t ** pVarsL )   { FREE( pAut->pVarsL ); pAut->pVarsL = pVarsL; }
void             Aut_AutoSetInit   ( Aut_Auto_t * pAut, Aut_State_t * pState ) { pAut->pInit[0] = pState; }

/**Function*************************************************************

  Synopsis    [Reading the contents of states.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char *           Aut_StateReadName   ( Aut_State_t * pState )     { return pState->pName; }
bool             Aut_StateReadAcc    ( Aut_State_t * pState )     { return pState->fAcc;  }
bool             Aut_StateReadMark   ( Aut_State_t * pState )     { return pState->fMark; }
unsigned         Aut_StateReadDataU  ( Aut_State_t * pState )     { return pState->uData; }
char *           Aut_StateReadData   ( Aut_State_t * pState )     { return pState->pData; }
Aut_State_t *    Aut_StateReadPrev   ( Aut_State_t * pState )     { return pState->pPrev; }
Aut_State_t *    Aut_StateReadNext   ( Aut_State_t * pState )     { return pState->pNext; }
Aut_State_t *    Aut_StateReadOrder  ( Aut_State_t * pState )     { return pState->pOrder;}
int              Aut_StateReadNumTransFrom( Aut_State_t * pState ){ return pState->nTransFrom;}
Aut_Trans_t *    Aut_StateReadHeadFrom( Aut_State_t * pState )    { return pState->pHeadFrom; }
Aut_Trans_t *    Aut_StateReadTailFrom( Aut_State_t * pState )    { return pState->pTailFrom; }
int              Aut_StateReadNumTransTo( Aut_State_t * pState )  { return pState->nTransTo;}
Aut_Trans_t *    Aut_StateReadHeadTo( Aut_State_t * pState )      { return pState->pHeadTo; }
Aut_Trans_t *    Aut_StateReadTailTo( Aut_State_t * pState )      { return pState->pTailTo; }
DdNode *         Aut_StateReadCond   ( Aut_State_t * pState )     { return pState->bCond;   }
DdNode *         Aut_StateReadCondI  ( Aut_State_t * pState )     { return pState->bCondI;  }
Aut_Auto_t *     Aut_StateReadAut    ( Aut_State_t * pState )     { return pState->pAut;    }

/**Function*************************************************************

  Synopsis    [Setting the contents of states.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void             Aut_StateSetName  ( Aut_State_t * pState, char * pName )    { pState->pName  = pName; }
void             Aut_StateSetAcc   ( Aut_State_t * pState, bool fAcc )       { pState->fAcc   = fAcc;  }
void             Aut_StateSetMark  ( Aut_State_t * pState, bool fMark )      { pState->fMark  = fMark; }
void             Aut_StateSetDataU ( Aut_State_t * pState, unsigned uData )  { pState->uData  = uData; }
void             Aut_StateSetData  ( Aut_State_t * pState, char * pData )    { pState->pData  = pData; }
void             Aut_StateSetCond  ( Aut_State_t * pState, DdNode * bCond )  { pState->bCond  = bCond; }
void             Aut_StateSetCondI ( Aut_State_t * pState, DdNode * bCondI ) { pState->bCondI = bCondI;}


/**Function*************************************************************

  Synopsis    [Reading the contents of transitions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aut_State_t *    Aut_TransReadStateFrom( Aut_Trans_t * pTrans )        { return pTrans->pFrom; }
Aut_State_t *    Aut_TransReadStateTo  ( Aut_Trans_t * pTrans )        { return pTrans->pTo;   }
Aut_Trans_t *    Aut_TransReadNextFrom ( Aut_Trans_t * pTrans )        { return pTrans->pNextFrom; }
Aut_Trans_t *    Aut_TransReadNextTo   ( Aut_Trans_t * pTrans )        { return pTrans->pNextTo; }
Aut_Trans_t *    Aut_TransReadPrevFrom ( Aut_Trans_t * pTrans )        { return pTrans->pPrevFrom; }
Aut_Trans_t *    Aut_TransReadPrevTo   ( Aut_Trans_t * pTrans )        { return pTrans->pPrevTo; }
DdNode *         Aut_TransReadCond     ( Aut_Trans_t * pTrans )        { return pTrans->bCond; }
char *           Aut_TransReadData     ( Aut_Trans_t * pTrans )        { return pTrans->pData; }

/**Function*************************************************************

  Synopsis    [Setting the contents of transitions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void            Aut_TransSetCond      ( Aut_Trans_t * pTrans, DdNode * bCond )  { pTrans->bCond = bCond; }
void            Aut_TransSetData      ( Aut_Trans_t * pTrans, char * pData )    { pTrans->pData = pData; }
void            Aut_TransSetStateFrom ( Aut_Trans_t * pTrans, Aut_State_t * pStateFrom ) { pTrans->pFrom = pStateFrom; }
void            Aut_TransSetStateTo   ( Aut_Trans_t * pTrans, Aut_State_t * pStateTo )   { pTrans->pTo   = pStateTo;   }

/**Function*************************************************************

  Synopsis    [Reading the contents of variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char *          Aut_VarReadName( Aut_Var_t * pVar )                    { return pVar->pName; }
int             Aut_VarReadValueNum  ( Aut_Var_t * pVar )              { return pVar->nValues;   }
char **         Aut_VarReadValueNames ( Aut_Var_t * pVar )             { return pVar->pValueNames; }
char *          Aut_VarReadValueName ( Aut_Var_t * pVar, int iValue )  { return pVar->pValueNames[iValue]; }

/**Function*************************************************************

  Synopsis    [Setting the contents of variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void            Aut_VarSetName      ( Aut_Var_t * pVar, char * pName    )  { pVar->pName = pName; }
void            Aut_VarSetValueNum  ( Aut_Var_t * pVar, int nValues     )  { pVar->nValues = nValues; }
void            Aut_VarSetValueNames( Aut_Var_t * pVar, char ** ppNames )  { pVar->pValueNames = ppNames; }

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



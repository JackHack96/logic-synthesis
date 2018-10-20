/**CFile****************************************************************

  FileName    [aut.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [External declarations of the automaton package.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: aut.h,v 1.8 2004/04/08 04:48:24 alanmi Exp $]

***********************************************************************/

#ifndef __AUT_H__
#define __AUT_H__

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

typedef struct AutAutoStruct    Aut_Auto_t;    // the automaton
typedef struct AutStateStruct   Aut_State_t;   // one state in the state transition graph
typedef struct AutTransStruct   Aut_Trans_t;   // one transition in the state transition graph
typedef struct AutVarStruct     Aut_Var_t;     // one transition in the state transition graph
typedef struct AutManStruct     Aut_Man_t;     // the reusable BDD manager

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

// iterators through the states of the automaton

// iterates through all the states (State) of the automaton (Auto)
#define Aut_AutoForEachState( Auto, State )\
    for ( State = Aut_AutoReadHead(Auto);\
          State;\
          State = Aut_StateReadNext(State) )

// iterates through the states (State) starting from the given one (Start)
#define Aut_AutoForEachStateStart( Start, State )\
    for ( State = Start;\
          State;\
          State = Aut_StateReadNext(State) )

// the safe iterator through all the states (State) of the automaton (Auto)
// which allows for deleting the state that is currently being iterated through (State)
// because the second variable (State2) remembers the next state
#define Aut_AutoForEachStateSafe( Auto, State, State2 )\
    for ( State = Aut_AutoReadHead(Auto), State2 = (State? Aut_StateReadNext(State): NULL);\
          State;\
          State = State2, State2 = (State? Aut_StateReadNext(State): NULL) )

// iterates through the linked list of states, created and used by some procedures
#define Aut_AutoForEachStateSpecial( Auto, State )\
    for ( State = Aut_AutoReadOrder(Auto);\
          State;\
          State = Aut_StateReadOrder(State) )


// iterates through all the outgoing/incoming transitions (Trans) 
// of a state (State)
#define Aut_StateForEachTransitionFrom( State, Trans )\
    for ( Trans = Aut_StateReadHeadFrom(State);\
          Trans;\
          Trans = Aut_TransReadNextFrom(Trans) )
#define Aut_StateForEachTransitionTo( State, Trans )\
    for ( Trans = Aut_StateReadHeadTo(State);\
          Trans;\
          Trans = Aut_TransReadNextTo(Trans) )

// iterates through the outgoing/incoming transitions (Trans) 
// starting from the given state (Start)
#define Aut_StateForEachTransitionFromStart( Start, Trans )\
    for ( Trans = Start;\
          Trans;\
          Trans = Aut_TransReadNextFrom(Trans) )
#define Aut_StateForEachTransitionToStart( Start, Trans )\
    for ( Trans = Start;\
          Trans;\
          Trans = Aut_TransReadNextTo(Trans) )

// safely iterates through all the outgoing/incoming transitions (Trans) 
// of a state (State); the current transition (Trans) can be deleted 
// because the second variable (Trans2) remembers the next transtion
#define Aut_StateForEachTransitionFromSafe( State, Trans, Trans2 )\
    for ( Trans = Aut_StateReadHeadFrom(State), Trans2 = (Trans? Aut_TransReadNextFrom(Trans): NULL);\
          Trans;\
          Trans = Trans2, Trans2 = (Trans? Aut_TransReadNextFrom(Trans): NULL) )
#define Aut_StateForEachTransitionToSafe( State, Trans, Trans2 )\
    for ( Trans = Aut_StateReadHeadTo(State), Trans2 = (Trans? Aut_TransReadNextTo(Trans): NULL);\
          Trans;\
          Trans = Trans2, Trans2 = (Trans? Aut_TransReadNextTo(Trans): NULL) )


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DECLARATIONS                        ///
////////////////////////////////////////////////////////////////////////

/*=== autApi.c ===========================================================*/
extern char *          Aut_AutoReadName     ( Aut_Auto_t * pAut );
extern int             Aut_AutoReadVarNum   ( Aut_Auto_t * pAut );
extern Vmx_VarMap_t *  Aut_AutoReadVmx      ( Aut_Auto_t * pAut );
extern Vm_VarMap_t *   Aut_AutoReadVm       ( Aut_Auto_t * pAut );
extern Aut_Var_t **    Aut_AutoReadVars     ( Aut_Auto_t * pAut );
extern int             Aut_AutoReadStateNum ( Aut_Auto_t * pAut );
extern Aut_State_t *   Aut_AutoReadHead     ( Aut_Auto_t * pAut );
extern Aut_State_t *   Aut_AutoReadTail     ( Aut_Auto_t * pAut );
extern Aut_State_t *   Aut_AutoReadOrder    ( Aut_Auto_t * pAut );
extern int             Aut_AutoReadInitNum  ( Aut_Auto_t * pAut );
extern Aut_State_t *   Aut_AutoReadInit     ( Aut_Auto_t * pAut );
extern Aut_State_t **  Aut_AutoReadInitAll  ( Aut_Auto_t * pAut );
extern Aut_Man_t *     Aut_AutoReadMan      ( Aut_Auto_t * pAut );
extern DdManager *     Aut_AutoReadDd       ( Aut_Auto_t * pAut );
extern int             Aut_AutoReadInputs   ( Aut_Auto_t * pAut );
extern int             Aut_AutoReadOutputs  ( Aut_Auto_t * pAut );
extern char *          Aut_AutoReadData     ( Aut_Auto_t * pAut );

extern void            Aut_AutoSetName      ( Aut_Auto_t * pAut, char * pName );
extern void            Aut_AutoSetInitNum   ( Aut_Auto_t * pAut, int nInit );
extern void            Aut_AutoSetData      ( Aut_Auto_t * pAut, char * pData );
extern void            Aut_AutoSetVarNum    ( Aut_Auto_t * pAut, int nVars );
extern void            Aut_AutoSetVars      ( Aut_Auto_t * pAut, Aut_Var_t ** pVars );
extern void            Aut_AutoSetVarLNum   ( Aut_Auto_t * pAut, int nVarsL );
extern void            Aut_AutoSetVarsL     ( Aut_Auto_t * pAut, Aut_Var_t ** pVarsL );
extern void            Aut_AutoSetInit      ( Aut_Auto_t * pAut, Aut_State_t * pState );

extern char *          Aut_StateReadName   ( Aut_State_t * pState );
extern bool            Aut_StateReadAcc    ( Aut_State_t * pState );
extern bool            Aut_StateReadMark   ( Aut_State_t * pState );
extern unsigned        Aut_StateReadDataU  ( Aut_State_t * pState );
extern char *          Aut_StateReadData   ( Aut_State_t * pState );
extern Aut_State_t *   Aut_StateReadPrev   ( Aut_State_t * pState );
extern Aut_State_t *   Aut_StateReadNext   ( Aut_State_t * pState );
extern Aut_State_t *   Aut_StateReadOrder  ( Aut_State_t * pState );
extern int             Aut_StateReadNumTransFrom( Aut_State_t * pState );
extern Aut_Trans_t *   Aut_StateReadHeadFrom( Aut_State_t * pState );
extern Aut_Trans_t *   Aut_StateReadTailFrom( Aut_State_t * pState );
extern int             Aut_StateReadNumTransTo( Aut_State_t * pState );
extern Aut_Trans_t *   Aut_StateReadHeadTo ( Aut_State_t * pState );
extern Aut_Trans_t *   Aut_StateReadTailTo ( Aut_State_t * pState );
extern DdNode *        Aut_StateReadCond   ( Aut_State_t * pState );
extern DdNode *        Aut_StateReadCondI  ( Aut_State_t * pState );
extern Aut_Auto_t *    Aut_StateReadAut    ( Aut_State_t * pState );

extern void            Aut_StateSetName  ( Aut_State_t * pState, char * pName );
extern void            Aut_StateSetAcc   ( Aut_State_t * pState, bool fAcc );
extern void            Aut_StateSetMark  ( Aut_State_t * pState, bool fMark );
extern void            Aut_StateSetDataU ( Aut_State_t * pState, unsigned uData );
extern void            Aut_StateSetData  ( Aut_State_t * pState, char * pData );
extern void            Aut_StateSetCond  ( Aut_State_t * pState, DdNode * bCond );
extern void            Aut_StateSetCondI ( Aut_State_t * pState, DdNode * bCondI );

extern Aut_State_t *   Aut_TransReadStateFrom( Aut_Trans_t * pTrans );
extern Aut_State_t *   Aut_TransReadStateTo( Aut_Trans_t * pTrans );
extern Aut_Trans_t *   Aut_TransReadNextFrom( Aut_Trans_t * pTrans );
extern Aut_Trans_t *   Aut_TransReadPrevFrom( Aut_Trans_t * pTrans );
extern Aut_Trans_t *   Aut_TransReadNextTo ( Aut_Trans_t * pTrans );
extern Aut_Trans_t *   Aut_TransReadPrevTo ( Aut_Trans_t * pTrans );
extern DdNode *        Aut_TransReadCond   ( Aut_Trans_t * pTrans );
extern char *          Aut_TransReadData   ( Aut_Trans_t * pTrans );
extern void            Aut_TransSetCond    ( Aut_Trans_t * pTrans, DdNode * bCond );
extern void            Aut_TransSetData    ( Aut_Trans_t * pTrans, char * pData );
extern void            Aut_TransSetStateFrom( Aut_Trans_t * pTrans, Aut_State_t * pStateFrom );
extern void            Aut_TransSetStateTo( Aut_Trans_t * pTrans, Aut_State_t * pStateTo );

extern char *          Aut_VarReadName      ( Aut_Var_t * pVar );
extern int             Aut_VarReadValueNum  ( Aut_Var_t * pVar );
extern char **         Aut_VarReadValueNames( Aut_Var_t * pVar );
extern char *          Aut_VarReadValueName ( Aut_Var_t * pVar, int iValue );
extern void            Aut_VarSetName      ( Aut_Var_t * pVar, char * pName    );
extern void            Aut_VarSetValueNum  ( Aut_Var_t * pVar, int nValues     );
extern void            Aut_VarSetValueNames( Aut_Var_t * pVar, char ** ppNames );

/*=== autSpecial.c ===========================================================*/
extern void            Aut_AutoStartSpecial( Aut_Auto_t * pAut );
extern void            Aut_AutoStopSpecial( Aut_Auto_t * pAut );
extern void            Aut_AutoAddToSpecial( Aut_State_t * pState );
extern void            Aut_AutoMoveSpecial( Aut_State_t * pState );
extern void            Aut_AutoResetSpecial( Aut_Auto_t * pAut );
extern int             Aut_AutoCountSpecial( Aut_Auto_t * pAut );
extern void            Aut_AutoCreateSpecialFromArray( Aut_Auto_t * pAut, Aut_State_t * ppStates[], int nNodes );
extern int             Aut_AutoCreateArrayFromSpecial( Aut_Auto_t * pAut, Aut_State_t * ppStates[] );
extern st_table *      Aut_AutoCreateTableFromSpecial( Aut_Auto_t * pAut );
extern void            Aut_AutoPrintSpecial( Aut_Auto_t * pAut );

/*=== autCreate.c ===========================================================*/
extern Aut_Auto_t *    Aut_AutoAlloc( Vmx_VarMap_t * pVmx, Aut_Man_t * pMan );
extern Aut_Auto_t *    Aut_AutoClone( Aut_Auto_t * pAut, Aut_Man_t * pMan );
extern Aut_Auto_t *    Aut_AutoDup( Aut_Auto_t * pAut, Aut_Man_t * pMan );
extern void            Aut_AutoFree( Aut_Auto_t * pAut );
extern char *          Aut_AutoRegisterName( Aut_Auto_t * pAut, char * pName );
extern void            Aut_AutoAddTransition( Aut_Trans_t * pTrans );
extern void            Aut_AutoDeleteTransition( Aut_Trans_t * pTrans );
extern Aut_Trans_t *   Aut_AutoFindTransition( Aut_State_t * pStateFrom, Aut_State_t * pStateTo );
extern Aut_State_t *   Aut_AutoFindState( Aut_Auto_t * pAut, char * pStateName );
extern bool            Aut_AutoTransOverlap( Aut_Trans_t * pTrans1, Aut_Trans_t * pTrans2 );

extern Aut_State_t *   Aut_StateAlloc( Aut_Auto_t * pAut );
extern Aut_State_t *   Aut_StateDup( Aut_Auto_t * pAut, Aut_State_t * pState );
extern void            Aut_StateFree( Aut_State_t * pState );

extern Aut_Trans_t *   Aut_TransAlloc( Aut_Auto_t * pAut );
extern Aut_Trans_t *   Aut_TransDup( Aut_Auto_t * pAut, Aut_Trans_t * pTrans, DdManager * ddOld );
extern void            Aut_TransFree( Aut_Auto_t * pAut, Aut_Trans_t * pTrans );

extern Aut_Var_t *     Aut_VarAlloc();
extern Aut_Var_t *     Aut_VarDup( Aut_Var_t * pVar );
extern void            Aut_VarFree( Aut_Var_t * pVar );

extern Aut_Man_t *     Aut_ManAlloc( int nVars );
extern Aut_Man_t *     Aut_ManCreate( DdManager * dd );
extern void            Aut_ManRef( Aut_Man_t * pMan );
extern void            Aut_ManDeref( Aut_Man_t * pMan );
extern void            Aut_ManResize( Aut_Man_t * pMan, int nSizeNew );
/*=== autLists.c =========================================================*/
extern void            Aut_AutoStateAddFirst( Aut_State_t * pState );
extern void            Aut_AutoStateAddLast( Aut_State_t * pState );
extern void            Aut_AutoStateDelete( Aut_State_t * pState );
extern void            Aut_StateAddLastTransFrom( Aut_State_t * pState, Aut_Trans_t * pTrans );
extern void            Aut_StateAddLastTransTo( Aut_State_t * pState, Aut_Trans_t * pTrans );
extern void            Aut_StateDeleteTransFrom( Aut_Trans_t * pTrans );
extern void            Aut_StateDeleteTransTo( Aut_Trans_t * pTrans );
/*=== autUtils.c =========================================================*/
extern int             Aut_AutoCountTransitions( Aut_Auto_t * pAut );
extern void            Aut_AutoComputeSumCond( Aut_Auto_t * pAut );
extern void            Aut_AutoComputeSumCondI( Aut_Auto_t * pAut, int nInputs );
extern DdNode *        Aut_StateComputeSumCond( Aut_State_t * pState );
extern void            Aut_AutoDerefSumCond( Aut_Auto_t * pAut );
extern void            Aut_AutoDerefSumCondI( Aut_Auto_t * pAut );
extern DdNode *        Aut_AutoGetCubeInput( Aut_Auto_t * pAut, int nInputs );
extern DdNode *        Aut_AutoGetCubeOutput( Aut_Auto_t * pAut, int nInputs );
extern Aut_State_t *   Aut_AutoCreateDcState( Aut_Auto_t * pAut );
extern void            Aut_AutoCleanMark( Aut_Auto_t * pAut );
extern void            Aut_AutoSetMark( Aut_Auto_t * pAut );
extern void            Aut_AutoSetMarkAccepting( Aut_Auto_t * pAut );
extern void            Aut_AutoSetMarkNonAccepting( Aut_Auto_t * pAut );
extern void            Aut_AutoCleanMark2( Aut_Auto_t * pAut );
extern void            Aut_AutoSetMark2( Aut_Auto_t * pAut );
extern void            Aut_AutoAssignNumbers( Aut_Auto_t * pAut );
extern Aut_State_t **  Aut_AutoCollectStates( Aut_Auto_t * pAut );
extern int             Aut_AutoCountAccepting( Aut_Auto_t * pAut );
extern int             Aut_AutoCountNonAccepting( Aut_Auto_t * pAut );
extern void            Aut_AutoRemoveMarkedStates( Aut_Auto_t * pAut );
extern int             Aut_AutoReachability( Aut_Auto_t * pAut );
extern void            Aut_AutoStateRename( Aut_Auto_t * pAut, char * pOldName, char * pNewName );
extern void            Aut_AutoStateAddToTable( Aut_State_t * pState );
extern bool            Aut_AutoCheckIsBinary( Aut_Auto_t * pAut );
extern void            Aut_AutoBinaryEncode( Aut_Auto_t * pAut );

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

#endif

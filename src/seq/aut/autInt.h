/**CFile****************************************************************

  FileName    [autInt.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Internal declarations of the STG-based automata manipulation.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: autInt.h,v 1.9 2004/04/08 04:48:25 alanmi Exp $]

***********************************************************************/

#ifndef __AUT_INT_H__
#define __AUT_INT_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "mv.h"
#include "aut.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

struct AutAutoStruct
{
    // general info
    char *            pName;
    // input/output info
    int               nVars;          // the total number of variables (includes FSM inputs and outputs)
    Aut_Var_t **      pVars;          // the variable names and their value names
    Vmx_VarMap_t *    pVmx;           // information about the encoding of MV variables
    // state info
    int               nStates;        // the number of states
    Aut_State_t *     pHead;          // the head transition
    Aut_State_t *     pTail;          // the tail transition
    st_table *        tName2State;    // mapping of state names into state pointers 
    // special state list
    Aut_State_t *     pOrder;         // the next one in the ordering list
    Aut_State_t **    ppTail;         // the place where the last one goes
    // the information about initial states
    int               nInit;          // the number of first states in the table that are initial
    Aut_State_t **    pInit;          // the array of initial states
    // functionality
    Aut_Man_t *       pMan;           // the BDD manager used to represent the conditions
    // memory
    Extra_MmFixed_t * pMemState;      // memory manager for states
    Extra_MmFixed_t * pMemTrans;      // memory manager for transitions
    Extra_MmFlex_t  * pMemNames;      // memory manager for the names
    // the latch vars from the original network
    int               nVarsL;
    Aut_Var_t **      pVarsL;
    // other things
    char *            pData;           // user's data
};

struct AutStateStruct // 16 words
{
    // basic info
    char *           pName;           // the name of this state
    unsigned         fAcc   :  1;     // tells whether this state is accepting
    unsigned         fMark  :  1;     // internal mark
    unsigned         fMark2 :  1;     // internal mark
    // various data that can be used by the user
    unsigned         uData  : 29;     // user's data
    char *           pData;           // user's data
    char *           pData2;          // user's data
    // linked lists of states
    Aut_State_t *    pPrev;           // the previous state
    Aut_State_t *    pNext;           // the next state
    Aut_State_t *    pOrder;          // the next state in the order
    // the transitions from this state
    int              nTransFrom;      // the number of transitions from this state
    Aut_Trans_t *    pHeadFrom;       // the head transition
    Aut_Trans_t *    pTailFrom;       // the tail transition
    // the transitions into this state
    int              nTransTo;        // the number of transitions from this state
    Aut_Trans_t *    pHeadTo;         // the head transition
    Aut_Trans_t *    pTailTo;         // the tail transition
    // temporary data for the product states
    DdNode *         bCond;           // the sum of conditions from this state
    DdNode *         bCondI;          // the sum of U-domains of the conditions
    // the automaton, from which this state comes
    Aut_Auto_t *     pAut;
};

struct AutTransStruct // 8 words
{
    Aut_State_t *    pFrom;           // the pointer to the from state
    Aut_State_t *    pTo;             // the pointer to the to state
    Aut_Trans_t *    pNextFrom;       // the next transition in the list
    Aut_Trans_t *    pPrevFrom;       // the previous transition in the list 
    Aut_Trans_t *    pNextTo;         // the next transition in the list
    Aut_Trans_t *    pPrevTo;         // the previous transition in the list 
    DdNode *         bCond;           // the condition of this transtion
    char *           pData;           // user's data
};

struct AutVarStruct
{
    char *           pName;           // the name of the variable
    int              nValues;         // the number of values
    char **          pValueNames;     // the array of value names
};

struct AutManStruct
{
    DdManager *      dd;              // the BDD manager
    int              nRefs;           // the reference counter
};


////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

// these interators are meant for the internal use
// because they access the next-pointers directly for efficiency

// iterator through the states of the automaton
#define Aut_AutoForEachState_int( Auto, State )\
    for ( State = (Auto)->pHead;\
          State;\
          State = State->pNext )
#define Aut_AutoForEachStateStart_int( Start, State )\
    for ( State = Start;\
          State;\
          State = State->pNext )
#define Aut_AutoForEachStateSafe_int( Auto, State, State2 )\
    for ( State = (Auto)->pHead, State2 = (State? State->pNext: NULL);\
          State;\
          State = State2, State2 = (State? State->pNext: NULL) )
#define Aut_AutoForEachStateSpecial_int( Auto, State )\
    for ( State = (Auto)->pOrder;\
          State;\
          State = State->pOrder )

// iterator through the transitions of a state
#define Aut_StateForEachTransitionFrom_int( State, Trans )\
    for ( Trans = (State)->pHeadFrom;\
          Trans;\
          Trans = Trans->pNextFrom )
#define Aut_StateForEachTransitionFromSafe_int( State, Trans, Trans2 )\
    for ( Trans = (State)->pHeadFrom, Trans2 = (Trans? Trans->pNextFrom: NULL);\
          Trans;\
          Trans = Trans2, Trans2 = (Trans? Trans->pNextFrom: NULL) )
#define Aut_StateForEachTransitionFromStart_int( Start, Trans )\
    for ( Trans = Start;\
          Trans;\
          Trans = Trans->pNextFrom )

// iterator through the transitions of a state
#define Aut_StateForEachTransitionTo_int( State, Trans )\
    for ( Trans = (State)->pHeadTo;\
          Trans;\
          Trans = Trans->pNextTo )
#define Aut_StateForEachTransitionToSafe_int( State, Trans, Trans2 )\
    for ( Trans = (State)->pHeadTo, Trans2 = (Trans? Trans->pNextTo: NULL);\
          Trans;\
          Trans = Trans2, Trans2 = (Trans? Trans->pNextTo: NULL) )
#define Aut_StateForEachTransitionToStart_int( Start, Trans )\
    for ( Trans = Start;\
          Trans;\
          Trans = Trans->pNextTo )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DECLARATIONS                        ///
////////////////////////////////////////////////////////////////////////

/*=== autPart.c ===========================================================*/

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
#endif

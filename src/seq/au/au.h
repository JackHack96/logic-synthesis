/**CFile****************************************************************

  FileName    [au.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [External declarations of the FSM manager.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: au.h,v 1.1 2004/02/19 03:06:43 alanmi Exp $]

***********************************************************************/

#ifndef __AU_H__
#define __AU_H__

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

typedef struct AuAutoStruct    Au_Auto_t;    // the automaton
typedef struct AuRelStruct     Au_Rel_t;     // the transition relation 
typedef struct AuStateStruct   Au_State_t;   // one state in the state transition graph
typedef struct AuTransStruct   Au_Trans_t;   // one transition in the state transition graph

struct AuTransStruct
{
    Mvc_Cover_t *    pCond;           // the input/output condition
    DdNode *         bCond;
    unsigned         uCond;           // the truth table of the condition
    int              StateCur;        // the current state
    int              StateNext;       // the next state 
    Au_Trans_t *    pNext;           // the next transition
};

struct AuStateStruct
{
    // basic info
    char *           pName;           // the name of this state
    int              fAcc;            // tells whether this state is accepting
    // the list of transitions
    int              nTrans;          // the number of transitions from this state
    Au_Trans_t *    pHead;           // the head transition
    Au_Trans_t *    pTail;           // the tail transition
    // temporary data for the product states
    DdNode *         bSubset;
    DdNode *         bTrans;
    int              State1;          // the state of the first machine
    int              State2;          // the state of the second machine
    short            Level;           // the shortest distance from the initial state
    short            fMark;           // used for mark special states

    unsigned short * pSubs;           // the subset states
    int              nSubs;           // the number of subset states
};

struct AuAutoStruct
{
    char *           pName;
    // input/output info
    int              nInputs;
    int              nOutputs;
    char **          ppNamesInput;
    char **          ppNamesOutput;
    int              nInputsFsm;
    // state and state transition info
    int              nStates;
    int              nStatesAlloc;
    Au_State_t **   pStates;
    st_table *       tStateNames;
    // the information about initial states
    int              nInitial;        // the number of first states in the table that are initial
    // functionality
    Fnc_Manager_t *  pMan;            // the functionality manager
    Au_Rel_t *      pTR;            // the transition relation
};


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

// iterator through the transitions
#define Au_StateForEachTransition( State, Trans )\
    for ( Trans = (State)->pHead;\
          Trans;\
          Trans = Trans->pNext )
#define Au_StateForEachTransitionSafe( State, Trans, Trans2 )\
    for ( Trans = (State)->pHead, Trans2 = (Trans? Trans->pNext: NULL);\
          Trans;\
          Trans = Trans2, Trans2 = (Trans? Trans->pNext: NULL) )
#define Au_StateForEachTransitionStart( Start, Trans )\
    for ( Trans = Start;\
          Trans;\
          Trans = Trans->pNext )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DECLARATIONS                        ///
////////////////////////////////////////////////////////////////////////

/*=== autCreate.c ===========================================================*/
extern Au_Auto_t *      Au_AutoAlloc();
extern void              Au_AutoFree( Au_Auto_t * pAut );
extern Au_State_t *     Au_AutoStateAlloc();
extern Au_Trans_t *     Au_AutoTransAlloc();
extern void              Au_AutoTransAdd( Au_State_t * pState, Au_Trans_t * pTrans );
extern int               Au_AutoCountTransitions( Au_Auto_t * pAut );
extern int               Au_AutoCountProducts( Au_Auto_t * pAut );
/*=== autWriteFsm.c =============================================================*/
extern void              Au_AutoWriteKissFsm( Au_Auto_t * pAut, char * FileName, int nInputs );
/*=== autWrite.c =============================================================*/
extern void              Au_AutoWriteKiss( Au_Auto_t * pAut, char * FileName );
/*=== autReadFsm.c =============================================================*/
extern int               Au_FsmReadKiss( Mv_Frame_t * pMvsis, char * FileName, char * FileNameOut );
/*=== autRead.c =============================================================*/
extern Au_Auto_t *      Au_AutoReadKiss( Mv_Frame_t * pMvsis, char * FileName );
/*=== autCompl.c =============================================================*/
extern int               Au_AutoComplete( Au_Auto_t * pAut, int nInputs, int fAccepting, int fCheckOnly );
/*=== autFilter.c =============================================================*/
extern Au_Auto_t *      Au_AutoProgressive( Au_Auto_t * pAut, int nInputs, int fMoore, int fVerbose );

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

#endif

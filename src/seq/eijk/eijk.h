/**CFile****************************************************************

  FileName    [eijk.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [VanEijk-based sequential optimization]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: eijk.h,v 1.2 2005/04/05 19:42:43 casem Exp $]

***********************************************************************/

#ifndef __EIJK_H__
#define __EIJK_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////
#include "mv.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

// This is how equivalence classes are represented
typedef struct EquivClsStruct {
  struct EquivClsStruct* pNext;
  struct EquivClsStruct* pPrev;
  Fraig_Node_t* pNode;
} EquivCls;

// This is how the set of all equivalence classes is represented.
typedef struct EquivClsSetStruct {
  struct EquivClsSetStruct* pNext;
  struct EquivClsSetStruct* pPrev;
  EquivCls* pEquivCls;
} EquivClsSet;

// This is some supplimental node information.
typedef struct {
  ////////////////////
  // Name info
  char* pCIName; // this is the name of the CI if the node is a CI
  char* pCOName; // this is the name of the CO if the node is a CO

  ////////////////////
  // output marking
  int nIsCO; // is this node a CO?
  int nOutputNumber; // the index this node has in the output array

  ////////////////////
  // needed for BuildTimeFrames
  EquivCls* pEquivCls; // the node's equivalence class
  EquivCls* pAbsoluteEquivCls; // the node's absolute equivalence class
  Fraig_Node_t* pFrame1Rep; // the equivalence class' representative in frame 1
  Fraig_Node_t* pFrame2Latch; // the node to use as a pseudo-latch for frame 2
  Fraig_Node_t* pGoodStateRep; // the equivalence class' representative in the
                               // good state indicator machine
  Fraig_Node_t* pGoodStateEquiv; // a node's equivalent in the
                                 // good state indicator machine
  Fraig_Node_t* pFrame2ReachableRep; // the classes' representative in the frame
                                     // 
                                     // used to determine if frame 2 is reachable
  Fraig_Node_t* pFrame2ReachableEquiv; // the node's equivalent in the
                                       // used to determine if frame 2 is reachable
  Fraig_Node_t* pFrame2Equiv; // the node's equivalent in frame 2
  Fraig_Node_t* pFrame2Rep; // the equivalence classes' representative in frame
                            // 2
  int nStayInEquivCls; // this is 1 if the node should remain in its equivalence
                       // class, 0 if it should be moved to a new class

  ////////////////////
  // needed for InitialEquivCls
  Fraig_Node_t* pInitialEquiv; // the node's equivalent in the initial sim frame

  ////////////////////
  // needed for CollapseEquivalents
  Fraig_Node_t* pCollapseEquiv;  // the node's equivalent in the collapsed aig
  Fraig_Node_t* pCollapseRep;  // the classes' representative in the collapsed
                               // aig
} Eijk_AuxInfo;
#define ReadEijkAux(n)   ((Eijk_AuxInfo*) Fraig_NodeReadData0(n))
#define SetEijkAux(n,a)   Fraig_NodeSetData0(n, (Fraig_Node_t*)a)

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DECLARATIONS                        ///
////////////////////////////////////////////////////////////////////////

/*=== eijk.c =============================================================*/
extern void Eijk_Init( Mv_Frame_t * pMvsis );
extern void Eijk_End( Mv_Frame_t * pMvsis );
extern int Eijk_CommandEijk( Mv_Frame_t * pMvsis, int argc, char **argv );

/*=== vaneijk_algo.c =====================================================*/
extern Fraig_Man_t* vaneijk_algo( Fraig_Man_t* pMan,
                                  int* pnLatches,
                                  int nStats,
                                  int nMultiRun,
                                  int nVerbose,
                                  FILE* pOut );

/*=== equiv_class.c =====================================================*/
extern EquivClsSet* InitEquivClassSet();
extern void FreeEquivClassSet(EquivClsSet* pFree);
extern EquivCls* NewEquivClass(EquivClsSet* pSet);
extern void MoveNodeToNewClass(EquivCls* pFrom,
                               EquivCls* pTo);
extern void AddToClass(Fraig_Node_t* pNode, 
                       EquivCls* pTo);
extern int DiffEquivClasses(EquivCls* pFirst,
                            EquivCls* pSecond);
extern int DiffEquivClassSets(EquivClsSet* pFirst,
                              EquivClsSet* pSecond);

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
#endif

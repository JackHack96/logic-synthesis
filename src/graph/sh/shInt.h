/**CFile****************************************************************

  FileName    [shInt.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Internal declarations of structural hashing package.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: shInt.h,v 1.8 2004/04/12 20:41:08 alanmi Exp $]

***********************************************************************/
/*
    This is a general And-Inv graph (AIG) package, which can be used to 
    represent and manipulate Boolean functions as a non-canonical AIGs.
    Structural hashing of AIGs (compacting of the AIG graphs using a hash table)
    can be performed on the graphs. The one-level hashing hashes each AND-gate
    by its two children. The two-level hashing hashing each two-level AND-gate
    cluster (1-3 AND gates) into its canonical representations. The details
    of a similar two-level structural hashing algorithm can be found here:
    A. Kuehlmann, V. Paruthi, F. Krohm, M. K. Ganai. Robust Boolean reasoning 
    for equivalence checking and functional property verification. IEEE Trans. 
    CAD, Vol. 21, No. 12, December 2002, pp. 1377-1394.

    The package can be started by calling Sh_ManagerAlloc() and giving it
    the starting number of variables and the starting hash table size. 
    The number of variables can be increased during the computation. The table
    resizes dynamically to accomodate the growing graphs.

    The garbage collection turns on when the number of nodes reaches 3 times
    the size of the current table. If the number of free nodes after the garbage 
    collection is less than 1/3 of the limit (3 times then the table size),
    the table is resized by dubling its size.

    The index of an AIG node is something that vaguely reminds one of "index" 
    in DdNode structure of CUDD package. The elementary variables have index 
    equal to the variable number. All the indices from 0 to SH_CONST_INDEX
    are reserved for elementary variables, whose number can be increased 
    at runtime by calling procedure Sh_ManagerResize().

    All the indices afer SH_CONST_INDEX are reserved for the internal AND-nodes
    of the AIGs. They serve for identifying the nodes and giving them names 
    when visualizing or printing into a BLIF file. After a garbage collection, 
    the indices of internal nodes are preserved. This allows for identifying
    each node uniquely during the whole run of the package.

    The reference counting rules and the syntax of AIG-based procedures 
    are very similar to CUDD package. Therefore BDD based computations coded
    with CUDD can be mapped into the AIG-based computation by replacing
    procedure names.
*/

#ifndef __SH_INT_H__
#define __SH_INT_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "mvtypes.h"
#include "util.h"
#include "extra.h"
#include "vm.h"
#include "vmx.h"
#include "sh.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

#define SH_CONST_INDEX      1000000   // the index of the constant 1 node

#define shNodeIsVar( p )        ((Sh_Regular(p))->index <  SH_CONST_INDEX)
#define shNodeIsConst( p )      ((Sh_Regular(p))->index == SH_CONST_INDEX)
#define shNodeIsAnd( p )        ((Sh_Regular(p))->index >  SH_CONST_INDEX)
#define shNodeReadOne( p )      ((Sh_Regular(p))->pOne)
#define shNodeReadTwo( p )      ((Sh_Regular(p))->pTwo)

// macros for internal reference counting
#define shRef( p )          ((Sh_Regular(p))->nRefs++)
#define shDeref( p )        ((Sh_Regular(p))->nRefs--)
// no reference counting (can be used with garbage collection disabled)
//#define shNodeRef( p )          
//#define shNodeDeref( p )        

// uncomment this macro to switch to standard memory management
//#define USE_SYSTEM_MEMORY_MANAGEMENT 

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

typedef struct ShTableStruct      Sh_Table_t;   

struct ShManagerStruct
{
    // the elementary variable nodes
    int            nVars;
    Sh_Node_t **   pVars;
    // the constant 1 node
    Sh_Node_t *    pConst1;
    // the AND nodes
    Sh_Table_t *   pTable;         // hash table of two-input AND nodes

    // various flags and counters
    bool           fTwoLevelHash;  // set to one to enable two level hashing
    bool           fEnableGC;      // set to one to enable garbage collections
    bool           fVerbose;       // output messages about nodes and GCs
    int            nRefErrors;     // the number of referencing errors in Sh_RecursiveDeref()
    int            nTravIds;       // the counter of traversals

    // canonicity
    short *        pCanTable;      // the canonicity table (2^14 entries)
    // the hash table of subgraphs of size 4
    st_table *     tSubgraphs;     // the size-4 subgraphs
    char *         pBuffer;        // the buffer storing the formula strings

    // memory management for the nodes
    Extra_MmFixed_t * pMem;
    Extra_MmFixed_t * pMemCut;     // memory manager for cutsets
};

struct ShNetworkStruct 
{
    Sh_Manager_t * pMan;         // the manager of this network
    // various types of nodes
    int            nInputs;
    int            nOutputs;
    Sh_Node_t **   ppOutputs;
    int            nInputsCore;
    Sh_Node_t **   ppInputsCore;
    int            nOutputsCore;
    Sh_Node_t **   ppOutputsCore;
    int            nSpecials;
    Sh_Node_t **   ppSpecials;
    // statistics
    int            nNodes;       // the total number of nodes
    Sh_Node_t **   ppNodes;      // the temporary array of nodes 
    // the special linked list
    Sh_Node_t *    pOrder;       // special ordering of nodes (such as DFS)
    Sh_Node_t **   ppTail;       // the pointer to the place where next entry is added (internal use only)
    // MV variable maps
    Vm_VarMap_t *  pVmL;
    Vm_VarMap_t *  pVmR;
    Vm_VarMap_t *  pVmS;
    Vm_VarMap_t *  pVmLC;
    Vm_VarMap_t *  pVmRC;
};

struct ShNodeStruct // 36 bytes
{
    Sh_Node_t *    pNext;          // the next pointer in the hash table of AND gates
    Sh_Node_t *    pOrder;         // used for DFS linking, levelizing, etc
    Sh_Node_t *    pOne;           // the left child
    Sh_Node_t *    pTwo;           // the right child
    int            nRefs;          // the reference counter
    int            TravId;         // the current traversal ID of the node
    unsigned       index   : 30;   // the number of the node (the variable number)
    unsigned       fMark   :  1;   // user's flag
    unsigned       fMark2  :  1;   // user's flag
    unsigned       pData;          // user's data
    unsigned       pData2;         // user's data
};

////////////////////////////////////////////////////////////////////////
///                      ITERATORS                                   ///
////////////////////////////////////////////////////////////////////////

// specialized iterator through the ordered nodes
#define Sh_NetworkForEachNodeSpecial( Net, Node )                 \
    for ( Node = Net->pOrder;                                     \
          Node;                                                   \
          Node = Node->pOrder )
#define Sh_NetworkForEachNodeSpecialSafe( Net, Node, Node2 )      \
    for ( Node = Sh_NetworkReadOrder(Net),                        \
          Node2 = (Node? Node->pOrder: NULL);                     \
          Node;                                                   \
          Node = Node2,                                           \
          Node2 = (Node? Node->pOrder: NULL) )

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

// get the polarities of AND gates in the comb
#define St_SignReadPolR( s )           ((s) & 1)
#define St_SignReadPolL( s )           (((s)>>1) & 1)
#define St_SignReadPolRR( s )          (((s)>>2) & 1)
#define St_SignReadPolRL( s )          (((s)>>3) & 1)
#define St_SignReadPolLR( s )          (((s)>>4) & 1)
#define St_SignReadPolLL( s )          (((s)>>5) & 1)
// macros to get the signs of AND-gates from the sign of comb 
#define St_SignReadAnd2( s )           (((s) >> 6) & 15)
#define St_SignReadAnd1( s )            ((s) >> 10)
// macros to get the signs of inputs from the sign of AND gate
#define St_SignReadAndInput2( s )       ((s) & 3)
#define St_SignReadAndInput1( s )      (((s) >> 2) & 3)

// macros to set the same things
#define St_SignWritePolR( s, i )       ((s) |= (i))
#define St_SignWritePolL( s, i )       ((s) |= ((i)<<1))
#define St_SignWritePolRR( s, i )      ((s) |= ((i)<<2))
#define St_SignWritePolRL( s, i )      ((s) |= ((i)<<3))
#define St_SignWritePolLR( s, i )      ((s) |= ((i)<<4))
#define St_SignWritePolLL( s, i )      ((s) |= ((i)<<5))
#define St_SignWriteAnd2( s, i )       ((s) |= ((i) << 6))
#define St_SignWriteAnd1( s, i )       ((s) |= ((i) << 10))
#define St_SignWriteAndInput2( s, i )  ((s) |= (i))
#define St_SignWriteAndInput1( s, i )  ((s) |= ((i) << 2))

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/*=== shHash.c ===========================================================*/
extern Sh_Table_t *    Sh_TableAlloc( int nTableSizeInit );
extern void            Sh_TableFree( Sh_Manager_t * pMan, bool fCheckZeroRefs );
extern int             Sh_TableReadNodes( Sh_Manager_t * pMan );
extern int             Sh_TableReadNodeUniqueNum( Sh_Manager_t * pMan );
extern Sh_Node_t *     Sh_TableLookup( Sh_Manager_t * pMan, Sh_Node_t * pNode1, Sh_Node_t * pNode2 );
extern Sh_Node_t *     Sh_TableInsertUnique( Sh_Manager_t * pMan, Sh_Node_t * pNode );
/*=== shCanon.c ===========================================================*/
extern Sh_Node_t *     Sh_CanonNodeAnd( Sh_Manager_t * p, Sh_Node_t * pNode1, Sh_Node_t * pNode2 );
/*=== shSign.c ===========================================================*/
extern short *         Sh_SignPrecompute();
extern Sh_Node_t *     Sh_SignConstructNodes( Sh_Manager_t * p, short Sign, Sh_Node_t * pNodes[], int nNodes );
extern char *          Sh_SignPrint( short Sign );
extern void            Sh_SignTablePrint( short * pSign2Sign, unsigned * pSign2True );
/*=== shTravId.c =========================================================*/
extern int             Sh_ManagerReadTravId( Sh_Manager_t * pMan );
extern void            Sh_ManagerIncrementTravId( Sh_Manager_t * pMan );
extern int             Sh_NodeReadTravId( Sh_Node_t * pNode );
extern void            Sh_NodeSetTravId( Sh_Node_t * pNode, int TravId );
extern void            Sh_NodeSetTravIdCurrent( Sh_Manager_t * pMan, Sh_Node_t * pNode );
extern bool            Sh_NodeIsTravIdCurrent( Sh_Manager_t * pMan, Sh_Node_t * pNode );
extern bool            Sh_NodeIsTravIdPrevious( Sh_Manager_t * pMan, Sh_Node_t * pNode );

extern void            Sh_NetworkStartSpecial( Sh_Network_t * pNet );
extern void            Sh_NetworkStopSpecial( Sh_Network_t * pNet );
extern void            Sh_NetworkAddToSpecial( Sh_Network_t * pNet, Sh_Node_t * pNode );
extern void            Sh_NetworkMoveSpecial( Sh_Network_t * pNet, Sh_Node_t * pNode );
extern void            Sh_NetworkResetSpecial( Sh_Network_t * pNet );
extern int             Sh_NetworkCountSpecial( Sh_Network_t * pNet );
extern void            Sh_NetworkCreateSpecialFromArray( Sh_Network_t * pNet, Sh_Node_t * ppNodes[], int nNodes );
extern int             Sh_NetworkCreateArrayFromSpecial( Sh_Network_t * pNet, Sh_Node_t * ppNodes[] );
extern void            Sh_NetworkPrintSpecial( Sh_Network_t * pNet );

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

#endif

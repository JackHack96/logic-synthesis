/**CFile****************************************************************

  FileName    [ntkInt.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Declarations of the network/node package.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkInt.h,v 1.43 2005/02/28 05:34:24 alanmi Exp $]

***********************************************************************/
 
#ifndef __NTK_INT_H__
#define __NTK_INT_H__

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

/*
    Latches are represented by latch structures, which has the pointer
    to the node (without fanout) representing the reset table of the 
    latch and the pointers to the latch output and the latch input.
    These are treated as inputs/output to the network and are placed 
    in the linked lists of PIs/POs. 
    
    In the case when the latch output is also a primary input, 
    or when the latch input is also a primary output, there is only 
    one PI/PO used, which is pointed to by the corresponding latch. 
    Note that the node, representing latch reset, is not treated as a 
    node of the network. It is not placed in the network's node 
    name table and in the network's linked list of nodes.
*/

typedef struct NtkPinListStruct    Ntk_PinList_t;   // double linked list of pins
typedef struct NtkNodeListStruct   Ntk_NodeList_t;  // double linked list of nodes
typedef struct NtkLatchListStruct  Ntk_LatchList_t; // double linked list of latches

// various lists
struct NtkPinListStruct
{
    Ntk_Pin_t *      pHead;      // the first pin in the list
    Ntk_Pin_t *      pTail;      // the last pin in the list
    int              nItems;     // the number of pins in the list
};
struct NtkNodeListStruct
{
    Ntk_Node_t *     pHead;      // the first node in the list
    Ntk_Node_t *     pTail;      // the last node in the list
    int              nItems;     // the number of nodes in the list
};
struct NtkLatchListStruct
{
    Ntk_Latch_t *    pHead;      // the first latch in the list
    Ntk_Latch_t *    pTail;      // the last latch in the list
    int              nItems;     // the number of latches in the list
};

// pin
struct NtkPinStruct
{
    Ntk_Pin_t *      pNext;      // the next pin in the linked list
    Ntk_Pin_t *      pPrev;      // the previous pin in the linked list

    Ntk_Pin_t *      pLink;      // the cross link between the fanin and fanout pin
    Ntk_Node_t *     pNode;      // the node pointed to by this pin (for a fanin pin, this is the fanin)
    char *           pData;      // used for temporary data
};
// latch
struct NtkLatchStruct
{
    Ntk_Latch_t *    pNext;      // the double linked list of latches
    Ntk_Latch_t *    pPrev;      // the double linked list of latches

    int              Type;       // the type of the latch (currently not used)
    int              Reset;      // the short cut to the reset value for constant reset nodes
    Ntk_Node_t *     pInput;     // the pointer to the input node (PO)
    Ntk_Node_t *     pOutput;    // the pointer to the output node (PI) 
    Ntk_Node_t *     pNode;      // the pointer to the node representing functionality
    Ntk_Network_t *  pNet;       // the network, to which this latch belongs
    char *           pData;      // used for temporary data
};
// node
struct NtkNodeStruct
{
    Ntk_Node_t *     pNext;       // the linked list of all nodes in the network
    Ntk_Node_t *     pPrev;       // the linked list of all nodes in the network
    Ntk_Node_t *     pOrder;      // the single-linked list for node ordering (DFS, etc)
    Ntk_Node_t *     pLink;       // the single-linked list for node ordering (DFS, etc)

    int              Id;          // the picture ID of the node
    int              TravId;      // the current traversal ID of the node
    char *           pName;       // the name (if given by the user)

    short            Type;        // type of this node (PI, PO, INTERNAL)
    short            Subtype;     // subtype of PI/PO node
    short            Level;       // the logic level of this node
    char             nValues;     // the number of values
    char             fNdTfi;      // the node has ND nodes in its TFI

    Ntk_PinList_t    lFanins;     // the linked list of fanins
    Ntk_PinList_t    lFanouts;    // the linked list of fanouts

    Fnc_Function_t * pF;          // the node's local functionality
    Fnc_Global_t *   pG;          // the node's global functionality

    double           dArrTimeRise;	// default arrival time (rising) for inputs
    double           dArrTimeFall;	// default arrival time (falling) for inputs

    char *           pData;       // used by the application packages
    Ntk_Node_t *     pCopy;       // used internally by the Ntk package
    Ntk_Network_t *  pNet;        // the network, to which this node belongs
    Ntk_NodeBinding_t * pMap;     // the mapping information
};
// network
struct NtkNetworkStruct 
{
    // general information about the network
    // network and node names
    char *           pName;      // the network name
    st_table *       tName2Node; // the table hashing node name into their pointers
    // unique node IDs
    int              nIds;       // the counter of unique node IDs (is not the same as number of nodes)
    Ntk_Node_t **    pId2Node;   // the table of nodes by their unique ID
    int              nIdsAlloc;  // the max number of entries in the ID table
    // unique traversal IDs
    int              nTravIds;   // the counter of unique traversal IDs

    // the network nodes
    Ntk_NodeList_t   lCis;       // the linked list of only primary inputs
    Ntk_NodeList_t   lCos;       // the linked list of only primary outputs
    Ntk_NodeList_t   lNodes;     // the linked list of only internal nodes
    Ntk_NodeList_t   lNodes2;    // the linked list of only internal nodes
    Ntk_LatchList_t  lLatches;   // the linked list of latches
    Ntk_Node_t *     pOrder;     // special ordering of nodes (such as DFS)
    Ntk_Node_t **    ppTail;     // the pointer to the place where next entry is added (internal use only)

    // the levelized network
    Ntk_Node_t **    ppLevels;   // the linked lists of noded by level
    int              nLevels;    // the number of pointer to the linked lists allocated

    // the functionality
    Mv_Frame_t *     pMvsis;     // the MVSIS framework to which this network belongs
    Fnc_Manager_t *  pMan;       // the functionality manager
    DdManager *      pDdGlo;     // the BDD manager used to represent the global MDDs
    Vmx_VarMap_t *   pVmxGlo;    // the extended variable map used to construct global MDDs

    // timing
    double           dDefaultArrTimeRise; // the default arrival time (rising)
    double           dDefaultArrTimeFall; // the default arrival time (falling)

    // the EXDC network
    Ntk_Network_t *  pNetExdc;
    char *           pData;      // miscellaneous data
    char *           pSpec;      // the name of the spec file if present

    // the backup network and the step number
    Ntk_Network_t *  pNetBackup; // the pointer to the previous backup network
    int              iStep;      // the generation number for the given network

    // memory management
    Extra_MmFixed_t* pManPin;    // the pin memory manager
    Extra_MmFixed_t* pManNode;   // the node memory manager
    Extra_MmFlex_t * pNames;     // storage for the externally visible node names (PIs, POs)
    // temporary storage
    Ntk_Node_t **    pArray1;    // after updating memory manager, will be unnecessary
    Ntk_Node_t **    pArray2;    // after updating memory manager, will be unnecessary
    Ntk_Node_t **    pArray3;    // after updating memory manager, will be unnecessary
    int              nArraysAlloc;// the number of entries allocated
};

// binding of a mapped gate 
struct Ntk_NodeBindingStruct_t_
{
    int                 iSignature;   // hack to ensure that data is valid
    char *              pGate;        // the Mio library gate
    Ntk_Node_t **       ppFaninOrder; // the fanins ordered according to the library
    float               Arrival;      // the maximum arrival time of the node
};   


/*
// node
struct NtkNodeStruct // 12 words
{
    Ntk_Node_t *     pNext;       // the linked list of all nodes in the network
    Ntk_Node_t *     pPrev;       // the linked list of all nodes in the network
    Ntk_Node_t *     pOrder;      // the single-linked list for temporary collections of nodes

    unsigned         Type    :  2;// unknown, internal, latch, black box
    unsigned         Id      : 30;// the picture ID of the node
    unsigned         TravId;      // the current traversal ID of the node
    unsigned         Level   : 16;// the logic level of this node

    unsigned         nFanins : 16;// the number of fanins
    Ntk_Pin_t *      pRingI;      // the ring of fanin pins 
    Ntk_Pin_t *      pRingO;      // the ring of fanout pins (typically, only one pin)

    union {
	Fnc_Function_t * pLoc;        // the local function of the node
	Ntk_Node_t *     pReset;      // the reset node of the latch	
    } Func;

    Ntk_Node_t *     pCopy;       // used internally by the Ntk package
    Ntk_Network_t *  pNet;        // the network, to which this node belongs

    char *           pData;       // used by the application packages
};
// net
struct NtkNetStruct // 12 words
{
    Ntk_Net_t *      pNext;       // the linked list of all nets in the network
    Ntk_Net_t *      pPrev;       // the linked list of all nets in the network
    Ntk_Net_t *      pOrder;      // the single-linked list for temporary collections of nets

    char *           pName;       // the name (if given by the user)
    unsigned         Type    :  8;// the net type
    unsigned         nValues :  8;// the number of values

    unsigned         nFanouts: 16;// the number of fanouts
    Ntk_Pin_t *      pRingI;      // the fanin pin (always one)
    Ntk_Pin_t *      pRingO;      // the ring of fanout pints 

    Fnc_Global_t *   pG;          // the net's global functionality

    Ntk_Net_t *      pCopy;       // used internally by the Ntk package
    Ntk_Network_t *  pNet;        // the network, to which this node belongs

    unsigned         uData1;      // used for temporary data
    unsigned         uData2;      // used for temporary data
};
// pin
struct NtkPinStruct // 8 words
{
    Ntk_Pin_t *      pNextOut;    // the next pin in the linked list
    Ntk_Pin_t *      pPrevOut;    // the previous pin in the linked list
    Ntk_Pin_t *      pNextIn;     // the next pin in the linked list
    Ntk_Pin_t *      pPrevIn;     // the previous pin in the linked list
    Ntk_Node_t *     pNode;       // the node pointed to by this pin (for a fanin pin, this is the fanin)
    Ntk_Net_t *      pNet;        // the net pointed to by this pin
    unsigned         uData1;      // used for temporary data
    unsigned         uData2;      // used for temporary data
};
*/

/*
struct NtkVarStruct
{
    char *           pName;
    int              nValues;
    char **          pValueNames;
};

struct NtkPortStruct
{
    Ntk_Var_t *      pVar;
    Ntk_Pin_t *      pFanouts;
    Ntk_Port_t *     pNext;
};

// node
struct NtkNodeStruct
{
    Ntk_Node_t *     pNext;       // the linked list of all nodes in the network
    Ntk_Node_t *     pPrev;       // the linked list of all nodes in the network
    Ntk_Node_t *     pOrder;      // the single-linked list for node ordering (DFS, etc)

    unsigned         Type  :  3;  // unknown, pi, po, int, latch, box
    unsigned         fIsCo :  1;  // set to 1 if the node is the CO
    unsigned         Id    : 28;  // the picture ID of the node
    int              TravId;      // the current traversal ID of the node

    Ntk_Port_t       lPorts;      // the linked list of output ports
    Ntk_Pin_t *      pFanins;     // the ring list of fanins

    union {
	Fnc_Function_t * pLoc;        // the local function of the node
	Ntk_Node_t *     pReset;      // the reset node of the latch	
	Ntk_Subckt_t *   pSubckt;     // the reset node of the latch	
    } Func;
    Fnc_Global_t *   pG;          // the node's global functionality

    unsigned         uData;       // used by the application packages
    Ntk_Node_t *     pCopy;       // used internally by the Ntk package
    Ntk_Network_t *  pNet;        // the network, to which this node belongs
};
// pin
struct NtkPinStruct // 8 words
{
    Ntk_Pin_t *      pNextOut;    // the next pin in the linked list
    Ntk_Pin_t *      pPrevOut;    // the previous pin in the linked list
    Ntk_Pin_t *      pNextIn;     // the next pin in the linked list
    Ntk_Pin_t *      pPrevIn;     // the previous pin in the linked list
    Ntk_Node_t *     pNodeOut;    // the node pointed to by this pin
    Ntk_Node_t *     pNodeIn;     // the node pointed to by this pin
    unsigned         uData1;      // used for temporary data
    unsigned         uData2;      // used for temporary data
};
*/


////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/*=== ntkList.c ====================================================*/
extern int              Ntk_NetworkNumInternal( Ntk_Network_t * pNet );
extern void             Ntk_NetworkNodeListAddFirst( Ntk_Node_t * pNode );
extern void             Ntk_NetworkNodeListAddLast( Ntk_Node_t * pNode );
extern void             Ntk_NetworkNodeListAddLast2( Ntk_Node_t * pNode );
extern void             Ntk_NetworkNodeListDelete( Ntk_Node_t * pNode );

extern void             Ntk_NetworkNodeCiListAddFirst( Ntk_Node_t * pNode );
extern void             Ntk_NetworkNodeCiListAddLast( Ntk_Node_t * pNode );
extern void             Ntk_NetworkNodeCiListDelete( Ntk_Node_t * pNode );

extern void             Ntk_NetworkNodeCoListAddFirst( Ntk_Node_t * pNode );
extern void             Ntk_NetworkNodeCoListAddLast( Ntk_Node_t * pNode );
extern void             Ntk_NetworkNodeCoListDelete( Ntk_Node_t * pNode );

extern void             Ntk_NetworkLatchListAddFirst( Ntk_Latch_t * pLatch );
extern void             Ntk_NetworkLatchListAddLast( Ntk_Latch_t * pLatch );
extern void             Ntk_NetworkLatchListDelete( Ntk_Latch_t * pLatch );

extern void             Ntk_NodeFaninListAddFirst( Ntk_Node_t * pNode, Ntk_Pin_t * pPin );
extern void             Ntk_NodeFaninListAddLast( Ntk_Node_t * pNode, Ntk_Pin_t * pPin );
extern void             Ntk_NodeFaninListDelete( Ntk_Node_t * pNode, Ntk_Pin_t * pPin );

extern void             Ntk_NodeFanoutListAddFirst( Ntk_Node_t * pNode, Ntk_Pin_t * pPin );
extern void             Ntk_NodeFanoutListAddLast( Ntk_Node_t * pNode, Ntk_Pin_t * pPin );
extern void             Ntk_NodeFanoutListDelete( Ntk_Node_t * pNode, Ntk_Pin_t * pPin );

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

#endif


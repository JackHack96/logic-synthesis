/**CFile****************************************************************

  FileName    [aigInt.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [AND-INV graph package with built in sat_sweep.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - July 10, 2004]

  Revision    [$Id: aigInt.h,v 1.3 2004/07/29 04:54:47 alanmi Exp $]

***********************************************************************/
 
#ifndef __AIG_INT_H__
#define __AIG_INT_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mv.h"
#include "aig.h"
#include "sat.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////
 
////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

// the number of simulation vectors to consider
#define NSIMS                   127
// when changing this number, make sure that it does not exceed the number 
// of primes used for computing the hash key in file "aigAnd.c"

// the bit masks
#define AIG_MASK(n)             ((~((unsigned)0)) >> (32-(n)))
#define AIG_FULL                 (~((unsigned)0))

// accessing the node in the AI-graph
#define Aig_NodeIsConst(p)      ((Aig_Regular(p))->Num == 0)
#define Aig_NodeIsVar(p)        ((Aig_Regular(p))->p1 == NULL && (Aig_Regular(p))->Num )
#define Aig_NodeIsAnd(p)        ((Aig_Regular(p))->p1)
#define Aig_NodeOne(p)          ((Aig_Regular(p))->p1)
#define Aig_NodeTwo(p)          ((Aig_Regular(p))->p2)
#define Aig_NodeReadRef(p)      ((Aig_Regular(p))->nRefs)
#define Aig_NodeRef(p)          ((Aig_Regular(p))->nRefs++)

// macros to get hold of the bits in the support info
#define Aig_NodeSetVar(p,i)      (Aig_Regular(p)->pSupp[(i)>>5] |= (1<<((i) & 31)))
#define Aig_NodeHasVar(p,i)     ((Aig_Regular(p)->pSupp[(i)>>5]  & (1<<((i) & 31))) > 0)

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

// the mapping manager
struct Aig_ManStruct_t_
{
    // the AI-graph
    Aig_Node_t **       pInputs;       // the array of inputs
    int                 nInputs;       // the number of inputs
    Aig_Node_t **       pOutputs;      // the array of outputs
    int                 nOutputs;      // the number of outputs
    Aig_Node_t *        pConst1;       // the constant 1 node

    // various hash-tables
    Aig_HashTable_t *   pTableS;       // hashing by structure
    Aig_HashTable_t *   pTableF;       // hashing by function
    Aig_NodeVec_t *     pVec;          // hashing by number

    // info about the original circuit
    char **             ppOutputNames; // the primary output names

    // parameters
    int                 fVerbose;      // the verbosiness flag
    int                 fOneLevel;     // performs only one level hashing

    // the memory managers
    Extra_MmFixed_t *   mmNodes;       // the memory manager for nodes
    Extra_MmFixed_t *   mmSims;        // the memory manager for cuts
    Extra_MmFixed_t *   mmSupps;       // the memory manager for supports
    int                 nSuppWords;    // the number of unsigned support vars

    // solving the SAT problem
    Sat_Solver_t *      pSat;          // the SAT solver
    Aig_NodeVec_t *     vNodes;        // the temporary array of nodes
    Sat_IntVec_t *      vVarsInt;      // the temporary array of variables
    Sat_IntVec_t *      vProj;         // the temporary array of projection vars 
    int                 nSatProof;     // the number of times the proof was found
    int                 nSatCounter;   // the number of times the counter example was found
    int                 nSatZeros;     // the number of times the simulation vector is zero


    // runtime statistics
    int                 timeToAig;     // time to transfer to the mapping structure
    int                 timeSims;      // time to compute k-feasible cuts
    int                 timeTrav;      // time to traverse the network
    int                 timeCnf;       // time to load CNF into the SAT solver
    int                 timeSat;       // time to compute the truth table for each cut
    int                 timeToNet;     // time to transfer back to the network
    int                 timeTotal;     // the total mapping time
    int                 time1;         // time to transfer back to the network
    int                 time2;         // time to transfer back to the network
};

// the mapping node
struct Aig_NodeStruct_t_ 
{
    // general information about the node
    int                 Num;           // the unique number of this node
    unsigned            fMark0  : 1;   // the mark used for traversals
    unsigned            fMark1  : 1;   // the mark used for traversals
    unsigned            NumTemp :14;   // the level of the given node
    unsigned            nRefs   :16;   // the number of references (fanouts) of the given node

    // the successors of this node     
    Aig_Node_t *        p1;            // the first child
    Aig_Node_t *        p2;            // the second child

    // various linked lists
    Aig_Node_t *        pNextS;        // the next node in the structural hash table
    Aig_Node_t *        pNextF;        // the next node in the functional hash table
    Aig_Node_t *        pEqu;          // the next functionally-equivalent node
    Aig_Node_t *        pDiff;         // the next functionally-different, simulation-equivalent node

    // simulation data
    Aig_Sims_t *        pSims;         // the simulation vectors
    // support data
    unsigned *          pSupp;         // the support of the given node 

    // misc information  
    char *              pData0;        // temporary storage for the corresponding network node
    char *              pData1;        // temporary storage for the corresponding network node
}; 

// the entry in the hash table
struct Aig_SimsStruct_t_
{
    unsigned            uHash  : 31;   // the hash value 
    unsigned            fInv   :  1;   // the flag marking the complement of the simulation info
    unsigned            uTests[NSIMS]; // the simulation info
};

// the vector of nodes
struct Aig_NodeVecStruct_t_
{
    Aig_Node_t **       pArray;        // the array of nodes
    int                 nSize;         // the number of entries in the array
    int                 nCap;          // the number of allocated entries
};

// the hash table 
struct Aig_HashTableStruct_t_
{
    Aig_Node_t **       pBins;         // the table bins
    int                 nBins;         // the size of the table
    int                 nEntries;      // the total number of entries in the table
};

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/*=== aigAnd.c =============================================================*/
extern Aig_Node_t *      Aig_NodeCreate( Aig_Man_t * p, Aig_Node_t * p1, Aig_Node_t * p2 );
/*=== aigDfs.c =============================================================*/
extern Aig_NodeVec_t *   Aig_Dfs( Aig_Man_t * pMan );
extern int               Aig_CountLevels( Aig_Man_t * pMan );
extern void              Aig_Unmark( Aig_Man_t * pMan );
extern void              Aig_DfsForTwo( Aig_Man_t * pMan, Aig_Node_t * p1, Aig_Node_t * p2 );
extern void              Aig_PrintNodeList( Aig_Man_t * pMan, Sat_IntVec_t * vNodes );
/*=== aigSat.c =============================================================*/
extern int               Aig_NodeAddClauses( Aig_Man_t * p, Aig_Node_t * pNode, int fVerbose );
/*=== aigVec.c =============================================================*/
extern Aig_NodeVec_t *   Aig_NodeVecAlloc( int nCap );
extern void              Aig_NodeVecFree( Aig_NodeVec_t * p );
extern Aig_Node_t **     Aig_NodeVecReadArray( Aig_NodeVec_t * p );
extern int               Aig_NodeVecReadSize( Aig_NodeVec_t * p );
extern void              Aig_NodeVecGrow( Aig_NodeVec_t * p, int nCapMin );
extern void              Aig_NodeVecShrink( Aig_NodeVec_t * p, int nSizeNew );
extern void              Aig_NodeVecClear( Aig_NodeVec_t * p );
extern void              Aig_NodeVecPush( Aig_NodeVec_t * p, Aig_Node_t * Entry );
extern Aig_Node_t *      Aig_NodeVecPop( Aig_NodeVec_t * p );
extern void              Aig_NodeVecWriteEntry( Aig_NodeVec_t * p, int i, Aig_Node_t * Entry );
extern Aig_Node_t *      Aig_NodeVecReadEntry( Aig_NodeVec_t * p, int i );\

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

#endif

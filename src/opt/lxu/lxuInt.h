/**CFile****************************************************************

  FileName    [lxuInt.h]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Internal declarations of fast extract for unate covers.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: lxuInt.h,v 1.4 2004/06/22 18:12:30 satrajit Exp $]

***********************************************************************/
 
#ifndef __LXU_INT_H__
#define __LXU_INT_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "util.h"
#include "extra.h"
#include <time.h>
#include <string.h>

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

// uncomment this macro to switch to standard memory management
//#define USE_SYSTEM_MEMORY_MANAGEMENT 

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/*  
	Here is an informal description of the FX data structure.
	(1) The sparse matrix is filled with literals, associated with 
	    cubes (row) and variables (columns). The matrix contains 
        all the cubes of all the nodes in the network.
	(2) A cube is associated with 
	    (a) its literals in the matrix,
	    (b) the output variable of the node, to which this cube belongs,
	(3) A variable is associated with 
	    (a) its literals in the matrix and
		(b) the list of cube pairs in the cover, for which it is the output
	(4) A cube pair is associated with two cubes and contains the counters
	    of literals in the base and in the cubes without the base
	(5) A double-cube divisor is associated with list of all cube pairs 
        that produce it and its current weight (which is updated automatically 
        each time a new pair is added or an old pair is removed). 
	(6) A single-cube divisor is associated the pair of variables. 
*/

typedef double weightType;
#define weightType_POS_INF (HUGE_VAL)
#define weightType_NEG_INF (-(HUGE_VAL))

// sparse matrix
typedef struct LxuMatrix        Lxu_Matrix;     // the sparse matrix

// sparse matrix contents: cubes (rows), vars (columns), literals (entries)
typedef struct LxuCube          Lxu_Cube;       // one cube in the sparse matrix
typedef struct LxuVar           Lxu_Var;        // one literal in the sparse matrix
typedef struct LxuLit           Lxu_Lit;        // one entry in the sparse matrix

// double cube divisors
typedef struct LxuPair          Lxu_Pair;       // the pair of cubes
typedef struct LxuDouble        Lxu_Double;     // the double-cube divisor
typedef struct LxuSingle        Lxu_Single;     // the two-literal single-cube divisor

// various lists
typedef struct LxuListCube      Lxu_ListCube;   // the list of cubes
typedef struct LxuListVar       Lxu_ListVar;    // the list of literals
typedef struct LxuListLit       Lxu_ListLit;    // the list of entries
typedef struct LxuListPair      Lxu_ListPair;   // the list of pairs
typedef struct LxuListDouble    Lxu_ListDouble; // the list of divisors
typedef struct LxuListSingle    Lxu_ListSingle; // the list of single-cube divisors

// various heaps
typedef struct LxuHeapDouble    Lxu_HeapDouble; // the heap of divisors
typedef struct LxuHeapSingle    Lxu_HeapSingle; // the heap of variables

// placement stuff
// coordType_POS_INF is often used as a nil value i.e. coords of an empty rectangle
typedef double coord;
#define coordType_POS_INF (HUGE_VAL)
#define coordType_NEG_INF (-(HUGE_VAL))

typedef struct LxuRect Lxu_Rect;
typedef struct LxuPoint Lxu_Point;


// Placement stuff
struct LxuPoint {
	    coord x, y;
};

struct LxuRect {
	    Lxu_Point tl, br;       // topleft and bottom right
};


// various lists

// the list of cubes in the sparse matrix 
struct LxuListCube
{
	Lxu_Cube *       pHead;
	Lxu_Cube *       pTail;
	int              nItems;
};

// the list of literals in the sparse matrix 
struct LxuListVar
{
	Lxu_Var *        pHead;
	Lxu_Var *        pTail;
	int              nItems;
};

// the list of entries in the sparse matrix 
struct LxuListLit
{
	Lxu_Lit *        pHead;
	Lxu_Lit *        pTail;
	int              nItems;
};

// the list of cube pair in the sparse matrix 
struct LxuListPair
{
	Lxu_Pair *       pHead;
	Lxu_Pair *       pTail;
	int              nItems;
};

// the list of divisors in the sparse matrix 
struct LxuListDouble
{
	Lxu_Double *     pHead;
	Lxu_Double *     pTail;
	int              nItems;
};

// the list of divisors in the sparse matrix 
struct LxuListSingle
{
	Lxu_Single *     pHead;
	Lxu_Single *     pTail;
	int              nItems;
};


// various heaps

// the heap of double cube divisors by weight
struct LxuHeapDouble
{
	Lxu_Double **    pTree;
	int              nItems;
	int              nItemsAlloc;
	int              i;
};

// the heap of variable by their occurrence in the cubes
struct LxuHeapSingle
{
	Lxu_Single **    pTree;
	int              nItems;
	int              nItemsAlloc;
	int              i;
};



// sparse matrix
struct LxuMatrix // ~ 30 words
{
	// information about the network
	int              fMvNetwork;  // set to 1 if the network has MV nodes
	int *            pValue2Node; // the mapping of values into nodes
	// the cubes
	Lxu_ListCube     lCubes;      // the double linked list of cubes
	// the values (binary literals)
	Lxu_ListVar      lVars;       // the double linked list of variables
	Lxu_Var **       ppVars;      // the array of variables
	// the double cube divisors
	Lxu_ListDouble * pTable;      // the hash table of divisors
	int              nTableSize;  // the hash table size
	int              nDivs;       // the number of divisors in the table
	int              nDivsTotal;  // the number of divisors in the table
	Lxu_HeapDouble * pHeapDouble;    // the heap of divisors by weight
	// the single cube divisors
	Lxu_ListSingle   lSingles;    // the linked list of single cube divisors  
	Lxu_HeapSingle * pHeapSingle; // the heap of variables by the number of literals in the matrix
	// storage for cube pairs
	Lxu_Pair ***     pppPairs;
	Lxu_Pair **      ppPairs;
	// temporary storage for cubes 
	Lxu_Cube *       pOrderCubes;
	Lxu_Cube **      ppTailCubes;
	// temporary storage for variables 
	Lxu_Var *        pOrderVars;
	Lxu_Var **       ppTailVars;
	// temporary storage for variables
	Lxu_Var **       pVarsTemp;
	int              nVarsTemp;
	// temporary storage for pairs
	Lxu_Pair **      pPairsTemp;
	int              nPairsTemp;
	int              nPairsMax;
	// statistics
	int              nEntries;    // the total number of entries in the sparse matrix
	int              nDivs1;      // the single cube divisors taken
	int              nDivs2;      // the double cube divisors taken
	int              nDivs3;      // the double cube divisors with complement
	// memory manager
	Extra_MmFixed_t* pMemMan;     // the memory manager for all small sized entries
	int		 allPlaced;   // Set to true after initial placement
	int		 plot;        // Whether we plot the intermediate placements or not
	char *           extPlacerCmd;   // Name of the external placer; invoked by a system call
};

// the cube in the sparse matrix
struct LxuCube // 9 words
{
	int              iCube;       // the number of this cube in the cover
	Lxu_Cube *       pFirst;      // the pointer to the first cube of this cover
	Lxu_Var *        pVar;        // the variable representing the output of the cover
	Lxu_ListLit      lLits;       // the row in the table 
	Lxu_Cube *       pPrev;       // the previous cube
	Lxu_Cube *       pNext;       // the next cube
    Lxu_Cube *       pOrder;      // the specialized linked list of cubes
};

// the variable in the sparse matrix
struct LxuVar // 10 words
{
	int              iVar;        // the number of this variable
    int              nCubes;      // the number of cubes assoc with this var
    Lxu_Cube *       pFirst;      // the first cube assoc with this var
    Lxu_Pair ***     ppPairs;     // the pairs of cubes assoc with this var
	Lxu_ListLit      lLits;       // the column in the table 
	Lxu_Var *        pPrev;       // the previous variable
	Lxu_Var *        pNext;       // the next variable
    Lxu_Var *        pOrder;      // the specialized linked list of variables
	Lxu_Point		 position;	  // where this node gets placed
	int 			 Type;		  // 1 => PI; 2 => LatchOutput; 3 => PO; 4 => LatchInput;
									// 0 => neither
};

// the literal entry in the sparse matrix 
struct LxuLit // 8 words
{
	int              iVar;        // the number of this variable
	int              iCube;       // the number of this cube
	Lxu_Cube *       pCube;       // the cube of this literal
	Lxu_Var *        pVar;        // the variable of this literal
	Lxu_Lit *        pHPrev;      // prev lit in the cube
	Lxu_Lit *        pHNext;      // next lit in the cube
	Lxu_Lit *        pVPrev;      // prev lit of the var     
	Lxu_Lit *        pVNext;      // next lit of the var   
	int				 temp;		  // temporary storage used by Euclid
};

// the cube pair
struct LxuPair // 10 words
{
	int              nLits1;      // the number of literals in the two cubes
	int              nLits2;      // the number of literals in the two cubes
	int              nBase;       // the number of literals in the base
	Lxu_Double *     pDiv;        // the divisor of this pair
	Lxu_Cube *       pCube1;      // the first cube of the pair
	Lxu_Cube *       pCube2;      // the second cube of the pair
	int              iCube1;      // the first cube of the pair
	int              iCube2;      // the second cube of the pair
	Lxu_Pair *       pDPrev;      // the previous pair in the divisor
	Lxu_Pair *       pDNext;      // the next pair in the divisor
};

// the double cube divisor
struct LxuDouble // 10+6+4 words
{
	int              Num;         // the unique number of this divisor
	int              HNum;        // the heap number of this divisor
	weightType       Weight;      // the weight of this divisor
	weightType       lWeight;     // the logical weight of this divisor
	weightType       pWeight;     // the physical weight of this divisor
	unsigned         Key;         // the hash key of this divisor
	Lxu_ListPair     lPairs;      // the pairs of cubes, which produce this divisor
	Lxu_Double *     pPrev;       // the previous divisor in the table
	Lxu_Double *     pNext;       // the next divisor in the table
    Lxu_Double *     pOrder;      // the specialized linked list of divisors
	Lxu_Point        position;	  // where this divisor will get placed if selected
};

// the single cube divisor
struct LxuSingle // 7+6+4 words
{
	int              Num;         // the unique number of this divisor
	int              HNum;        // the heap number of this divisor
	weightType       Weight;      // the weight of this divisor
	weightType       lWeight;     // the logical weight of this divisor
	weightType       pWeight;     // the physical weight of this divisor
	Lxu_Var *        pVar1;       // the first variable of the single-cube divisor
	Lxu_Var *        pVar2;       // the second variable of the single-cube divisor
	Lxu_Single *     pPrev;       // the previous divisor in the list
	Lxu_Single *     pNext;       // the next divisor in the list
	Lxu_Point        position;	  // where this divisor will get placed if selected
};


////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

// minimum/maximum
#define Lxu_Min( a, b ) ( ((a)<(b))? (a):(b) )
#define Lxu_Max( a, b ) ( ((a)>(b))? (a):(b) )

// selection of the minimum/maximum cube in the pair
#define Lxu_PairMinCube( pPair )    (((pPair)->iCube1 < (pPair)->iCube2)? (pPair)->pCube1: (pPair)->pCube2)
#define Lxu_PairMaxCube( pPair )    (((pPair)->iCube1 > (pPair)->iCube2)? (pPair)->pCube1: (pPair)->pCube2)
#define Lxu_PairMinCubeInt( pPair ) (((pPair)->iCube1 < (pPair)->iCube2)? (pPair)->iCube1: (pPair)->iCube2)
#define Lxu_PairMaxCubeInt( pPair ) (((pPair)->iCube1 > (pPair)->iCube2)? (pPair)->iCube1: (pPair)->iCube2)

// iterators

#define Lxu_MatrixForEachCube( Matrix, Cube )\
	for ( Cube = (Matrix)->lCubes.pHead;\
          Cube;\
		  Cube = Cube->pNext )
#define Lxu_MatrixForEachCubeSafe( Matrix, Cube, Cube2 )\
	for ( Cube = (Matrix)->lCubes.pHead, Cube2 = (Cube? Cube->pNext: NULL);\
          Cube;\
		  Cube = Cube2, Cube2 = (Cube? Cube->pNext: NULL) )

#define Lxu_MatrixForEachVariable( Matrix, Var )\
	for ( Var = (Matrix)->lVars.pHead;\
          Var;\
		  Var = Var->pNext )
#define Lxu_MatrixForEachVariableSafe( Matrix, Var, Var2 )\
	for ( Var = (Matrix)->lVars.pHead, Var2 = (Var? Var->pNext: NULL);\
          Var;\
		  Var = Var2, Var2 = (Var? Var->pNext: NULL) )

#define Lxu_MatrixForEachSingle( Matrix, Single )\
	for ( Single = (Matrix)->lSingles.pHead;\
          Single;\
		  Single = Single->pNext )
#define Lxu_MatrixForEachSingleSafe( Matrix, Single, Single2 )\
	for ( Single = (Matrix)->lSingles.pHead, Single2 = (Single? Single->pNext: NULL);\
          Single;\
		  Single = Single2, Single2 = (Single? Single->pNext: NULL) )

#define Lxu_TableForEachDouble( Matrix, Key, Div )\
	for ( Div = (Matrix)->pTable[Key].pHead;\
          Div;\
		  Div = Div->pNext )
#define Lxu_TableForEachDoubleSafe( Matrix, Key, Div, Div2 )\
	for ( Div = (Matrix)->pTable[Key].pHead, Div2 = (Div? Div->pNext: NULL);\
          Div;\
		  Div = Div2, Div2 = (Div? Div->pNext: NULL) )

#define Lxu_MatrixForEachDouble( Matrix, Div, Index )\
    for ( Index = 0; Index < (Matrix)->nTableSize; Index++ )\
        Lxu_TableForEachDouble( Matrix, Index, Div )
#define Lxu_MatrixForEachDoubleSafe( Matrix, Div, Div2, Index )\
    for ( Index = 0; Index < (Matrix)->nTableSize; Index++ )\
        Lxu_TableForEachDoubleSafe( Matrix, Index, Div, Div2 )


#define Lxu_CubeForEachLiteral( Cube, Lit )\
	for ( Lit = (Cube)->lLits.pHead;\
          Lit;\
		  Lit = Lit->pHNext )
#define Lxu_CubeForEachLiteralSafe( Cube, Lit, Lit2 )\
	for ( Lit = (Cube)->lLits.pHead, Lit2 = (Lit? Lit->pHNext: NULL);\
          Lit;\
		  Lit = Lit2, Lit2 = (Lit? Lit->pHNext: NULL) )

#define Lxu_VarForEachLiteral( Var, Lit )\
	for ( Lit = (Var)->lLits.pHead;\
          Lit;\
		  Lit = Lit->pVNext )

#define Lxu_CubeForEachDivisor( Cube, Div )\
	for ( Div = (Cube)->lDivs.pHead;\
          Div;\
		  Div = Div->pCNext )

#define Lxu_DoubleForEachPair( Div, Pair )\
	for ( Pair = (Div)->lPairs.pHead;\
          Pair;\
		  Pair = Pair->pDNext )
#define Lxu_DoubleForEachPairSafe( Div, Pair, Pair2 )\
	for ( Pair = (Div)->lPairs.pHead, Pair2 = (Pair? Pair->pDNext: NULL);\
          Pair;\
		  Pair = Pair2, Pair2 = (Pair? Pair->pDNext: NULL) )


// iterator through the cube pairs belonging to the given cube 
#define Lxu_CubeForEachPair( pCube, pPair, i )\
  for ( i = 0;\
        i < pCube->pVar->nCubes &&\
        (((unsigned)(pPair = pCube->pVar->ppPairs[pCube->iCube][i])) >= 0);\
		i++ )\
	    if ( pPair )

// iterator through all the items in the heap
#define Lxu_HeapDoubleForEachItem( Heap, Div )\
	for ( Heap->i = 1;\
		  Heap->i <= Heap->nItems && (Div = Heap->pTree[Heap->i]);\
		  Heap->i++ )
#define Lxu_HeapSingleForEachItem( Heap, Single )\
	for ( Heap->i = 1;\
		  Heap->i <= Heap->nItems && (Single = Heap->pTree[Heap->i]);\
		  Heap->i++ )

// starting the rings
#define Lxu_MatrixRingCubesStart( Matrix )    (((Matrix)->ppTailCubes = &((Matrix)->pOrderCubes)), ((Matrix)->pOrderCubes = NULL))
#define Lxu_MatrixRingVarsStart(  Matrix )    (((Matrix)->ppTailVars  = &((Matrix)->pOrderVars)),  ((Matrix)->pOrderVars  = NULL))
// stopping the rings
#define Lxu_MatrixRingCubesStop(  Matrix )
#define Lxu_MatrixRingVarsStop(   Matrix )
// resetting the rings
#define Lxu_MatrixRingCubesReset( Matrix )    (((Matrix)->pOrderCubes = NULL), ((Matrix)->ppTailCubes = NULL))
#define Lxu_MatrixRingVarsReset(  Matrix )    (((Matrix)->pOrderVars  = NULL), ((Matrix)->ppTailVars  = NULL))
// adding to the rings
#define Lxu_MatrixRingCubesAdd( Matrix, Cube) ((*((Matrix)->ppTailCubes) = Cube), ((Matrix)->ppTailCubes = &(Cube)->pOrder), ((Cube)->pOrder = (Lxu_Cube *)1))
#define Lxu_MatrixRingVarsAdd(  Matrix, Var ) ((*((Matrix)->ppTailVars ) = Var ), ((Matrix)->ppTailVars  = &(Var)->pOrder ), ((Var)->pOrder  = (Lxu_Var *)1))
// iterating through the rings
#define Lxu_MatrixForEachCubeInRing( Matrix, Cube )\
    if ( (Matrix)->pOrderCubes )\
	for ( Cube = (Matrix)->pOrderCubes;\
          Cube != (Lxu_Cube *)1;\
		  Cube = Cube->pOrder )
#define Lxu_MatrixForEachCubeInRingSafe( Matrix, Cube, Cube2 )\
    if ( (Matrix)->pOrderCubes )\
	for ( Cube = (Matrix)->pOrderCubes, Cube2 = ((Cube != (Lxu_Cube *)1)? Cube->pOrder: (Lxu_Cube *)1);\
          Cube != (Lxu_Cube *)1;\
		  Cube = Cube2, Cube2 = ((Cube != (Lxu_Cube *)1)? Cube->pOrder: (Lxu_Cube *)1) )
#define Lxu_MatrixForEachVarInRing( Matrix, Var )\
    if ( (Matrix)->pOrderVars )\
	for ( Var = (Matrix)->pOrderVars;\
          Var != (Lxu_Var *)1;\
		  Var = Var->pOrder )
#define Lxu_MatrixForEachVarInRingSafe( Matrix, Var, Var2 )\
    if ( (Matrix)->pOrderVars )\
	for ( Var = (Matrix)->pOrderVars, Var2 = ((Var != (Lxu_Var *)1)? Var->pOrder: (Lxu_Var *)1);\
          Var != (Lxu_Var *)1;\
		  Var = Var2, Var2 = ((Var != (Lxu_Var *)1)? Var->pOrder: (Lxu_Var *)1) )
// the procedures are related to the above macros
extern void Lxu_MatrixRingCubesUnmark( Lxu_Matrix * p );
extern void Lxu_MatrixRingVarsUnmark( Lxu_Matrix * p );


// macros working with memory
// MEM_ALLOC: allocate the given number (Size) of items of type (Type)
// MEM_FREE:  deallocate the pointer (Pointer) to the given number (Size) of items of type (Type)
#ifdef USE_SYSTEM_MEMORY_MANAGEMENT
#define MEM_ALLOC_LXU( Manager, Type, Size )          ((Type *)malloc( (Size) * sizeof(Type) ))
#define MEM_FREE_LXU( Manager, Type, Size, Pointer )  if ( Pointer ) { free(Pointer); Pointer = NULL; }
#else
#define MEM_ALLOC_LXU( Manager, Type, Size )\
        ((Type *)Lxu_MemFetch( Manager, (Size) * sizeof(Type) ))
#define MEM_FREE_LXU( Manager, Type, Size, Pointer )\
         if ( Pointer ) { Lxu_MemRecycle( Manager, (char *)(Pointer), (Size) * sizeof(Type) ); Pointer = NULL; }
#endif

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/*===== lxu.c ====================================================*/
extern char *       Lxu_MemFetch( Lxu_Matrix * p, int nBytes );
extern void         Lxu_MemRecycle( Lxu_Matrix * p, char * pItem, int nBytes );
/*===== lxuCreate.c ====================================================*/
/*===== lxuReduce.c ====================================================*/
/*===== lxuPrint.c ====================================================*/
extern void         Lxu_MatrixPrint( FILE * pFile, Lxu_Matrix * p );
extern void         Lxu_MatrixPrintDivisorProfile( FILE * pFile, Lxu_Matrix * p );
/*===== lxuSelect.c ====================================================*/
extern weightType   Lxu_Select( Lxu_Matrix * p, Lxu_Single ** ppSingle, Lxu_Double ** ppDouble );
extern weightType   Lxu_SelectSCD( Lxu_Matrix * p, weightType Weight, Lxu_Var ** ppVar1, Lxu_Var ** ppVar2 );
/*===== lxuUpdate.c ====================================================*/
extern void         Lxu_Update( Lxu_Matrix * p, Lxu_Single * pSingle, Lxu_Double * pDouble );
extern void         Lxu_UpdateDouble( Lxu_Matrix * p, Lxu_Double * d );
extern void         Lxu_UpdateSingle( Lxu_Matrix * p, Lxu_Single * s );
/*===== lxuPair.c ====================================================*/
extern void         Lxu_PairCanonicize( Lxu_Cube ** ppCube1, Lxu_Cube ** ppCube2 );
extern unsigned     Lxu_PairHashKeyArray( Lxu_Matrix * p, int piVarsC1[], int piVarsC2[], int nVarsC1, int nVarsC2 );
extern unsigned     Lxu_PairHashKey( Lxu_Matrix * p, Lxu_Cube * pCube1, Lxu_Cube * pCube2, int * pnBase, int * pnLits1, int * pnLits2 );
extern unsigned     Lxu_PairHashKeyMv( Lxu_Matrix * p, Lxu_Cube * pCube1, Lxu_Cube * pCube2, int * pnBase, int * pnLits1, int * pnLits2 );
extern int          Lxu_PairCompare( Lxu_Pair * pPair1, Lxu_Pair * pPair2 );
extern void         Lxu_PairAllocStorage( Lxu_Var * pVar, int nCubes );
extern void         Lxu_PairFreeStorage( Lxu_Var * pVar );
extern void         Lxu_PairClearStorage( Lxu_Cube * pCube );
extern Lxu_Pair *   Lxu_PairAlloc( Lxu_Matrix * p, Lxu_Cube * pCube1, Lxu_Cube * pCube2 );
extern void         Lxu_PairAdd( Lxu_Pair * pPair );
/*===== lxuSingle.c ====================================================*/
extern void         Lxu_MatrixComputeSingles( Lxu_Matrix * p );
extern void         Lxu_MatrixComputeSinglesOne( Lxu_Matrix * p, Lxu_Var * pVar );
extern int          Lxu_SingleCountCoincidence( Lxu_Matrix * p, Lxu_Var * pVar1, Lxu_Var * pVar2 );
/*===== lxuMatrix.c ====================================================*/
// matrix
extern Lxu_Matrix * Lxu_MatrixAllocate();
extern void         Lxu_MatrixDelete( Lxu_Matrix * p );
// double-cube divisor
extern void         Lxu_MatrixAddDivisor( Lxu_Matrix * p, Lxu_Cube * pCube1, Lxu_Cube * pCube2 );
extern void         Lxu_MatrixDelDivisor( Lxu_Matrix * p, Lxu_Double * pDiv );
// single-cube divisor
extern void          Lxu_MatrixAddSingle( Lxu_Matrix * p, Lxu_Var * pVar1, Lxu_Var * pVar2, weightType Weight );
// variable
extern Lxu_Var *    Lxu_MatrixAddVar( Lxu_Matrix * p );
// cube
extern Lxu_Cube *   Lxu_MatrixAddCube( Lxu_Matrix * p, Lxu_Var * pVar, int iCube );
// literal
extern void         Lxu_MatrixAddLiteral( Lxu_Matrix * p, Lxu_Cube * pCube, Lxu_Var * pVar );
extern void         Lxu_MatrixDelLiteral( Lxu_Matrix * p, Lxu_Lit * pLit );
/*===== lxuList.c ====================================================*/
// matrix -> variable 
extern void         Lxu_ListMatrixAddVariable( Lxu_Matrix * p, Lxu_Var * pVar );
extern void         Lxu_ListMatrixDelVariable( Lxu_Matrix * p, Lxu_Var * pVar );
// matrix -> cube
extern void         Lxu_ListMatrixAddCube( Lxu_Matrix * p, Lxu_Cube * pCube );
extern void         Lxu_ListMatrixDelCube( Lxu_Matrix * p, Lxu_Cube * pCube );
// matrix -> single
extern void         Lxu_ListMatrixAddSingle( Lxu_Matrix * p, Lxu_Single * pSingle );
extern void         Lxu_ListMatrixDelSingle( Lxu_Matrix * p, Lxu_Single * pSingle );
// table -> divisor
extern void         Lxu_ListTableAddDivisor( Lxu_Matrix * p, Lxu_Double * pDiv );
extern void         Lxu_ListTableDelDivisor( Lxu_Matrix * p, Lxu_Double * pDiv );
// cube -> literal 
extern void         Lxu_ListCubeAddLiteral( Lxu_Cube * pCube, Lxu_Lit * pLit );
extern void         Lxu_ListCubeDelLiteral( Lxu_Cube * pCube, Lxu_Lit * pLit );
// var -> literal
extern void         Lxu_ListVarAddLiteral( Lxu_Var * pVar, Lxu_Lit * pLit );
extern void         Lxu_ListVarDelLiteral( Lxu_Var * pVar, Lxu_Lit * pLit );
// divisor -> pair
extern void         Lxu_ListDoubleAddPairLast( Lxu_Double * pDiv, Lxu_Pair * pLink );
extern void         Lxu_ListDoubleAddPairFirst( Lxu_Double * pDiv, Lxu_Pair * pLink );
extern void         Lxu_ListDoubleAddPairMiddle( Lxu_Double * pDiv, Lxu_Pair * pSpot, Lxu_Pair * pLink );
extern void         Lxu_ListDoubleDelPair( Lxu_Double * pDiv, Lxu_Pair * pPair );
/*===== lxuHeapDouble.c ====================================================*/
extern Lxu_HeapDouble * Lxu_HeapDoubleStart();
extern void         Lxu_HeapDoubleStop( Lxu_HeapDouble * p );
extern void         Lxu_HeapDoublePrint( FILE * pFile, Lxu_HeapDouble * p );
extern void         Lxu_HeapDoubleCheck( Lxu_HeapDouble * p );
extern void         Lxu_HeapDoubleCheckOne( Lxu_HeapDouble * p, Lxu_Double * pDiv );

extern void         Lxu_HeapDoubleInsert( Lxu_HeapDouble * p, Lxu_Double * pDiv );  
extern void         Lxu_HeapDoubleUpdate( Lxu_HeapDouble * p, Lxu_Double * pDiv );  
extern void         Lxu_HeapDoubleDelete( Lxu_HeapDouble * p, Lxu_Double * pDiv );  
extern weightType   Lxu_HeapDoubleReadMaxWeight( Lxu_HeapDouble * p );  
extern Lxu_Double * Lxu_HeapDoubleReadMax( Lxu_HeapDouble * p );  
extern Lxu_Double * Lxu_HeapDoubleGetMax( Lxu_HeapDouble * p );  
/*===== lxuHeapSingle.c ====================================================*/
extern Lxu_HeapSingle * Lxu_HeapSingleStart();
extern void         Lxu_HeapSingleStop( Lxu_HeapSingle * p );
extern void         Lxu_HeapSinglePrint( FILE * pFile, Lxu_HeapSingle * p );
extern void         Lxu_HeapSingleCheck( Lxu_HeapSingle * p );
extern void         Lxu_HeapSingleCheckOne( Lxu_HeapSingle * p, Lxu_Single * pSingle );

extern void         Lxu_HeapSingleInsert( Lxu_HeapSingle * p, Lxu_Single * pSingle );  
extern void         Lxu_HeapSingleUpdate( Lxu_HeapSingle * p, Lxu_Single * pSingle );  
extern void         Lxu_HeapSingleDelete( Lxu_HeapSingle * p, Lxu_Single * pSingle );  
extern weightType   Lxu_HeapSingleReadMaxWeight( Lxu_HeapSingle * p );  
extern Lxu_Single * Lxu_HeapSingleReadMax( Lxu_HeapSingle * p );
extern Lxu_Single * Lxu_HeapSingleGetMax( Lxu_HeapSingle * p );  

/*===== lxuEuclid.c ====================================================*/
extern void Lxu_EuclidStart(weightType alpha, weightType beta, int interval);
extern void Lxu_EuclidStop();
extern void Lxu_EuclidPlaceAllAndComputeWeights(Lxu_Matrix *p);
extern weightType Lxu_EuclidCombine(weightType l, weightType p);
extern void Lxu_EuclidSetPoint (Lxu_Point *lv, const Lxu_Point *p);
extern int Lxu_EuclidGoodRect (Lxu_Rect *r);
extern void LxuEuclidComputePWeightDouble (Lxu_Matrix *p, Lxu_Double *pDiv);
extern void LxuEuclidComputePWeightSingle (Lxu_Matrix *p, Lxu_Single *pSingle);
extern coord Lxu_ComputeHPWL(Lxu_Matrix *p);

////////////////////////////////////////////////////////////////////////
///                         END OF FILE                              ///
////////////////////////////////////////////////////////////////////////
#endif


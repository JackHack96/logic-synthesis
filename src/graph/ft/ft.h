/**CFile****************************************************************

  FileName    [ft.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Data structure for algebraic factoring.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ft.h,v 1.3 2003/05/27 23:14:47 alanmi Exp $]

***********************************************************************/

#ifndef __FT_H__
#define __FT_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "mvc.h"
#include "vm.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

// the types of nodes in FFs
enum { FT_NODE_NONE, FT_NODE_AND, FT_NODE_OR, FT_NODE_INV, FT_NODE_LEAF, FT_NODE_0, FT_NODE_1 };

#define FT_MV_MASK(nValues)     ((~((unsigned)0)) >> (32-nValues))
#define FT_MV_FULL               (~((unsigned)0))

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

// factored forms
typedef struct FtListStruct        Ft_List_t;
typedef struct FtNodeStruct        Ft_Node_t;
typedef struct FtTreeStruct        Ft_Tree_t;


struct FtListStruct
{
    Ft_Node_t *       pHead;      // the head leaf
    Ft_Node_t *       pTail;      // the tail leaf
};

struct FtNodeStruct
{
    char              Type;       // LEAF/AND/OR
    char              nValues;    // the number of values of this MV var
    unsigned          VarNum:15;  // the number of the MV vars
    unsigned          fMark:1;    // the flag used to mark the leaf nodes
    unsigned          uData;      // the data (literal, simulation vector or BDD)
    Ft_Node_t *       pOne;       // the first branch
    Ft_Node_t *       pTwo;       // the second branch
};

struct FtTreeStruct
{
    Vm_VarMap_t *     pVm;        // the variable map of this tree
    int               nLeaves;    // the total number of input values
    int               nRoots;     // the number of i-sets
    Ft_Node_t **      pRoots;     // the roots of FF
    Ft_Node_t *       pDefault;   // the temporary storage for the def-value FF in ND networks
    unsigned *        uLeafData;  // the data assigned to the leaves
    unsigned *        uRootData;  // the data assigned to the roots
    int               nNodes;     // the number of nodes
    int               fMark;      // the multi-purpose mark
    Mvc_Manager_t *   pMem;       // the memory manager
};


////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

#define Ft_ForEachLeaf( List, Leaf )\
	for ( Leaf = List->pHead;\
          Leaf;\
		  Leaf = Leaf->pOne )
#define Ft_ForEachLeafSafe( Tree, Leaf, Leaf2 )\
	for ( Leaf = List->, Leaf2 = (Leaf? Leaf->pOne: NULL);\
          Leaf;\
		  Leaf = Leaf2, Leaf2 = (Leaf? Leaf->pOne: NULL) )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/*=== ftFactor.c =====================================================*/
extern Ft_Node_t *   Ft_Factor( Ft_Tree_t * pTree, Mvc_Cover_t * pCover );
/*=== ftTriv.c =======================================================*/
extern Ft_Node_t *   Ft_FactorTrivial( Ft_Tree_t * pTree, Mvc_Cover_t * pCover );
extern Ft_Node_t *   Ft_FactorTrivialCube( Ft_Tree_t * pTree, Mvc_Cover_t * pCover, Mvc_Cube_t * pCube );
extern Ft_Node_t *   Ft_FactorTrivialNode( Ft_Tree_t * pTree, int iLit );
/*=== ftTree.c =======================================================*/
extern Ft_Tree_t *   Ft_TreeCreate( Mvc_Manager_t * pMem, int nLeaves, int nRoots );
extern void          Ft_TreeFree( Ft_Tree_t * pTree );
extern void          Ft_TreeFreeRoot( Ft_Tree_t * pTree, int iRoot );
extern Ft_Node_t *   Ft_TreeNodeCreate( Ft_Tree_t * pTree, int Type, Ft_Node_t * pNode1, Ft_Node_t * pNode2 );
extern void          Ft_TreeNodeFree( Ft_Tree_t * pTree, Ft_Node_t * pNode );
extern void          Ft_TreeCountLeafRefs( Ft_Tree_t * pTree );
/*=== ftPrint.c ======================================================*/
extern void          Ft_TreePrint( FILE * pFile, Ft_Tree_t * pTree, char * pNamesIn[], char * pNameOut );
/*=== ftList.c ======================================================*/
extern void          Ft_ListAddLeaf( Ft_List_t * pList, Ft_Node_t * pLink ); 
extern void          Ft_ListDelLeaf( Ft_List_t * pList, Ft_Node_t * pLink );
/*=== ftTrans.c ======================================================*/
extern void          Ft_FactorTransform( Ft_Tree_t * pTree );
extern int           Ft_FactorCountLeaves( Ft_Tree_t * pTree );
extern int           Ft_FactorCountLeavesOne( Ft_Node_t * pRoot );
extern int           Ft_FactorCountLeafValues( Ft_Tree_t * pTree );
/*=== ftSop.c ======================================================*/
extern Mvc_Cover_t * Ft_Unfactor( Mvc_Manager_t * pMan, Ft_Tree_t * pTree, int iSet, bool fComplement );
extern int           Ft_UnfactorCount( Ft_Tree_t * pTree, int iSet, bool fCompl );


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

#endif


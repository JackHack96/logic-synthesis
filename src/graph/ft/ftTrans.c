/**CFile****************************************************************

  FileName    [ftFactor.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Procedures for algebraic factoring.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ftTrans.c,v 1.3 2003/05/27 23:14:48 alanmi Exp $]

***********************************************************************/

#include "ft.h"
#include "string.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void        Ft_FactorTransformOne( Ft_Tree_t * pTree, int i );
static void        Ft_FactorPrintRoot( Ft_Tree_t * pTree, Ft_Node_t * pRoot, int i );

static void        Ft_FactorMerge( Ft_Node_t * pRoot );
static void        Ft_FactorMerge_rec( Ft_Node_t * pNode, Ft_List_t * pList, Ft_Node_t ** ppPivot, int * pfType );

static Ft_Node_t * Ft_FactorSweep( Ft_Tree_t * pTree, Ft_Node_t * pRoot );
static Ft_Node_t * Ft_FactorSweep_rec( Ft_Tree_t * pTree, Ft_Node_t * pNode );

static void        Ft_FactorRaise( Ft_Node_t * pRoot );
static void        Ft_FactorLower( Ft_Node_t * pRoot );

static unsigned    Ft_FactorFreeValues( Ft_Node_t * pRoot, Ft_Node_t * pLeaf );
static bool        Ft_FactorFreeValues_rec( Ft_Node_t * pNode, Ft_Node_t * pLeaf, unsigned * pData );

static void        Ft_FactorLinkLeaves( Ft_List_t * pList, Ft_Node_t * pRoot );
static void        Ft_FactorLinkLeaves_rec( Ft_List_t * pList, Ft_Node_t * pNode );

static void        Ft_FactorCleanMark( Ft_Node_t * pRoot );
static void        Ft_FactorCleanMark_rec( Ft_Node_t * pNode );

static int         Ft_FactorCountLeavesOne_rec( Ft_Node_t * pNode );

static int         Ft_FactorCountLeafValuesOne( Ft_Node_t * pRoot );
static int         Ft_FactorCountLeafValuesOne_rec( Ft_Node_t * pNode );


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Simplifies the MV factored form.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ft_FactorTransform( Ft_Tree_t * pTree )
{
    int i;
    assert( pTree->nLeaves == Vm_VarMapReadValuesInNum(pTree->pVm) );

    // transforms the tree
    for ( i = 0; i < pTree->nRoots; i++ )
        if ( pTree->pRoots[i] )
            Ft_FactorTransformOne( pTree, i );

    // count the leaves
    Ft_FactorCountLeaves( pTree );
}


/**Function*************************************************************

  Synopsis    [Counts the total number of leaves in the tree.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ft_FactorCountLeaves( Ft_Tree_t * pTree )
{
    int i;
    // count the leaves
    pTree->nNodes = 0;
    for ( i = 0; i < pTree->nRoots; i++ )
        if ( pTree->pRoots[i] )
            pTree->nNodes += Ft_FactorCountLeavesOne( pTree->pRoots[i] );
    return pTree->nNodes;
}

/**Function*************************************************************

  Synopsis    [Counts the total number of leaves in the tree.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ft_FactorCountLeafValues( Ft_Tree_t * pTree )
{
    int nLeafValues;
    int i;
    // count the leaves
    nLeafValues = 0;
    for ( i = 0; i < pTree->nRoots; i++ )
        if ( pTree->pRoots[i] )
            nLeafValues += Ft_FactorCountLeafValuesOne( pTree->pRoots[i] );
    return nLeafValues;
}



/**Function*************************************************************

  Synopsis    [Simplifies the factored form assuming the MV variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ft_FactorTransformOne( Ft_Tree_t * pTree, int i )
{
    Ft_Node_t * pRoot;
    int fPrintOut = 0;

    // do not transform a trivial tree
    pRoot = pTree->pRoots[i];
    if ( pRoot->Type == FT_NODE_0 || pRoot->Type == FT_NODE_1 )
        return;
    if ( pRoot->Type == FT_NODE_LEAF )
        return;

    if ( fPrintOut ) Ft_FactorPrintRoot( pTree, pRoot, i );
    
    // merge the adjacent literals
    Ft_FactorMerge( pRoot );
    pRoot = Ft_FactorSweep( pTree, pRoot );
    if ( fPrintOut ) Ft_FactorPrintRoot( pTree, pRoot, i );

    // lower literals
    Ft_FactorLower( pRoot );
    pRoot = Ft_FactorSweep( pTree, pRoot );
    if ( fPrintOut ) Ft_FactorPrintRoot( pTree, pRoot, i );

    // raise literals
    Ft_FactorRaise( pRoot );
    pRoot = Ft_FactorSweep( pTree, pRoot );
    if ( fPrintOut ) Ft_FactorPrintRoot( pTree, pRoot, i );
/*    
    // merge the adjacent literals
    Ft_FactorMerge( pRoot );
    pRoot = Ft_FactorSweep( pTree, pRoot );
    if ( fPrintOut ) Ft_FactorPrintRoot( pTree, pRoot, i );
*/
/*
    // lower literals
    Ft_FactorLower( pRoot );
    pRoot = Ft_FactorSweep( pTree, pRoot );
    if ( fPrintOut ) Ft_FactorPrintRoot( pTree, pRoot, i );
*/

    pTree->pRoots[i] = pRoot;
}

/**Function*************************************************************

  Synopsis    [Print the tree.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ft_FactorPrintRoot( Ft_Tree_t * pTree, Ft_Node_t * pRoot, int i )
{
    pTree->pRoots[i] = pRoot;
    Ft_FactorCountLeaves( pTree );
    Ft_TreePrint( stdout, pTree, NULL, NULL );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Merges and sweeps the literals.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ft_FactorMerge( Ft_Node_t * pRoot )
{
    Ft_List_t lList, * pList = &lList;
    Ft_Node_t * pPivot, * pLeaf;
    bool fType; // 1 if AND; 0 if OR

    if ( pRoot->Type == FT_NODE_LEAF )
        return;

    Ft_FactorCleanMark( pRoot );
    while ( 1 )
    {
        // clean the list
        pList->pHead = NULL;
        pList->pTail = NULL;
        // clean the pivot
        pPivot = NULL;

        // collect the literals to be merged with the pivot
        fType = -1;
        Ft_FactorMerge_rec( pRoot, pList, &pPivot, &fType );
        if ( pPivot == NULL )
            break;

        // merge and mark the literals
        Ft_ForEachLeaf( pList, pLeaf )
        {
            assert( pLeaf->VarNum == pPivot->VarNum );
            if ( fType )
            {
                pPivot->uData &= pLeaf->uData;
                pLeaf->uData = FT_MV_MASK( pLeaf->nValues );
            }
            else
            {
                pPivot->uData |= pLeaf->uData;
                pLeaf->uData = 0;
            }
            pLeaf->fMark = 1;
        }
        // mark the pivot
        pPivot->fMark = 1;
    }
}

/**Function*************************************************************

  Synopsis    [Performs the recursive step of Ft_FactorMerge.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ft_FactorMerge_rec( Ft_Node_t * pNode, Ft_List_t * pList, Ft_Node_t ** ppPivot, int * pfType )
{
    Ft_Node_t * pNodeNext;
    bool fTypeCur;

    // get the current type
    assert( pNode->Type != FT_NODE_LEAF );
    fTypeCur = (int)(pNode->Type == FT_NODE_AND);

    if ( *pfType != -1 && *pfType != fTypeCur )
        return;

    // consider branch 1
    pNodeNext = pNode->pOne;
    if ( pNodeNext->Type != FT_NODE_LEAF )
        Ft_FactorMerge_rec( pNodeNext, pList, ppPivot, pfType );
    else if ( pNodeNext->fMark == 0 )
    { // this leaf has not been collected yet
        if ( *ppPivot == NULL )
        { // there is no pivot -> assign it
            *ppPivot = pNodeNext;
            *pfType = fTypeCur;
            // pivot is not added to the list
        }
        else if ( (*ppPivot)->VarNum == pNodeNext->VarNum && *pfType == fTypeCur )
        { // the leaf matches the pivot -> add the leaf to the list
            Ft_ListAddLeaf( pList, pNodeNext );
        }
    }

    if ( *pfType != -1 && *pfType != fTypeCur )
        return;

    // consider branch 2
    pNodeNext = pNode->pTwo;
    if ( pNodeNext->Type != FT_NODE_LEAF )
        Ft_FactorMerge_rec( pNodeNext, pList, ppPivot, pfType );
    else if ( pNodeNext->fMark == 0 )
    { // this leaf has not been collected yet
        if ( *ppPivot == NULL )
        { // there is no pivot -> assign it
            *ppPivot = pNodeNext;
            *pfType = fTypeCur;
            // pivot is not added to the list
        }
        else if ( (*ppPivot)->VarNum == pNodeNext->VarNum && *pfType == fTypeCur )
        { // the leaf matches the pivot -> add the leaf to the list
            Ft_ListAddLeaf( pList, pNodeNext );
        }
    }
}


/**Function*************************************************************

  Synopsis    [Merges and sweeps the literals.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ft_Node_t * Ft_FactorSweep( Ft_Tree_t * pTree, Ft_Node_t * pRoot )
{
    // do not transform a trivial tree
    assert ( pRoot->Type != FT_NODE_0 && pRoot->Type != FT_NODE_1 );
    return Ft_FactorSweep_rec( pTree, pRoot );
}

/**Function*************************************************************

  Synopsis    [Performs the recursive step of Ft_FactorSweep.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ft_Node_t * Ft_FactorSweep_rec( Ft_Tree_t * pTree, Ft_Node_t * pNode )
{
    Ft_Node_t * pNodeNew;
    if ( pNode->Type == FT_NODE_LEAF )
    {
        if ( pNode->uData == 0 || 
             pNode->uData == FT_MV_MASK( pNode->nValues ) )
        {
            Ft_TreeNodeFree( pTree, pNode );
            return NULL;
        }
        return pNode;
    }
    pNode->pOne = Ft_FactorSweep_rec( pTree, pNode->pOne );
    pNode->pTwo = Ft_FactorSweep_rec( pTree, pNode->pTwo );
    if ( pNode->pOne == NULL && pNode->pTwo == NULL )
    {
        Ft_TreeNodeFree( pTree, pNode );
        return NULL;
    }
    else if ( pNode->pOne == NULL )
    {
        pNodeNew = pNode->pTwo;
        Ft_TreeNodeFree( pTree, pNode );
        return pNodeNew;
    }
    else if ( pNode->pTwo == NULL )
    {
        pNodeNew = pNode->pOne;
        Ft_TreeNodeFree( pTree, pNode );
        return pNodeNew;
    }
    return pNode;
}

/**Function*************************************************************

  Synopsis    [Raise all literals of the FF.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ft_FactorRaise( Ft_Node_t * pRoot )
{
    Ft_List_t lList, * pList = &lList;
    Ft_Node_t * pLeaf;
    unsigned uFreeValues;

    Ft_FactorLinkLeaves( pList, pRoot );
    Ft_ForEachLeaf( pList, pLeaf )
        if ( pLeaf->nValues > 2 )
        {
            uFreeValues = Ft_FactorFreeValues( pRoot, pLeaf );
            pLeaf->uData |=  uFreeValues;
        }
}

/**Function*************************************************************

  Synopsis    [Lower all literals of the FF.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ft_FactorLower( Ft_Node_t * pRoot )
{
    Ft_List_t lList, * pList = &lList;
    Ft_Node_t * pLeaf;
    unsigned uFreeValues;

    Ft_FactorLinkLeaves( pList, pRoot );
    Ft_ForEachLeaf( pList, pLeaf )
        if ( pLeaf->nValues > 2 )
        {
            uFreeValues = Ft_FactorFreeValues( pRoot, pLeaf );
            pLeaf->uData &= ~uFreeValues;
        }
}

/**Function*************************************************************

  Synopsis    [Raise one literal.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Ft_FactorFreeValues( Ft_Node_t * pRoot, Ft_Node_t * pLeaf )
{
    unsigned uFreeValues;
    bool RetValue;
    assert( pLeaf->Type == FT_NODE_LEAF );
    // call recursively to determine incompatible values
    RetValue = Ft_FactorFreeValues_rec( pRoot, pLeaf, &uFreeValues );
    assert( RetValue );
    return uFreeValues;
}

/**Function*************************************************************

  Synopsis    [Recursively computes the free values of one literal.]

  Description [Returns 1 if the literal is on this path; 0 otherwise.
  In the variable pData, returns the set of values that can be added 
  to the given literal. Does not modify the values of the literals.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Ft_FactorFreeValues_rec( Ft_Node_t * pNode, Ft_Node_t * pLeaf, unsigned * pData )
{
    unsigned uData1, uData2;
    bool Res1, Res2; 

    if ( pNode->Type == FT_NODE_LEAF )
    {
        if ( pNode == pLeaf )
            // skip the leaf if it is the one being considered
            *pData = 0; 
        else if ( pNode->VarNum == pLeaf->VarNum )
            // the values to be added are the complement of the current value set
            *pData = pNode->uData ^ FT_MV_MASK( pNode->nValues );
        else // this is a different variable -> nothing can be added
            *pData = 0;
        return (int)( pNode == pLeaf );
    }

    // call recursively
    Res1 = Ft_FactorFreeValues_rec( pNode->pOne, pLeaf, &uData1 );
    Res2 = Ft_FactorFreeValues_rec( pNode->pTwo, pLeaf, &uData2 );
    assert( Res1 == 0 || Res2 == 0 );
    assert( pNode->Type == FT_NODE_AND || pNode->Type == FT_NODE_OR );

    // combine the solutions
    if ( pNode->Type == FT_NODE_AND )
        *pData = uData1 | uData2;
    else 
    {
        if ( Res1 )
            *pData = uData1;
        else if ( Res2 )
            *pData = uData2;
        else
            *pData = uData1 & uData2;
    }
    return Res1 || Res2;    
}




/**Function*************************************************************

  Synopsis    [Links the leaves into a linked list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ft_FactorLinkLeaves( Ft_List_t * pList, Ft_Node_t * pRoot )
{
    if ( pRoot->Type == FT_NODE_0 || pRoot->Type == FT_NODE_1 )
        return;
    pList->pHead = NULL;
    pList->pTail = NULL;
    Ft_FactorLinkLeaves_rec( pList, pRoot );
}

/**Function*************************************************************

  Synopsis    [Recursive part of Ft_FactorLinkLeaves().]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ft_FactorLinkLeaves_rec( Ft_List_t * pList, Ft_Node_t * pNode )
{
    assert( pNode->Type != FT_NODE_NONE );
    if ( pNode->Type != FT_NODE_LEAF )
    {
        Ft_FactorLinkLeaves_rec( pList, pNode->pOne );
        Ft_FactorLinkLeaves_rec( pList, pNode->pTwo );
    }
    else
    {
        Ft_ListAddLeaf( pList, pNode );
    }
}

/**Function*************************************************************

  Synopsis    [Cleans the marks of the leaf nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ft_FactorCleanMark( Ft_Node_t * pRoot )
{
    if ( pRoot->Type == FT_NODE_0 || pRoot->Type == FT_NODE_1 )
        return;
    Ft_FactorCleanMark_rec( pRoot );
}

/**Function*************************************************************

  Synopsis    [Recursive part of Ft_FactorCleanMark().]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ft_FactorCleanMark_rec( Ft_Node_t * pNode )
{
    assert( pNode->Type != FT_NODE_NONE );
    if ( pNode->Type != FT_NODE_LEAF )
    {
        Ft_FactorCleanMark_rec( pNode->pOne );
        Ft_FactorCleanMark_rec( pNode->pTwo );
    }
    pNode->fMark = 0;
}


/**Function*************************************************************

  Synopsis    [Count the number of leaf nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ft_FactorCountLeavesOne( Ft_Node_t * pRoot )
{
    if ( pRoot->Type == FT_NODE_0 || pRoot->Type == FT_NODE_1 )
        return 0;
    return Ft_FactorCountLeavesOne_rec( pRoot );
}

/**Function*************************************************************

  Synopsis    [Recursive part of Ft_FactorCountLeavesOne().]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ft_FactorCountLeavesOne_rec( Ft_Node_t * pNode )
{
    assert( pNode->Type != FT_NODE_NONE );
    if ( pNode->Type != FT_NODE_LEAF )
        return Ft_FactorCountLeavesOne_rec( pNode->pOne ) + 
               Ft_FactorCountLeavesOne_rec( pNode->pTwo );
    return 1;
}


/**Function*************************************************************

  Synopsis    [Count the number of leaf nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ft_FactorCountLeafValuesOne( Ft_Node_t * pRoot )
{
    if ( pRoot->Type == FT_NODE_0 || pRoot->Type == FT_NODE_1 )
        return 0;
    return Ft_FactorCountLeafValuesOne_rec( pRoot );
}

/**Function*************************************************************

  Synopsis    [Recursive part of Ft_FactorCountLeavesOne().]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ft_FactorCountLeafValuesOne_rec( Ft_Node_t * pNode )
{
    int i, Counter;
    assert( pNode->Type != FT_NODE_NONE );
    if ( pNode->Type != FT_NODE_LEAF )
        return Ft_FactorCountLeafValuesOne_rec( pNode->pOne ) + 
               Ft_FactorCountLeafValuesOne_rec( pNode->pTwo );
    Counter = 0;
    for ( i = 0; i < pNode->nValues; i++ )
        Counter += (int)((pNode->uData & (1<<i)) > 0);
    return Counter;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



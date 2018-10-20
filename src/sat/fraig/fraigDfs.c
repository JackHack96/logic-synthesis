/**CFile****************************************************************

  FileName    [fraigDfs.c]

  PackageName [FRAIG: Functionally reduced AND-INV graphs.]

  Synopsis    [DFS traversal of the graph.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.1. Started - October 1, 2004]

  Revision    [$Id: fraigDfs.c,v 1.7 2005/07/08 01:01:31 alanmi Exp $]

***********************************************************************/

#include "fraigInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void  Fraig_DfsReverse_rec( Fraig_Node_t * pNode, Fraig_NodeVec_t * vNodes );
static void  Fraig_DfsLim_rec( Fraig_Node_t * pNode, int Level, Fraig_NodeVec_t * vNodes );
static int   Fraig_CountLevels_rec( Fraig_Node_t * pNode );
static void  Fraig_Unmark_rec( Fraig_Node_t * pNode );
static int   Fraig_CountNodes_rec( Fraig_Node_t * pNode );
static void  Fraig_Dfs2_rec( Fraig_Node_t * pNode, Fraig_NodeVec_t * vNodes, int fEquiv );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes the DFS ordering of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_NodeVec_t * Fraig_Dfs( Fraig_Man_t * pMan, int fEquiv )
{
    Fraig_NodeVec_t * vNodes;
    int i;
    // perform the traversal
    vNodes = Fraig_NodeVecAlloc( 100 );
    for ( i = 0; i < pMan->nOutputs; i++ )
        Fraig_Dfs_rec( Fraig_Regular(pMan->pOutputs[i]), vNodes, fEquiv );
    for ( i = 0; i < vNodes->nSize; i++ )
        vNodes->pArray[i]->fMark0 = 0;
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Computes the DFS ordering of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_NodeVec_t * Fraig_DfsOne( Fraig_Man_t * pMan, Fraig_Node_t * pNode, int fEquiv )
{
    Fraig_NodeVec_t * vNodes;
    int i;
    // perform the traversal
    vNodes = Fraig_NodeVecAlloc( 100 );
    Fraig_Dfs_rec( Fraig_Regular(pNode), vNodes, fEquiv );
    for ( i = 0; i < vNodes->nSize; i++ )
        vNodes->pArray[i]->fMark0 = 0;
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Computes the DFS ordering of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_NodeVec_t * Fraig_DfsNodes( Fraig_Man_t * pMan, Fraig_Node_t ** ppNodes, int nNodes, int fEquiv )
{
    Fraig_NodeVec_t * vNodes;
    int i;
    // perform the traversal
    vNodes = Fraig_NodeVecAlloc( 100 );
    for ( i = 0; i < nNodes; i++ )
        Fraig_Dfs_rec( Fraig_Regular(ppNodes[i]), vNodes, fEquiv );
    for ( i = 0; i < vNodes->nSize; i++ )
        vNodes->pArray[i]->fMark0 = 0;
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Recursively computes the DFS ordering of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_Dfs_rec( Fraig_Node_t * pNode, Fraig_NodeVec_t * vNodes, int fEquiv )
{
    assert( !Fraig_IsComplement(pNode) );
    if ( pNode->fMark0 )
        return;
    pNode->fMark0 = 1;
    // visit the transitive fanin
    if ( Fraig_NodeIsAnd(pNode) )
    {
        Fraig_Dfs_rec( Fraig_Regular(pNode->p1), vNodes, fEquiv );
        Fraig_Dfs_rec( Fraig_Regular(pNode->p2), vNodes, fEquiv );
    }
    if ( fEquiv && pNode->pNextE )
        Fraig_Dfs_rec( pNode->pNextE, vNodes, fEquiv );
    // save the node
    Fraig_NodeVecPush( vNodes, pNode );
}

/**Function*************************************************************

  Synopsis    [Recursively computes the DFS ordering of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_Dfs2_rec( Fraig_Node_t * pNode, Fraig_NodeVec_t * vNodes, int fEquiv )
{
    assert( !Fraig_IsComplement(pNode) );
    if ( pNode->fMark1 )
        return;
    pNode->fMark1 = 1;
    // visit the transitive fanin
    if ( Fraig_NodeIsAnd(pNode) )
    {
        Fraig_Dfs2_rec( Fraig_Regular(pNode->p1), vNodes, fEquiv );
        Fraig_Dfs2_rec( Fraig_Regular(pNode->p2), vNodes, fEquiv );
    }
    if ( fEquiv && pNode->pNextE )
        Fraig_Dfs2_rec( pNode->pNextE, vNodes, fEquiv );
    // save the node if it is not yet saved
    if ( !pNode->fMark0 )
        Fraig_NodeVecPush( vNodes, pNode );
}


/**Function*************************************************************

  Synopsis    [Computes the reverse-DFS ordering of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_NodeVec_t * Fraig_DfsReverse( Fraig_Man_t * pMan )
{
    Fraig_NodeVec_t * vNodes;
    int i;
    // perform the traversal
    vNodes = Fraig_NodeVecAlloc( 100 );
    for ( i = 0; i < pMan->nInputs; i++ )
        Fraig_DfsReverse_rec( pMan->vAnds->pArray[i], vNodes );
    for ( i = 0; i < vNodes->nSize; i++ )
        vNodes->pArray[i]->fMark0 = 0;
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Recursively computes the reverse-DFS ordering of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_DfsReverse_rec( Fraig_Node_t * pNode, Fraig_NodeVec_t * vNodes )
{
    Fraig_Node_t * pFanout;
    assert( !Fraig_IsComplement(pNode) );
    if ( pNode->fMark0 )
        return;
    pNode->fMark0 = 1;
    // visit the transitive fanin
    Fraig_NodeForEachFanout( pNode, pFanout )
        Fraig_DfsReverse_rec( pFanout, vNodes );
    // set the number of levels
    Fraig_NodeVecPush( vNodes, pNode );
}

/**Function*************************************************************

  Synopsis    [Counts the number of nodes reachable from the primary outputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_CountNodes( Fraig_Man_t * pMan )
{
    int nNodes, i;
    nNodes = 0;
    for ( i = 0; i < pMan->nOutputs; i++ )
        nNodes += Fraig_CountNodes_rec( Fraig_Regular(pMan->pOutputs[i]) );
    for ( i = 0; i < pMan->nOutputs; i++ )
        Fraig_Unmark_rec( Fraig_Regular(pMan->pOutputs[i]) );
    return nNodes;
}

/**Function*************************************************************

  Synopsis    [Counts the number of nodes reachable from the given node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_CountNodesOne( Fraig_Man_t * pMan, Fraig_Node_t * pNode )
{
    int nNodes;
    nNodes = Fraig_CountNodes_rec( Fraig_Regular(pNode) );
    Fraig_Unmark_rec( Fraig_Regular(pNode) );
    return nNodes;
}

/**Function*************************************************************

  Synopsis    [Counts the number of nodes reachable from the given array of nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_CountNodesArray( Fraig_Man_t * pMan, Fraig_Node_t ** ppNodes, int nNodes )
{
    int nNodeCount, i;
    nNodeCount = 0;
    for ( i = 0; i < nNodes; i++ )
        nNodeCount += Fraig_CountNodes_rec( Fraig_Regular(ppNodes[i]) );
    for ( i = 0; i < nNodes; i++ )
        Fraig_Unmark_rec( Fraig_Regular(ppNodes[i]) );
    return nNodeCount;
}

/**Function*************************************************************

  Synopsis    [Computes the number of logic levels not counting PIs/POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_CountNodes_rec( Fraig_Node_t * pNode )
{
    assert( !Fraig_IsComplement(pNode) );
    if ( pNode->fMark0 )
        return 0;
    if ( !Fraig_NodeIsAnd(pNode) )
        return 0;
    pNode->fMark0 = 1;
    return 1 + Fraig_CountNodes_rec( Fraig_Regular(pNode->p1) ) +
               Fraig_CountNodes_rec( Fraig_Regular(pNode->p2) );
}

/**Function*************************************************************

  Synopsis    [Computes the number of logic levels not counting PIs/POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_CountLevels( Fraig_Man_t * pMan )
{
    int i, LevelsMax, LevelsCur;
    // perform the traversal
    LevelsMax = -1;
    for ( i = 0; i < pMan->nOutputs; i++ )
    {
        LevelsCur = Fraig_CountLevels_rec( Fraig_Regular(pMan->pOutputs[i]) );
        if ( LevelsMax < LevelsCur )
            LevelsMax = LevelsCur;
    }
    for ( i = 0; i < pMan->nOutputs; i++ )
        Fraig_Unmark_rec( Fraig_Regular(pMan->pOutputs[i]) );
    return LevelsMax;
}

/**Function*************************************************************

  Synopsis    [Computes the number of logic levels not counting PIs/POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_CountLevelsNodes( Fraig_Man_t * pMan, Fraig_Node_t ** ppRoots, int nRoots )
{
    int i, LevelsMax, LevelsCur;
    // perform the traversal
    LevelsMax = -1;
    for ( i = 0; i < nRoots; i++ )
    {
        LevelsCur = Fraig_CountLevels_rec( Fraig_Regular(ppRoots[i]) );
        if ( LevelsMax < LevelsCur )
            LevelsMax = LevelsCur;
    }
    for ( i = 0; i < nRoots; i++ )
        Fraig_Unmark_rec( Fraig_Regular(ppRoots[i]) );
    return LevelsMax;
}

/**Function*************************************************************

  Synopsis    [Recursively computes the number of logic levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_CountLevels_rec( Fraig_Node_t * pNode )
{
    Fraig_Node_t * pTemp;
    unsigned Level;
    assert( !Fraig_IsComplement(pNode) );
    if ( !Fraig_NodeIsAnd(pNode) )
    {
        pNode->NumTemp = 0;
        return 0;
    }
    if ( pNode->fMark0 )
        return pNode->NumTemp;
    pNode->fMark0 = 1;
    // visit the functinally equivalent nodes
    pNode->NumTemp = 0;
    if ( pNode->pNextE )
    {
        Level = Fraig_CountLevels_rec( pNode->pNextE );
        if ( pNode->NumTemp < Level )
            pNode->NumTemp = Level;         
    }
    // visit the transitive fanin
    Level = 1 + Fraig_CountLevels_rec( Fraig_Regular(pNode->p1) );
    if ( pNode->NumTemp < Level )
        pNode->NumTemp = Level;         
    Level = 1 + Fraig_CountLevels_rec( Fraig_Regular(pNode->p2) );
    if ( pNode->NumTemp < Level )
        pNode->NumTemp = Level;  
    // set the same level for all the equivalent nodes
    for ( pTemp = pNode->pNextE; pTemp; pTemp = pTemp->pNextE )
        pTemp->NumTemp = pNode->NumTemp;
    return pNode->NumTemp;
}

/**Function*************************************************************

  Synopsis    [Unmarks the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_Unmark( Fraig_Man_t * pMan )
{
    int i;
    for ( i = 0; i < pMan->nOutputs; i++ )
        Fraig_Unmark_rec( Fraig_Regular(pMan->pOutputs[i]) );
}

/**Function*************************************************************

  Synopsis    [Recursively unmarks the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_Unmark_rec( Fraig_Node_t * pNode )
{
    assert( !Fraig_IsComplement(pNode) );
    if ( pNode->fMark0 == 0 )
        return;
    pNode->fMark0 = 0;
    if ( !Fraig_NodeIsAnd(pNode) )
        return;
    if ( pNode->pNextE )
        Fraig_Unmark_rec( pNode->pNextE );
    Fraig_Unmark_rec( Fraig_Regular(pNode->p1) );
    Fraig_Unmark_rec( Fraig_Regular(pNode->p2) );
}




/**Function*************************************************************

  Synopsis    [Computes the common TFI cone of two nodes.]

  Description [Returns 1 if the p2 is in the TFI of p1.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_DfsForOne( Fraig_Man_t * pMan, Fraig_Node_t * p1 )
{
    Fraig_NodeVec_t * vNodes;
    Sat_IntVec_t * vNums;
    int i;
    // collect the nodes
    vNodes = pMan->vNodes;
    Fraig_NodeVecClear( vNodes );
    Fraig_Dfs_rec( p1, vNodes, 1 );
    // remove the marks and copy the node numbers
    vNums = pMan->vVarsInt;
    Sat_IntVecClear( vNums );
    for ( i = 0; i < vNodes->nSize; i++ )
    {
        vNodes->pArray[i]->fMark0 = 0;
        Sat_IntVecPush( vNums, vNodes->pArray[i]->Num ); 
    }
}

/**Function*************************************************************

  Synopsis    [Computes the union of the TFI cone of two nodes.]

  Description [Returns 1 if the p2 is in the TFI of p1.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_DfsForTwo( Fraig_Man_t * pMan, Fraig_Node_t * p1, Fraig_Node_t * p2 )
{
    Fraig_NodeVec_t * vNodes;
    Sat_IntVec_t * vNums;
    int i, RetValue;
    // collect the nodes
    vNodes = pMan->vNodes;
    Fraig_NodeVecClear( vNodes );
    Fraig_Dfs_rec( p1, vNodes, 1 );
    // check if p2 is in the TFI of p1
    RetValue = (p2->fMark0 == 1);
    Fraig_Dfs_rec( p2, vNodes, 1 );
    // remove the marks and copy the node numbers
    vNums = pMan->vVarsInt;
    Sat_IntVecClear( vNums );
    for ( i = 0; i < vNodes->nSize; i++ )
    {
        vNodes->pArray[i]->fMark0 = 0;
        Sat_IntVecPush( vNums, vNodes->pArray[i]->Num ); 
    }
//printf( " %d", Sat_IntVecReadSize(vNums) );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Computes the union of the TFI cones of two nodes minus the common part.]

  Description [Returns 1 if the p2 is in the TFI of p1.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_DfsForTwoMinus( Fraig_Man_t * pMan, Fraig_Node_t * p1, Fraig_Node_t * p2 )
{
    Fraig_NodeVec_t * vNodes;
    Sat_IntVec_t * vNums;
    Fraig_Node_t * pFanout;
    int i, RetValue;
    int fAddToSet;
    // collect the nodes
    vNodes = pMan->vNodes;
    Fraig_NodeVecClear( vNodes );
    Fraig_Dfs_rec( p1, vNodes, 1 );
    // check if p2 is in the TFI of p1
    RetValue = (p2->fMark0 == 1);
    Fraig_Dfs2_rec( p2, vNodes, 1 );
    // remove the marks and copy the node numbers
    vNums = pMan->vVarsInt;
    Sat_IntVecClear( vNums );
    for ( i = 0; i < vNodes->nSize; i++ )
    {
        // save the node if it is marked once, or if it has a fanout that is marked once
        if ( vNodes->pArray[i]->fMark0 ^ vNodes->pArray[i]->fMark1 )
            fAddToSet = 1;
        else
        {
            fAddToSet = 0;
            Fraig_NodeForEachFanout( vNodes->pArray[i], pFanout )
            {
                if ( pFanout->fMark0 ^ pFanout->fMark1 )
                {
                    fAddToSet = 1;
                    break;
                }
            }
        }

        if ( fAddToSet )
            Sat_IntVecPush( vNums, vNodes->pArray[i]->Num ); 

        vNodes->pArray[i]->fMark0 = 0;
        vNodes->pArray[i]->fMark1 = 0;
    }
//printf( " %d", Sat_IntVecReadSize(vNums) );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Computes the limited DFS ordering for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_NodeVec_t * Fraig_DfsLim( Fraig_Man_t * pMan, Fraig_Node_t * pNode, int nLevels )
{
    Fraig_NodeVec_t * vNodes;
    int i;
    // perform the traversal
    vNodes = Fraig_NodeVecAlloc( 100 );
    Fraig_DfsLim_rec( pNode, nLevels, vNodes );
    for ( i = 0; i < vNodes->nSize; i++ )
        vNodes->pArray[i]->fMark0 = 0;
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Recursively computes the DFS ordering of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_DfsLim_rec( Fraig_Node_t * pNode, int Level, Fraig_NodeVec_t * vNodes )
{
    assert( !Fraig_IsComplement(pNode) );
    if ( pNode->fMark0 )
        return;
    pNode->fMark0 = 1;
    // visit the transitive fanin
    Level--;
    if ( Level > 0 && Fraig_NodeIsAnd(pNode) )
    {
        Fraig_DfsLim_rec( Fraig_Regular(pNode->p1), Level, vNodes );
        Fraig_DfsLim_rec( Fraig_Regular(pNode->p2), Level, vNodes );
    }
    // add the node to the list
    Fraig_NodeVecPush( vNodes, pNode );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



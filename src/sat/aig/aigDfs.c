/**CFile****************************************************************

  FileName    [aigDfs.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [AND-INV graph package with built in sat_sweep.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: aigDfs.c,v 1.2 2004/07/29 04:54:47 alanmi Exp $]

***********************************************************************/

#include "aigInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void  Aig_Dfs_rec( Aig_Node_t * pNode, Aig_NodeVec_t * vNodes );
static int   Aig_CountLevels_rec( Aig_Node_t * pNode );
static void  Aig_Unmark_rec( Aig_Node_t * pNode );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes the DFS ordering of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_NodeVec_t * Aig_Dfs( Aig_Man_t * pMan )
{
    Aig_NodeVec_t * vNodes;
    int i;
    // perform the traversal
    vNodes = Aig_NodeVecAlloc( 100 );
    for ( i = 0; i < pMan->nOutputs; i++ )
        Aig_Dfs_rec( Aig_Regular(pMan->pOutputs[i]), vNodes );
    for ( i = 0; i < pMan->nOutputs; i++ )
        Aig_Unmark_rec( Aig_Regular(pMan->pOutputs[i]) );
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Recursively computes the DFS ordering of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_Dfs_rec( Aig_Node_t * pNode, Aig_NodeVec_t * vNodes )
{
    assert( !Aig_IsComplement(pNode) );
    if ( pNode->fMark0 )
        return;
    pNode->fMark0 = 1;
    // visit the transitive fanin
    if ( Aig_NodeIsAnd(pNode) )
    {
        Aig_Dfs_rec( Aig_Regular(pNode->p1), vNodes );
        Aig_Dfs_rec( Aig_Regular(pNode->p2), vNodes );
    }
    // set the number of levels
    Aig_NodeVecPush( vNodes, pNode );
}

/**Function*************************************************************

  Synopsis    [Computes the number of logic levels not counting PIs/POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_CountLevels( Aig_Man_t * pMan )
{
    int i, LevelsMax, LevelsCur;
    // perform the traversal
    LevelsMax = -1;
    for ( i = 0; i < pMan->nOutputs; i++ )
    {
        LevelsCur = Aig_CountLevels_rec( Aig_Regular(pMan->pOutputs[i]) );
        if ( LevelsMax < LevelsCur )
            LevelsMax = LevelsCur;
    }
    for ( i = 0; i < pMan->nOutputs; i++ )
        Aig_Unmark_rec( Aig_Regular(pMan->pOutputs[i]) );
    return LevelsMax;
}

/**Function*************************************************************

  Synopsis    [Recursively computes the number of logic levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_CountLevels_rec( Aig_Node_t * pNode )
{
    int Level1, Level2;
    assert( !Aig_IsComplement(pNode) );
    if ( !Aig_NodeIsAnd(pNode) )
    {
        pNode->NumTemp = 0;
        return 0;
    }
    if ( pNode->fMark0 )
        return pNode->NumTemp;
    pNode->fMark0 = 1;
    // visit the transitive fanin
    Level1 = Aig_CountLevels_rec( Aig_Regular(pNode->p1) );
    Level2 = Aig_CountLevels_rec( Aig_Regular(pNode->p2) );
    // set the number of levels
    pNode->NumTemp = 1 + ((Level1>Level2)? Level1: Level2);
    return pNode->NumTemp;
}

/**Function*************************************************************

  Synopsis    [Unmarks the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_Unmark( Aig_Man_t * pMan )
{
    int i;
    for ( i = 0; i < pMan->nOutputs; i++ )
        Aig_Unmark_rec( Aig_Regular(pMan->pOutputs[i]) );
}

/**Function*************************************************************

  Synopsis    [Recursively unmarks the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_Unmark_rec( Aig_Node_t * pNode )
{
    assert( !Aig_IsComplement(pNode) );
    if ( pNode->fMark0 == 0 )
        return;
    pNode->fMark0 = 0;
    if ( !Aig_NodeIsAnd(pNode) )
        return;
    Aig_Unmark_rec( Aig_Regular(pNode->p1) );
    Aig_Unmark_rec( Aig_Regular(pNode->p2) );
}




/**Function*************************************************************

  Synopsis    [Computes the common TFI cone of two nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_DfsForTwo( Aig_Man_t * pMan, Aig_Node_t * p1, Aig_Node_t * p2 )
{
    Aig_NodeVec_t * vNodes;
    Sat_IntVec_t * vNums;
    int i;
    // collect the nodes
    vNodes = pMan->vNodes;
    Aig_NodeVecClear( vNodes );
    Aig_Dfs_rec( p1, vNodes );
    Aig_Dfs_rec( p2, vNodes );
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

  Synopsis    [Prints the AI-graph nodes in the list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_PrintNodeList( Aig_Man_t * pMan, Sat_IntVec_t * vNodes )
{
    Aig_Node_t * pNode;
    int i, nSize, * pArray;

    nSize  = Sat_IntVecReadSize( vNodes );
    pArray = Sat_IntVecReadArray( vNodes );
    for ( i = 0; i < nSize; i++ )
    {
        pNode = pMan->pVec->pArray[ pArray[i] ];
        if ( Aig_NodeIsAnd(pNode) )
        {
            printf( "Node  %2d%s * %2d%s = %2d.   %2d\n",  
                Aig_Regular(pNode->p1)->Num, Aig_IsComplement(pNode->p1)? "\'": " ",  
                Aig_Regular(pNode->p2)->Num, Aig_IsComplement(pNode->p2)? "\'": " ", 
                pNode->Num, pNode->NumTemp );
        }
        else
            printf( "Node  %2d  * %2d  = %2d.   %2d\n",  
                -1, -1, pNode->Num, pNode->NumTemp );
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



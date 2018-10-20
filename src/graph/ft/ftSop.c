/**CFile****************************************************************

  FileName    [ftSop.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Converting of FFs or their complements into SOPs.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ftSop.c,v 1.2 2003/05/27 23:14:48 alanmi Exp $]

***********************************************************************/

#include "ft.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Mvc_Cover_t * Ft_Unfactor_rec( Mvc_Data_t * pData, Ft_Node_t * pNode, bool fCompl );
static int Ft_UnfactorCount_rec( Ft_Node_t * pNode, bool fCompl );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Cover_t * Ft_Unfactor( Mvc_Manager_t * pMan, Ft_Tree_t * pTree, int iSet, bool fCompl )
{
    Mvc_Cover_t * pCover;
    Mvc_Data_t * pData;

    assert( pTree->pRoots[iSet] );
    pCover = Mvc_CoverAlloc( pMan, Vm_VarMapReadValuesInNum(pTree->pVm) );
    pData  = Mvc_CoverDataAlloc( pTree->pVm, pCover );
    Mvc_CoverFree( pCover );

    pCover = Ft_Unfactor_rec( pData, pTree->pRoots[iSet], fCompl );
    Mvc_CoverDataFree( pData, pCover );
    return pCover;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Cover_t * Ft_Unfactor_rec( Mvc_Data_t * pData, Ft_Node_t * pNode, bool fCompl )
{
    Mvc_Cube_t * pCube;
    Mvc_Cover_t * pCover;
    Mvc_Cover_t * pCover2;
    if ( pNode->Type == FT_NODE_LEAF )
    {
        int iFirstBit, v;

        // start the cover with one cube
        pCover = Mvc_CoverAlloc( pData->pMan, Vm_VarMapReadValuesInNum(pData->pVm) );
        pCube = Mvc_CubeAlloc( pCover );
        Mvc_CoverAddCubeTail( pCover, pCube );

        // fill in the cube
        Mvc_CubeBitFill( pCube );
        iFirstBit = Vm_VarMapReadValuesFirst( pData->pVm, pNode->VarNum );
        // set the values of this literal
        if ( fCompl )
        {
            for ( v = 0; v < pNode->nValues; v++ )
                if ( pNode->uData & (1 << v) )
                    Mvc_CubeBitRemove( pCube, iFirstBit + v );
        }
        else
        {
            for ( v = 0; v < pNode->nValues; v++ )
                if ( (pNode->uData & (1 << v)) == 0 )
                    Mvc_CubeBitRemove( pCube, iFirstBit + v );
        }
        return pCover;
    }
    if ( pNode->Type == FT_NODE_AND && !fCompl || pNode->Type == FT_NODE_OR && fCompl )
    {
        pCover  = Ft_Unfactor_rec( pData, pNode->pOne, fCompl ); 
        pCover2 = Ft_Unfactor_rec( pData, pNode->pTwo, fCompl ); 
        Mvc_CoverIntersectCubes( pData, pCover, pCover2 );
        Mvc_CoverFree( pCover2 );
        return pCover;
    }
    if ( pNode->Type == FT_NODE_OR && !fCompl || pNode->Type == FT_NODE_AND && fCompl )
    {
        pCover  = Ft_Unfactor_rec( pData, pNode->pOne, fCompl ); 
        pCover2 = Ft_Unfactor_rec( pData, pNode->pTwo, fCompl ); 
        Mvc_CoverAppendCubes( pCover, pCover2 );
        Mvc_CoverFree( pCover2 );
        return pCover;
    }
    if ( pNode->Type == FT_NODE_0 && !fCompl || pNode->Type == FT_NODE_1 && fCompl )
        return Mvc_CoverAlloc( pData->pMan, Vm_VarMapReadValuesInNum(pData->pVm) );
    if ( pNode->Type == FT_NODE_1 && !fCompl || pNode->Type == FT_NODE_0 && fCompl )
    {
        pCover = Mvc_CoverAlloc( pData->pMan, Vm_VarMapReadValuesInNum(pData->pVm) );
        pCube = Mvc_CubeAlloc( pCover );
        Mvc_CoverAddCubeTail( pCover, pCube );
        Mvc_CubeBitFill( pCube );
        return pCover;
    }
    assert( 0 ); // unknown node type
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Counts the number of cubes in the FF after complementing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ft_UnfactorCount( Ft_Tree_t * pTree, int iSet, bool fCompl )
{
    assert( pTree->pRoots[iSet] );
    return Ft_UnfactorCount_rec( pTree->pRoots[iSet], fCompl );
}


/**Function*************************************************************

  Synopsis    [Counts the number of cubes in the FF after complementing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ft_UnfactorCount_rec( Ft_Node_t * pNode, bool fCompl )
{
    int Count;
    if ( pNode->Type == FT_NODE_LEAF )
        return 1;
    if ( pNode->Type == FT_NODE_AND && !fCompl || pNode->Type == FT_NODE_OR && fCompl )
    {
        Count = Ft_UnfactorCount_rec( pNode->pOne, fCompl ) *  
                Ft_UnfactorCount_rec( pNode->pTwo, fCompl );
        if ( Count < 10000 )
            return Count;
        return 10000;
    }
    if ( pNode->Type == FT_NODE_OR && !fCompl || pNode->Type == FT_NODE_AND && fCompl )
    {
        Count = Ft_UnfactorCount_rec( pNode->pOne, fCompl ) +  
                Ft_UnfactorCount_rec( pNode->pTwo, fCompl );
        if ( Count < 10000 )
            return Count;
        return 10000;
    }
    if ( pNode->Type == FT_NODE_0 && !fCompl || pNode->Type == FT_NODE_1 && fCompl )
        return 0;
    if ( pNode->Type == FT_NODE_1 && !fCompl || pNode->Type == FT_NODE_0 && fCompl )
        return 1;
    assert( 0 ); // unknown node type
    return 0;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



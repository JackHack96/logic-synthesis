/**CFile****************************************************************

  FileName    [fmbFlexRel.c]

  PackageName [Binary flexibility manager.]

  Synopsis    [Complete flexibility computation (relations).]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 3.0. Started - September 27, 2003.]

  Revision    [$Id: fmbFlexRel.c,v 1.1 2003/11/18 18:55:09 alanmi Exp $]

***********************************************************************/

#include "fmbInt.h"
#include "fmm.h"
 
////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Get the flexibility in the global space.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Fmb_FlexComputeGlobalRel( Fmb_Manager_t * pMan )
{
    DdManager * dd = pMan->ddGlo;
//    Ntk_Node_t * pNodeCo;
    Ntk_Node_t * pNode;
    DdNode * bFunc, * bFuncZ;
    DdNode * bRel, * bCond, * bTemp;
    Ntk_Node_t ** ppRoots;
    int nRoots, i;

    ppRoots = Wn_WindowReadRoots( pMan->pWnd );
    nRoots = Wn_WindowReadRootNum( pMan->pWnd );

    Extra_OperationTimeoutSet( 100 );

    // create the equivalence condition
    bRel = b1;   Cudd_Ref( bRel );
    // go through the outputs
//    Ntk_NetworkForEachCo( pMan->pNet, pNode )
//    Ntk_NetworkForEachCoDriver( pMan->pNet, pNodeCo, pNode )
    for ( i = 0; i < nRoots; i++ )
    {
        pNode = ppRoots[i];

        // get the modified functions
        bFuncZ = Ntk_NodeReadFuncBinGloZ( pNode );
        if ( bFuncZ == NULL )
            continue;
        // get the global functions of the spec
//        bFunc = Ntk_NodeReadFuncBinGlo( pNode );
        bFunc = Ntk_NodeReadFuncBinGlo( pNode );
        assert( bFunc != NULL );

        // get the condition  (bFunc == bFuncZ)
        bCond = Cudd_bddXnor( dd, bFunc, bFuncZ );  
        if ( bCond == NULL )
        {
            Cudd_RecursiveDeref( dd, bRel );
            Extra_OperationTimeoutReset();
            return NULL;
        }
        Cudd_Ref( bCond );

        // multiply this condition by the general condition
        bRel = Extra_bddAnd( dd, bTemp = bRel, bCond );     
        if ( bRel == NULL )
        {
            Cudd_RecursiveDeref( dd, bTemp );
            Cudd_RecursiveDeref( dd, bCond );
            Extra_OperationTimeoutReset();
            return NULL;
        }
        Cudd_Ref( bRel );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bCond );
    }
    Extra_OperationTimeoutReset();
    Cudd_Deref( bRel );
    return bRel;
}


/**Function*************************************************************

  Synopsis    [Get the flexibility in the local space.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvr_Relation_t * Fmb_FlexComputeLocalRel( Fmb_Manager_t * pMan, DdNode * bFlexGlo, Ntk_Node_t ** ppFanins, int nFanins )
{
    Mvr_Manager_t * pMvrMan;
    Mvr_Relation_t * pMvrLoc;
    DdManager * ddGlo;      // bFlexGlo is expressed using this manager
    DdManager * ddLoc;      // the resulting relation uses this manager
    DdNode * bTemp, * bFunc, * bImage;
    int * pPermute, i;

    pMvrMan = Ntk_NetworkReadManMvr( pMan->pNet );

    // get the global and local BDD managers
    ddGlo = pMan->ddGlo;
    ddLoc = Mvr_ManagerReadDdLoc(pMvrMan);

    // set the global functions of the fanins
    for ( i = 0; i < nFanins; i++ )
    {
        bFunc = Ntk_NodeReadFuncBinGlo( ppFanins[i] );
        pMan->pbArray[2*i+0] = Cudd_Not(bFunc);
        pMan->pbArray[2*i+1] = bFunc;
    }

    bImage = Fmm_ImageMvCompute( ddGlo, Cudd_Not(bFlexGlo), pMan->pbArray, 2*nFanins, 
                pMan->pVmxG, pMan->pVmxL, pMan->pbVars1, 100 ); 
    bImage = Cudd_Not( bImage ); // ImageMv does not complement image!!!
    Cudd_Ref( bImage );
//PRB( ddGlo, bFlexGlo );
//PRB( ddGlo, bImage );

    // transfer from the FM's own global manager into the MVR's local manager
    pPermute = Fmb_UtilsCreatePermMapRel( pMan );
    bImage = Extra_TransferPermute( ddGlo, ddLoc, bTemp = bImage, pPermute );
    Cudd_Ref( bImage );
    Cudd_RecursiveDeref( ddGlo, bTemp );
    FREE( pPermute );
//PRB( ddLoc, bImage );

    // create the relation
    pMvrLoc = Mvr_RelationCreate( pMvrMan, pMan->pVmxL, bImage ); 
    Cudd_Deref( bImage );
    // reorder the relation
//    Mvr_RelationReorder( pMvrLoc );
    return pMvrLoc;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



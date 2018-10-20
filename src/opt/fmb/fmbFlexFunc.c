/**CFile****************************************************************

  FileName    [fmbFlexFunc.c]

  PackageName [Binary flexibility manager.]

  Synopsis    [Complete flexibility computation (functions).]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 3.0. Started - September 27, 2003.]

  Revision    [$Id: fmbFlexFunc.c,v 1.1 2003/11/18 18:55:09 alanmi Exp $]

***********************************************************************/

#include "fmbInt.h"
 
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
DdNode * Fmb_FlexComputeGlobal( Fmb_Manager_t * pMan )
{
    DdManager * dd = pMan->ddGlo;
//    Ntk_Node_t * pNodeCo;
    Ntk_Node_t * pNode;
    DdNode * bFuncZ;
    DdNode * bSenseGlob, * bSense, * bTemp;
    int nInputs;
    Ntk_Node_t ** ppRoots;
    int nRoots, i;

    ppRoots = Wn_WindowReadRoots( pMan->pWnd );
    nRoots = Wn_WindowReadRootNum( pMan->pWnd );

    Extra_OperationTimeoutSet( 100 );

    nInputs = Vm_VarMapReadVarsInNum( Vmx_VarMapReadVm(pMan->pVmxG) );

    // create the equivalence condition
    bSenseGlob = b0;   Cudd_Ref( bSenseGlob );
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
//        bFunc = Ntk_NodeReadFuncBinGlo( pNode );
//        assert( bFunc != NULL );
//PRB( dd, bFuncZ );

//        Extra_OperationTimeoutSet( BDD_OPERATION_TIMEOUT );
        bSense = Extra_bddBooleanDiff( dd, bFuncZ, pMan->pbVars0[nInputs]->index ); 
        if ( bSense == NULL )
        {
            Cudd_RecursiveDeref( dd, bSenseGlob );
            return NULL;
        }
        Cudd_Ref( bSense );
//PRB( dd, bSense );

        // multiply this condition by the general condition
        bSenseGlob = Extra_bddOr( dd, bTemp = bSenseGlob, bSense );     
        if ( bSenseGlob == NULL )
        {
            Cudd_RecursiveDeref( dd, bTemp );
            Cudd_RecursiveDeref( dd, bSense );
            Extra_OperationTimeoutReset();
            return NULL;
        }
        Cudd_Ref( bSenseGlob );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bSense );
    }
    Extra_OperationTimeoutReset();
    Cudd_Deref( bSenseGlob );
//PRB( dd, Cudd_Not( bSenseGlob ) );
    return Cudd_Not( bSenseGlob );
}


/**Function*************************************************************

  Synopsis    [Get the flexibility in the local space.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvr_Relation_t * Fmb_FlexComputeLocal( Fmb_Manager_t * pMan, Ntk_Node_t * pNode, DdNode * bFlexGlo, Ntk_Node_t ** ppFanins, int nFanins )
{
    Mvr_Manager_t * pMvrMan;
    Mvr_Relation_t * pMvrLoc;
    DdManager * ddGlo;      // bFlexGlo is expressed using this manager
    DdManager * ddLoc;      // the resulting relation uses this manager
    DdNode * bTemp, * bImage;
    int * pPermute, i;
//    int * pTransMap;
    Cvr_Cover_t * pCvr;

    pMvrMan = Ntk_NetworkReadManMvr( pMan->pNet );

    // get the global and local BDD managers
    ddGlo = pMan->ddGlo;
    ddLoc = Mvr_ManagerReadDdLoc(pMvrMan);

    for ( i = 0; i < nFanins; i++ )
        pMan->pbArray[i] = Ntk_NodeReadFuncBinGlo( ppFanins[i] );

    bImage = Fmb_ImageCompute( ddGlo, pMan->pbArray, pMan->pbVars1, 
        Cudd_Not(bFlexGlo), nFanins, 100 ); 
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
//    pMvrLoc = Mvr_RelationCreate( pMvrMan, pMan->pVmxL, bImage ); 
//    Cudd_Deref( bImage );
    // reorder the relation
//    Mvr_RelationReorder( pMvrLoc );

/*
    pMvrLoc = Ntk_NodeGetFuncMvr(pNode);
    pMvrLoc = Mvr_RelationDup( pMvrLoc );

    // remap the image to match the map of the relation
    pTransMap = ALLOC( int, nFanins + 1 );
    for ( i = 0; i < nFanins; i++ )
        pTransMap[i] = i;
    pTransMap[nFanins] = -1; // unused

    bImage = Mvr_RelationRemap( pMvrLoc, bTemp = bImage, 
        pMan->pVmxL, Mvr_RelationReadVmx(pMvrLoc), pTransMap );  Cudd_Ref( bImage );
    FREE( pTransMap );

    Mvr_RelationAddDontCare( pMvrLoc, bImage );
    Cudd_RecursiveDeref( ddLoc, bImage );
    Cudd_RecursiveDeref( ddLoc, bTemp );
*/

    pCvr = Ntk_NodeGetFuncCvr(pNode);
    pMvrLoc = Fnc_FunctionDeriveMvrFromCvr( pMvrMan,
        Ntk_NetworkReadManVmx( pMan->pNet ), pCvr );

    Mvr_RelationAddDontCare( pMvrLoc, bImage );
    Cudd_RecursiveDeref( ddLoc, bImage );

    return pMvrLoc;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



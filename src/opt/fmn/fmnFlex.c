/**CFile****************************************************************

  FileName    [Dcmn_Flex.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Complete flexibility computation without windowing.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: fmnFlex.c,v 1.2 2003/11/18 18:55:12 alanmi Exp $]

***********************************************************************/

#include "fmnInt.h"
 
////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int *            Dcmn_FlexCreatePermMap( Dcmn_Man_t * pMan );

extern int timeGlobal;
extern int timeContain;
extern int timeImage;
extern int timeImagePrep;
extern int timeResub;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Get the flexibility in the global space.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Dcmn_FlexComputeGlobal( Dcmn_Man_t * pMan )
{
    DdManager * dd = pMan->ddGlo;
    Vm_VarMap_t * pVm;
    Ntk_Node_t * pNode;
    DdNode ** pbFuncs, ** pbFuncsZ;
    DdNode * bRel, * bCond, * bTemp;
    int * pValues, * pValuesFirst;
    int v, nVarsIn, nVarsOut;

//    Extra_OperationTimeoutSet( 100 );

    // get parameters
    pVm          = Vmx_VarMapReadVm( pMan->pVmx );
    nVarsIn      = Vm_VarMapReadVarsInNum( pVm );
    nVarsOut     = Vm_VarMapReadVarsOutNum( pVm );
    pValues      = Vm_VarMapReadValuesArray( pVm )      + nVarsIn;
    pValuesFirst = Vm_VarMapReadValuesFirstArray( pVm ) + nVarsIn;

    // create the containment condition
    bRel = b1;   Cudd_Ref( bRel );
    // go through the outputs
    Ntk_NetworkForEachCo( pMan->pNet, pNode )
//    for ( i = 0; i < nVarsOut; i++ )
    {
        // get the modified functions
        pbFuncsZ = Ntk_NodeReadFuncGlobZ( pNode );
        if ( pbFuncsZ[0] == NULL )
            continue;
        // get the global functions of the spec
//        pbFuncs = Ntk_NodeReadFuncGlob( pNode );
        pbFuncs = Ntk_NodeReadFuncGlobS( pNode );
        assert( pbFuncs[0] != NULL );


        for ( v = 0; v < pNode->nValues; v++ )
        {
            // get the condition  (bFuncZ => bFunc)
//            bCond = Cudd_bddOr( dd, Cudd_Not(bFuncZ), bFunc ); Cudd_Ref( bCond );
            bCond = Extra_bddAnd( dd, pbFuncsZ[v], Cudd_Not(pbFuncs[v]) );  
            if ( bCond == NULL )
            {
                Cudd_RecursiveDeref( dd, bRel );
                Extra_OperationTimeoutReset();
                return NULL;
            }
            bCond = Cudd_Not( bCond );
            Cudd_Ref( bCond );
//PRB( dd, pbFuncsZ[v] );
//PRB( dd, pbFuncs[v] );

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
    }
    Extra_OperationTimeoutReset();
    Cudd_Deref( bRel );
//PRB( dd, bRel );
    return bRel;
}



/**Function*************************************************************

  Synopsis    [Get the flexibility in the global space.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Dcmn_FlexComputeGlobalOne( Dcmn_Man_t * pMan, Ntk_Node_t * pNode )
{
    DdManager * dd = pMan->ddGlo;
    Vm_VarMap_t * pVm;
    DdNode ** pbFuncs, ** pbFuncsZ;
    DdNode * bRel, * bCond, * bTemp;
    int * pValues, * pValuesFirst;
    int v, nVarsIn, nVarsOut;

//    Extra_OperationTimeoutSet( 100 );

    // get parameters
    pVm          = Vmx_VarMapReadVm( pMan->pVmx );
    nVarsIn      = Vm_VarMapReadVarsInNum( pVm );
    nVarsOut     = Vm_VarMapReadVarsOutNum( pVm );
    pValues      = Vm_VarMapReadValuesArray( pVm )      + nVarsIn;
    pValuesFirst = Vm_VarMapReadValuesFirstArray( pVm ) + nVarsIn;

    // get the modified functions
    pbFuncsZ = Ntk_NodeReadFuncGlobZ( pNode );
    if ( pbFuncsZ[0] == NULL )
        return b1;

    // get the global functions of the spec
//  pbFuncs = Ntk_NodeReadFuncGlob( pNode );
    pbFuncs = Ntk_NodeReadFuncGlobS( pNode );
    assert( pbFuncs[0] != NULL );

    bRel = b1; Cudd_Ref( bRel );
    for ( v = 0; v < pNode->nValues; v++ )
    {
        // get the condition  (bFuncZ => bFunc)
        bCond = Extra_bddAnd( dd, pbFuncsZ[v], Cudd_Not(pbFuncs[v]) );  
        if ( bCond == NULL )
        {
            Cudd_RecursiveDeref( dd, bRel );
            Extra_OperationTimeoutReset();
            return NULL;
        }
        bCond = Cudd_Not( bCond );
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

  Synopsis    [Get the flexibility in the global space.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Dcmn_FlexComputeGlobalUseSpecNs( Dcmn_Man_t * pMan, DdNode * bSpecNs )
{
    DdManager * dd = pMan->ddGlo;
    DdNode * bSpecNsZ;
    DdNode * bCube;
    DdNode * bGlobal;

    // get the Z relation
    bSpecNsZ = Dcmn_UtilsNetworkDeriveRelation( pMan, 1 );  Cudd_Ref(bSpecNsZ);
//PRB( dd, bSpecNs );
//PRB( dd, bSpecNsZ );
    // get the cube of output variables
    bCube = Vmx_VarMapCharCubeOutput( dd, pMan->pVmx );  Cudd_Ref( bCube );
//PRB( dd, bCube );
    // assert containment and quantify the output vars
    // GloFlex = ForAll outs ( SpecNsZ => SpecNs )
    // GloFlex = NOT( Exist outs ( SpecNsZ * NOT(SpecNs) ) )
    bGlobal = Cudd_bddAndAbstract( dd, bSpecNsZ, Cudd_Not(bSpecNs), bCube ); Cudd_Ref( bGlobal );
    bGlobal = Cudd_Not( bGlobal );
//PRB( dd, bGlobal );
    Cudd_RecursiveDeref( dd, bCube );
    Cudd_RecursiveDeref( dd, bSpecNsZ );
    Cudd_Deref( bGlobal );
    return bGlobal;
}



/**Function*************************************************************

  Synopsis    [Get the flexibility in the local space.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvr_Relation_t * Dcmn_FlexComputeLocal( Dcmn_Man_t * pMan, DdNode * bFlexGlo, Ntk_Node_t ** ppFanins, int nFanins )
{
    Mvr_Manager_t * pMvrMan;
    Mvr_Relation_t * pMvrLoc;
    DdManager * ddGlo;      // bFlexGlo is expressed using this manager
    DdManager * ddLoc;      // the resulting relation uses this manager
    DdNode * bTemp, * bImage;
    int * pPermute;
    int clk, clkP;

clkP = clock();

    pMvrMan = Ntk_NetworkReadManMvr( pMan->pNet );

    // get the global and local BDD managers
    ddGlo = pMan->ddGlo;
    ddLoc = Mvr_ManagerReadDdLoc(pMvrMan);


clk = clock();
    // compute the MV image
//    if ( pMan->pPars->fUseNs || pMan->pPars->fUseNsc )
    if ( 1 )
    {
        bImage = Dcmn_UtilsNetworkImageNs( pMan, Cudd_Not(bFlexGlo), ppFanins, nFanins );
        Cudd_Ref( bImage ); // Image Ns complements image
//PRS( pMan->ddGlo, bImage );
    }
    else
    {
        assert( 0 );
        /*
        nValues = 0;
        for ( i = 0; i < nFanins; i++ )
        {
            pbFuncs = Ntk_NodeReadFuncGlob( ppFanins[i] );
            Dcmn_UtilsCopyFuncs( pMan->pbArray + nValues, pbFuncs, ppFanins[i]->nValues );
            nValues += ppFanins[i]->nValues;
        }
        bImage = Dcmn_ImageMvCompute( ddGlo, Cudd_Not(bFlexGlo), pMan->pbArray, nValues, 
                    pMan->pVmxG, pMan->pVmxL, pMan->pbVars1, 100 ); 
        Cudd_Ref( bImage );
        bImage = Cudd_Not( bImage ); // ImageMv does not complement image!!!

        for ( i = 0; i < nValues; i++ )
        {
//        PRB( ddGlo, pMan->pbArray[i] );
        }
        */
    }
clk = clock() - clk;

    // transfer from the FM's own global manager into the MVR's local manager
    pPermute = Dcmn_FlexCreatePermMap( pMan );
    bImage = Extra_TransferPermute( ddGlo, ddLoc, bTemp = bImage, pPermute );
    Cudd_Ref( bImage );
    Cudd_RecursiveDeref( ddGlo, bTemp );
    FREE( pPermute );

    // create the relation
    pMvrLoc = Mvr_RelationCreate( pMvrMan, pMan->pVmxL, bImage ); 
    Cudd_Deref( bImage );
    // reorder the relation
//    Mvr_RelationReorder( pMvrLoc );

clkP = clock() - clkP - clk;

timeImage     += clk;
timeImagePrep += clkP;

    return pMvrLoc;
}

/**Function*************************************************************

  Synopsis    [Get the flexibility in the local space.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvr_Relation_t * Dcmn_FlexComputeLocalUseProduct( Dcmn_Man_t * pMan, Ntk_Node_t ** ppFanins, int nFanins, Ntk_Node_t * pNode )
{
    Mvr_Manager_t * pMvrMan;
    Mvr_Relation_t * pMvrLoc;
    DdManager * ddGlo;      // bFlexGlo is expressed using this manager
    DdManager * ddLoc;      // the resulting relation uses this manager
//    DdNode ** pbFuncs;
    DdNode * bTemp, * bImage;
    int * pPermute;
//    int i;
    int clk, clkP;
//    int nValues;

clkP = clock();

    pMvrMan = Ntk_NetworkReadManMvr( pMan->pNet );

    // get the global and local BDD managers
    ddGlo = pMan->ddGlo;
    ddLoc = Mvr_ManagerReadDdLoc(pMvrMan);


clk = clock();
    // compute the MV image
//    if ( pMan->pPars->fUseNs || pMan->pPars->fUseNsc )
    if ( 1 )
    {
        bImage = Dcmn_UtilsNetworkImageNsUseProduct( pMan, ppFanins, nFanins, pNode );
        Cudd_Ref( bImage );  // complements inside
    }
    else
    {
        assert( 0 );
/*
        nValues = 0;
        for ( i = 0; i < nFanins; i++ )
        {
            pbFuncs = Ntk_NodeReadFuncGlob( ppFanins[i] );
            Dcmn_UtilsCopyFuncs( pMan->pbArray + nValues, pbFuncs, ppFanins[i]->nValues );
            nValues += ppFanins[i]->nValues;
        }
        bImage = Dcmn_ImageMvCompute( ddGlo, Cudd_Not(bFlexGlo), pMan->pbArray, nValues, 
                    pMan->pVmxG, pMan->pVmxL, pMan->pbVars1, 100 ); 
        Cudd_Ref( bImage );
        bImage = Cudd_Not( bImage );
        for ( i = 0; i < nValues; i++ )
        {
//        PRB( ddGlo, pMan->pbArray[i] );
        }
*/
    }
clk = clock() - clk;


//PRB( ddGlo, bFlexGlo );
//PRB( ddGlo, bImage );


    // transfer from the FM's own global manager into the MVR's local manager
    pPermute = Dcmn_FlexCreatePermMap( pMan );
    bImage = Extra_TransferPermute( ddGlo, ddLoc, bTemp = bImage, pPermute );
    Cudd_Ref( bImage );
    Cudd_RecursiveDeref( ddGlo, bTemp );
    FREE( pPermute );

    // create the relation
    pMvrLoc = Mvr_RelationCreate( pMvrMan, pMan->pVmxL, bImage ); 
    Cudd_Deref( bImage );
    // reorder the relation
//    Mvr_RelationReorder( pMvrLoc );

clkP = clock() - clkP - clk;

timeImage     += clk;
timeImagePrep += clkP;

    return pMvrLoc;
}

/**Function*************************************************************

  Synopsis    [Composes the special node functions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
/*
int * Dcmn_FlexCreatePermMap( Dcmn_Man_t * pMan )
{
    DdManager * dd = pMan->ddGlo;
    int * pPermute;
    int i, iVar, nBits;

    // allocate the permutation map
    pPermute = ALLOC( int, dd->size );
    for ( i = 0; i < dd->size; i++ )
        pPermute[i] = -1;

    // get the parameters
    iVar = 0;
    nBits = Vmx_VarMapReadBitsNum( pMan->pVmxL );
    for ( i = 0; i < nBits; i++ )
        pPermute[pMan->pbVars1[i]->index] = iVar++;
    return pPermute;
}
*/

int * Dcmn_FlexCreatePermMap( Dcmn_Man_t * pMan )
{
    DdManager * dd = pMan->ddGlo;
    int * pPermute, * pBitOrder, * pBitsFirst;
    int i, iVar, nBits;
    int nVarsIn;

    // allocate the permutation map
    pPermute = ALLOC( int, dd->size );
    for ( i = 0; i < dd->size; i++ )
        pPermute[i] = -1;

    // get the parameters
    pBitsFirst = Vmx_VarMapReadBitsFirst( pMan->pVmxL );
    nVarsIn    = Vm_VarMapReadVarsInNum( Vmx_VarMapReadVm(pMan->pVmxL) );

    // add the input variables
    iVar = 0;
    for ( i = 0; i < pBitsFirst[nVarsIn]; i++ )
        pPermute[dd->size/2 + i] = iVar++;
//        pPermute[pMan->nVars0 + i] = iVar++;

    // get the parameters
    pBitOrder  = Vmx_VarMapReadBitsOrder( pMan->pVmxG );
    pBitsFirst = Vmx_VarMapReadBitsFirst( pMan->pVmxG );
    nVarsIn    = Vm_VarMapReadVarsInNum( Vmx_VarMapReadVm(pMan->pVmxG) );
    nBits      = Vmx_VarMapReadBitsNum( pMan->pVmxG );

    // add the output variables
    for ( i = pBitsFirst[nVarsIn]; i < nBits; i++ )
        pPermute[pBitOrder[i]] = iVar++;
    return pPermute;
}




////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



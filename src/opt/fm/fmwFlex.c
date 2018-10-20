/**CFile****************************************************************

  FileName    [fmwFlex.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Complete flexibility computation with windowing.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: fmwFlex.c,v 1.3 2003/05/27 23:15:36 alanmi Exp $]

***********************************************************************/

#include "fmInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Mvr_Relation_t * fmwFlexReturnTrivial( Sh_Network_t * pNet, Mvr_Relation_t * pMvr );
static bool             fmwFlexComputeGlobalBdds( Fmw_Manager_t * pMan, DdManager * dd, Sh_Network_t * pNet );
static DdNode *         fmwFlexComputeGlobal( Fmw_Manager_t * pMan, Sh_Network_t * pNet );
static Mvr_Relation_t * fmwFlexComputeLocal( Fmw_Manager_t * pMan, DdNode * bFlexGlo, Sh_Network_t * pNet, Mvr_Relation_t * pMvr );
static DdNode *         fmwFlexDeriveGlobalRelation( Fmw_Manager_t * pMan, Sh_Network_t * pNet );
static DdNode *         fmwFlexComposeSpecials( Fmw_Manager_t * pMan, DdNode * bFlex, Vmx_VarMap_t * pVmx, Sh_Network_t * pNet );
static int *            fmwFlexCreatePermMap( Fmw_Manager_t * pMan );
static DdNode *         fmwFlexRemap( DdManager * dd, DdNode * bFlex, DdNode * pbFuncs[], DdNode * pbCodes[], int nValues, DdNode * bCubeChar );
static DdNode *         fmwFlexConvolve( DdManager * dd, DdNode * pbFuncs[], DdNode * pbCodes[], int nValues );
static DdNode *         fmwFlexTransferFromSetOutputs( Fmw_Manager_t * pMan, DdNode * bFlexGlo );
static int              fmwFlexGetGlobalBddSize( Sh_Network_t * pNet );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvr_Relation_t * Fm_FlexibilityCompute( Fmw_Manager_t * pMan, Sh_Network_t * pNet, Mvr_Relation_t * pMvr )
{
    DdManager * dd = pMan->dd;
    Mvr_Relation_t * pMvrLoc;
    Vm_VarMap_t * pVm;
    DdNode * bFlexGlo, * bTemp;

    assert( pNet->nInputsCore > 0 && pNet->nOutputsCore > 0 );

//    Sh_NodePrintArray( pNet->ppOutputsCore, pNet->nOutputsCore );

    // the computation of flexibility is not needed
    // if the structural hashing has derived constant functions for all i-sets
    // here we check this condition
    if ( pMvrLoc = fmwFlexReturnTrivial( pNet, pMvr ) )
    {
        if ( pMan->fVerbose )
            printf( "Strashing has derived a constant %d-valued node.\n", pNet->nOutputsCore );
        return pMvrLoc;
    }

    // create the internal list of nodes in the interleaved DFS order
    Sh_NetworkInterleaveNodes( pNet );;
    // clean the data fields of the nodes in the list
    Sh_NetworkCleanData( pNet );

    // create the encoded local variable map
    pVm = Vm_VarMapCreateInputOutput( pNet->pVmLC, pNet->pVmRC );
    pMan->pVmxLoc = Vmx_VarMapLookup( pMan->pManVmx, pVm, -1, NULL );
    // create the encoded global variable map
    pVm = Vm_VarMapCreateInputOutput( pNet->pVmL, pNet->pVmRC );
    pMan->pVmxGlo = Vmx_VarMapLookup( pMan->pManVmx, pVm, -1, NULL );
 
    // create the encoded local variable map with sets
    if ( pMan->fSetOutputs )
    { // create the set-output map
        pVm = Vm_VarMapCreateInputOutputSet( pNet->pVmL, pNet->pVmRC, 0, 1 );
        pMan->pVmxGloS = Vmx_VarMapLookup( pMan->pManVmx, pVm, -1, NULL );
        pMan->pVmxGloS = Fm_UtilsCreateVarOrdering( pMan->pVmxGloS, pNet );
    }
    else
    {
        pMan->pVmxGlo = Fm_UtilsCreateVarOrdering( pMan->pVmxGlo, pNet );
    }

    // resize the manager
    Fmw_ManagerResize( pMan, Vmx_VarMapReadBitsNum(pMan->pVmxLoc) );
    Fmw_ManagerResize( pMan, Vmx_VarMapReadBitsNum(pMan->pVmxGlo) );
    if ( pMan->pVmxGloS )
        Fmw_ManagerResize( pMan, Vmx_VarMapReadBitsNum(pMan->pVmxGloS) );

    // set the var encoding cubes at the nodes
    if ( !pMan->fSetOutputs )
        Fm_UtilsAssignLeaves( dd, pMan->pbVars0, pNet, pMan->pVmxGlo, 0 );
    else
        Fm_UtilsAssignLeavesSet( dd, pMan->pbVars0, pNet, pMan->pVmxGloS, 0 );

    // compute the global BDDs
    Sh_NetworkDfs( pNet );
    if ( !fmwFlexComputeGlobalBdds( pMan, dd, pNet ) )
    {
        Fm_UtilsDerefNetwork( dd, pNet );
        return NULL;
    }
//printf( "Global BDD size = %d\n", fmwFlexGetGlobalBddSize(pNet) );
    // derive the containment condition from both pairs
    bFlexGlo = fmwFlexComputeGlobal( pMan, pNet );  
    if ( bFlexGlo == NULL )
    {
        Fm_UtilsDerefNetwork( dd, pNet );
        return NULL;

        if ( pMan->fVerbose )
            printf( "Global flexbility comptutation has timed out!\n" );
        bFlexGlo = fmwFlexDeriveGlobalRelation( pMan, pNet );
        Cudd_Ref( bFlexGlo );
    }
    else
    {
       Cudd_Ref( bFlexGlo );
       if ( pMan->fSetOutputs )
       {
           bFlexGlo = fmwFlexTransferFromSetOutputs( pMan, bTemp = bFlexGlo ); Cudd_Ref( bFlexGlo );
           Cudd_RecursiveDeref( dd, bTemp );
       }
    }

    // derive the flexibility local
    pMvrLoc = fmwFlexComputeLocal( pMan, bFlexGlo, pNet, pMvr );
    Cudd_RecursiveDeref( dd, bFlexGlo );

    assert( Vm_VarMapReadVarsInNum( Mvr_RelationReadVm(pMvr) ) == 
            Vm_VarMapReadVarsInNum( Mvr_RelationReadVm(pMvrLoc) ) );

    // deref the intemediate BDDs
    Fm_UtilsDerefNetwork( dd, pNet );
    return pMvrLoc;
}


/**Function*************************************************************

  Synopsis    [Returns the trivial solution if it is computed by strashing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvr_Relation_t * fmwFlexReturnTrivial( Sh_Network_t * pNet, Mvr_Relation_t * pMvr )
{
    DdManager * dd;
    Mvr_Relation_t * pMvrLoc;
    Vmx_VarMap_t * pVmxLoc;
    DdNode ** pbCodes;
    DdNode * bFunc, * bTemp;
    unsigned Polarity;
    int nVarsIn, nVarsOut, i;

    Polarity = 0;
    for ( i = 0; i < pNet->nOutputsCore; i++ )
        if ( !Sh_NodeIsConst(pNet->ppOutputsCore[i]) )
            return NULL;
        else
        {
            if ( !Sh_IsComplement(pNet->ppOutputsCore[i]) )
                Polarity |= ( 1 << i );
        }

    // if we end up here, the relation is indeed a constant
    dd       = Mvr_RelationReadDd(pMvr);
    pVmxLoc  = Mvr_RelationReadVmx(pMvr);
    nVarsIn  = Vm_VarMapReadVarsInNum( Vmx_VarMapReadVm(pVmxLoc) );
    nVarsOut = Vm_VarMapReadVarsOutNum( Vmx_VarMapReadVm(pVmxLoc) );
    assert( nVarsOut == 1 );
    pbCodes  = Vmx_VarMapEncodeVar( dd, pVmxLoc, nVarsIn );

    // create the constant relation
    bFunc = b0; Cudd_Ref( bFunc );
    for ( i = 0; i < pNet->nOutputsCore; i++ )
        if ( Polarity & (1 << i) )
        {
            bFunc = Cudd_bddOr( dd, bTemp = bFunc, pbCodes[i] );  Cudd_Ref( bFunc );
            Cudd_RecursiveDeref( dd, bTemp );
        }

    // deref the codes
    Vmx_VarMapEncodeDeref( dd, pVmxLoc, pbCodes );

    // create the relation
    pMvrLoc = Mvr_RelationCreate( Mvr_RelationReadMan(pMvr), pVmxLoc, bFunc ); 
    Cudd_Deref( bFunc );
    return pMvrLoc;
}


/**Function*************************************************************

  Synopsis    [Computes the global BDDs for all the node.]

  Description [Assumes the internal nodes are linked in the specialized
  linked list. Returns 0 if the timeout has occurred.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool fmwFlexComputeGlobalBdds( Fmw_Manager_t * pMan, DdManager * dd, Sh_Network_t * pNet )
{
    Sh_Node_t * pNode;
    DdNode * bFunc1, * bFunc2, * bFunc;
    DdNode * bFunc1Z, * bFunc2Z, * bFuncZ;
    int TimeLimit = 0;

	TimeLimit = (int)(500 /* in miliseconds*/ * (float)(CLOCKS_PER_SEC) / 1000 ) + clock();

    // go through the internal nodes in the DFS order
    Sh_NetworkForEachNodeSpecial( pNet, pNode )
    {
        if ( TimeLimit && clock() > TimeLimit )
            return 0;

//        if ( Sh_NodeIsVar(pNode) )
        if ( pNode->pOne == NULL )
        {   // make sure the elementary BDD is set
//            assert( pNode->pData );
            if ( pNode->pData )
                continue;
            if ( pNode->pData == 0 )
            {
                if ( pMan->fVerbose )
                    printf( "Visiting trivial special node...\n" );
                assert( pNode->pData2 );
                bFunc = Fm_DataGetNodeGlo( pNet->pMan->pVars[(int)pNode->pTwo] );  Cudd_Ref( bFunc );
                Fm_DataSetNodeGlo( pNode, bFunc ); // takes ref
                continue;
            }
            assert( 0 );
            continue;
        }
        if ( shNodeIsConst(pNode) )
        {
            assert( pNode->pData == 0 );
            Fm_DataSetNodeGlo( pNode, dd->one );  Cudd_Ref( dd->one );
            continue;
        }
        // this is the internal node
        // make sure the BDD is not computed
        assert( pNode->pData == 0 );

        // compute the global BDD
        bFunc1 = Fm_DataGetNodeGlo( pNode->pOne );
        bFunc2 = Fm_DataGetNodeGlo( pNode->pTwo );
        // get the result
        bFunc = Cudd_bddAnd( dd, bFunc1, bFunc2 );  Cudd_Ref( bFunc );
        // set it at the node
        Fm_DataSetNodeGlo( pNode, bFunc ); // takes ref

        // if the global BDD Z is set, do not compute it
        if ( bFuncZ = Fm_DataGetNodeGloZ(pNode) )
            continue;

        // compute the global BDD Z
        bFunc1Z = Fm_DataGetNodeGloZ( pNode->pOne );
        bFunc2Z = Fm_DataGetNodeGloZ( pNode->pTwo );
        if ( bFunc1Z || bFunc2Z )
        {
            if ( bFunc1Z == NULL )
                bFunc1Z = bFunc1;
            if ( bFunc2Z == NULL )
                bFunc2Z = bFunc2;
            // get the result
            bFuncZ = Cudd_bddAnd( dd, bFunc1Z, bFunc2Z );  Cudd_Ref( bFuncZ );
            // set it at the node
            Fm_DataSetNodeGloZ( pNode, bFuncZ ); // takes ref
        }
    }
    return 1;
}


/**Function*************************************************************

  Synopsis    [Get the flexibility in the global space.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * fmwFlexComputeGlobal( Fmw_Manager_t * pMan, Sh_Network_t * pNet )
{
    DdManager * dd = pMan->dd;
    Vm_VarMap_t * pVm;
    DdNode * bFunc, * bFuncZ;
    DdNode * bRel, * bCond, * bTemp;
    Sh_Node_t * pNode;
    int * pValues, * pValuesFirst;
    int i, v, iValue, nVarsOut;

    Extra_OperationTimeoutSet( 300 );

    // get parameters
    pVm          = pNet->pVmR;
    nVarsOut     = Vm_VarMapReadVarsInNum( pVm );
    pValues      = Vm_VarMapReadValuesArray( pVm );
    pValuesFirst = Vm_VarMapReadValuesFirstArray( pVm );

    // create the containment condition
    bRel = b1;   Cudd_Ref( bRel );
    iValue = 0;
    for ( i = 0; i < nVarsOut; i++ )
    {
        for ( v = 0; v < pValues[i]; v++ )
        {
            // get the node
            pNode = pNet->ppOutputs[pValuesFirst[i] + v];

            // get the global BDD of the cut network
            bFuncZ = Fm_DataGetNodeGloZ( pNode );
            // if this BDD is not set, this output does not depend on the cut var
            if ( bFuncZ == NULL )
                continue;

            // get the global BDD of the primary network
            bFunc = Fm_DataGetNodeGlo( pNode );

            // get the condition  (bFuncZ => bFunc)
            bCond = Cudd_bddOr( dd, Cudd_Not(bFuncZ), bFunc ); Cudd_Ref( bCond );
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
    // at this point, the general condition is ready
    // compose the special variables
//    bRel = fmwFlexComposeSpecials( pMan, bTemp = bRel, pVmxGlo, pNet ); Cudd_Ref( bRel );
//    Cudd_RecursiveDeref( dd, bTemp );
    Cudd_Deref( bRel );
    return bRel;
}


/**Function*************************************************************

  Synopsis    [Get the MV relation of the node in the global space.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * fmwFlexDeriveGlobalRelation( Fmw_Manager_t * pMan, Sh_Network_t * pNet )
{
    DdNode * pbFuncs[32], * pbCodes[32];
    int i;
    for ( i = 0; i < pNet->nOutputsCore; i++ )
    {
        pbFuncs[i] = Fm_DataGetNodeGlo( pNet->ppOutputsCore[i] );
        pbCodes[i] = Fm_DataGetNodeGloZ( pNet->ppOutputsCore[i] );
    }
    if ( pNet->nOutputsCore == 2 )
    {
        if ( pbFuncs[0] == NULL )
            pbFuncs[0] = Cudd_Not( pbFuncs[1] );
        if ( pbFuncs[1] == NULL )
            pbFuncs[1] = Cudd_Not( pbFuncs[0] );
    }
    return fmwFlexConvolve( pMan->dd, pbFuncs, pbCodes, pNet->nOutputsCore );
}

/**Function*************************************************************

  Synopsis    [Get the flexibility in the local space.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvr_Relation_t * fmwFlexComputeLocal( Fmw_Manager_t * pMan, 
     DdNode * bFlexGlo, Sh_Network_t * pNet, Mvr_Relation_t * pMvr )
{
    Mvr_Relation_t * pMvrLoc;
    DdManager * ddGlo;      // bFlexGlo is expressed using this manager
    DdManager * ddLoc;      // the resulting relation uses this manager
    DdNode ** pbFuncs;
    DdNode * bCube, * bDontCare;
    DdNode * bTemp, * bImage, * bRel;
    int * pPermute;
    int nFuncs, i;

    // get the global and local BDD managers
    ddGlo = pMan->dd;
    ddLoc = Mvr_ManagerReadDdLoc( pMan->pManMvr );

    // shortcut to the binary image if the problem is binary
    if ( pMan->fBinary )
    {
        // collect the global BDDs of the fanins
        nFuncs = pNet->nInputsCore/2;
        assert( nFuncs == Vm_VarMapReadVarsInNum( Vmx_VarMapReadVm(pMan->pVmxLoc) ) );
        pbFuncs = ALLOC( DdNode *, nFuncs );
        for ( i = 0; i < nFuncs; i++ )
            pbFuncs[i] = Fm_DataGetNodeGlo( pNet->ppInputsCore[2*i+1] );

        bCube     = Vmx_VarMapCharCubeOutput( ddGlo, pMan->pVmxGlo ); Cudd_Ref( bCube );
        bDontCare = Cudd_bddUnivAbstract( ddGlo, bFlexGlo, bCube );   Cudd_Ref( bDontCare );
//PRB( ddGlo, bDontCare );

        // compute the binary image of the care set
        bImage = Fm_ImageCompute( ddGlo, Cudd_Not(bDontCare), pbFuncs, nFuncs, 
                    pMan->pbVars1, 100 ); 
        Cudd_Ref( bImage );
        bImage = Cudd_Not( bImage );

        Cudd_RecursiveDeref( ddGlo, bDontCare );
        Cudd_RecursiveDeref( ddGlo, bCube );
    }
    else
    {
        // collect the global BDDs of the fanins
        nFuncs = pNet->nInputsCore;
        assert( nFuncs == Vm_VarMapReadValuesInNum( Vmx_VarMapReadVm(pMan->pVmxLoc) ) );
        pbFuncs = ALLOC( DdNode *, nFuncs );
        for ( i = 0; i < nFuncs; i++ )
            pbFuncs[i] = Fm_DataGetNodeGlo( pNet->ppInputsCore[i] );

        // compute the MV image
        bImage = Fm_ImageMvCompute( ddGlo, Cudd_Not(bFlexGlo), pbFuncs, nFuncs, 
                    pMan->pVmxGlo, pMan->pVmxLoc, pMan->pbVars1, 100 ); 
        Cudd_Ref( bImage );
        bImage = Cudd_Not( bImage );
    }

    // transfer from the FM's own global manager into the MVR's local manager
    pPermute = fmwFlexCreatePermMap( pMan );
    bImage = Extra_TransferPermute( ddGlo, ddLoc, bTemp = bImage, pPermute );
    Cudd_Ref( bImage );
    Cudd_RecursiveDeref( ddGlo, bTemp );
    FREE( pPermute );
    FREE( pbFuncs );

    if ( pMan->fBinary )
    {
        int * pTransMap;
        // remap the cofactors to the new variable map
        pTransMap = ALLOC( int, pNet->nInputsCore/2 + 1 );
        for ( i = 0; i < pNet->nInputsCore/2 + 1; i++ )
            pTransMap[i] = i;

        // remap the original relation to use the same variable map
        bRel = Mvr_RelationRemap( pMvr, Mvr_RelationReadRel(pMvr), 
            Mvr_RelationReadVmx(pMvr), pMan->pVmxLoc, pTransMap );
        Cudd_Ref( bRel );
        FREE( pTransMap );

        // add the care set back to the don't-care
        bImage = Cudd_bddOr( ddLoc, bTemp = bImage, bRel ); Cudd_Ref( bImage );
        Cudd_RecursiveDeref( ddLoc, bTemp );
        Cudd_RecursiveDeref( ddLoc, bRel );
    }

    // create the relation
    pMvrLoc = Mvr_RelationCreate( pMan->pManMvr, pMan->pVmxLoc, bImage ); 
    Cudd_Deref( bImage );
    // reorder the relation
    Mvr_RelationReorder( pMvrLoc );
    return pMvrLoc;
}

/**Function*************************************************************

  Synopsis    [Composes the special node functions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int * fmwFlexCreatePermMap( Fmw_Manager_t * pMan )
{
    DdManager * dd = pMan->dd;
    int * pPermute, * pBitOrder, * pBitsFirst;
    int i, iVar, nBits;
    int nVarsIn;

    // allocate the permutation map
    pPermute = ALLOC( int, dd->size );
    for ( i = 0; i < dd->size; i++ )
        pPermute[i] = -1;

    // get the parameters
    pBitsFirst = Vmx_VarMapReadBitsFirst( pMan->pVmxLoc );
    nVarsIn    = Vm_VarMapReadVarsInNum( Vmx_VarMapReadVm(pMan->pVmxLoc) );

    // add the input variables
    iVar = 0;
    for ( i = 0; i < pBitsFirst[nVarsIn]; i++ )
        pPermute[dd->size/2 + i] = iVar++;

    // get the parameters
    pBitOrder  = Vmx_VarMapReadBitsOrder( pMan->pVmxGlo );
    pBitsFirst = Vmx_VarMapReadBitsFirst( pMan->pVmxGlo );
    nVarsIn    = Vm_VarMapReadVarsInNum( Vmx_VarMapReadVm(pMan->pVmxGlo) );
    nBits      = Vmx_VarMapReadBitsNum( pMan->pVmxGlo );

    // add the output variables
    for ( i = pBitsFirst[nVarsIn]; i < nBits; i++ )
        pPermute[pBitOrder[i]] = iVar++;
    return pPermute;
}

/**Function*************************************************************

  Synopsis    [Composes one MV variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * fmwFlexRemap( DdManager * dd, DdNode * bFlex, 
    DdNode * pbFuncs[], DdNode * pbCodes[], int nValues, DdNode * bCubeChar )
{
    DdNode * bCof, * bComp, * bRes, * bTemp;
    int i;

    bRes = b0;   Cudd_Ref( bRes );
    for ( i = 0; i < nValues; i++ )
    {
        bCof  = Cudd_bddAndAbstract( dd, bFlex, pbCodes[i], bCubeChar ); Cudd_Ref( bCof );
        bComp = Cudd_bddAnd( dd, bCof, pbFuncs[i] );                     Cudd_Ref( bComp );
        bRes  = Cudd_bddOr( dd, bTemp = bRes, bComp );                   Cudd_Ref( bRes );
        Cudd_RecursiveDeref( dd, bCof );
        Cudd_RecursiveDeref( dd, bComp );
        Cudd_RecursiveDeref( dd, bTemp );
    }
    Cudd_Deref( bRes );
    return bRes;
}

/**Function*************************************************************

  Synopsis    [Composes one MV variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * fmwFlexConvolve( DdManager * dd, DdNode * pbFuncs[], DdNode * pbCodes[], int nValues )
{
    DdNode * bComp, * bRes, * bTemp;
    int i;

    bRes = b0;   Cudd_Ref( bRes );
    for ( i = 0; i < nValues; i++ )
    {
        bComp = Cudd_bddAnd( dd, pbFuncs[i], pbCodes[i] );   Cudd_Ref( bComp );
        bRes  = Cudd_bddOr( dd, bTemp = bRes, bComp );       Cudd_Ref( bRes );
        Cudd_RecursiveDeref( dd, bComp );
        Cudd_RecursiveDeref( dd, bTemp );
    }
    Cudd_Deref( bRes );
    return bRes;
}

/**Function*************************************************************

  Synopsis    [This procedure transfers from the set output to normal outputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * fmwFlexTransferFromSetOutputs( Fmw_Manager_t * pMan, DdNode * bFlexGlo )
{
    Vm_VarMap_t * pVm, * pVmInit;
    DdManager * dd;
    DdNode ** pbFuncs;
    DdNode ** pbCodes;
    DdNode * bResult;
    int nVarsIn, nValuesIn, nBits, i;
    int * pBitsFirst;

    assert( pMan->pVmxGloS );

    // get the parameters
    dd         = pMan->dd;
    pVmInit    = Vmx_VarMapReadVm( pMan->pVmxGlo );
    pVm        = Vmx_VarMapReadVm( pMan->pVmxGloS );
    nVarsIn    = Vm_VarMapReadVarsInNum( pVm );
    nValuesIn  = Vm_VarMapReadValuesInNum( pVm );
    nBits      = Vmx_VarMapReadBitsNum( pMan->pVmxGloS );
    pBitsFirst = Vmx_VarMapReadBitsFirst( pMan->pVmxGloS );

    assert( nBits - pBitsFirst[nVarsIn] == Vm_VarMapReadValuesOutNum(pVmInit) );

    // set up the vector-compose problem
    pbFuncs = ALLOC( DdNode *, dd->size );
    for ( i = 0; i < dd->size; i++ )
        pbFuncs[i] = dd->vars[i];

    // encode the variables of the normal map (to get codes for the set vars)
    pbCodes  = Vmx_VarMapEncodeMap( dd, pMan->pVmxGlo );
    // modify by setting the composition relations
    for ( i = pBitsFirst[nVarsIn]; i < nBits; i++ )
        pbFuncs[i] = pbCodes[nValuesIn + (i - pBitsFirst[nVarsIn])];

    // perform the composition
    bResult = Cudd_bddVectorCompose( dd, bFlexGlo, pbFuncs );  Cudd_Ref( bResult );

    // deref the codes
    Vmx_VarMapEncodeDeref( dd, pMan->pVmxGlo, pbCodes );
    FREE( pbFuncs );

    Cudd_Deref( bResult );
    return bResult;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int fmwFlexGetGlobalBddSize( Sh_Network_t * pNet )
{
    DdNode ** pbFuncs;
    int nNodes, i;
    pbFuncs = ALLOC( DdNode *, pNet->nOutputs );
    for ( i = 0; i < pNet->nOutputs; i++ )
        pbFuncs[i] = Fm_DataGetNodeGlo( pNet->ppOutputs[i] );
    nNodes = Cudd_SharingSize( pbFuncs, pNet->nOutputs );
    FREE( pbFuncs );
    return nNodes;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



/**CFile****************************************************************

  FileName    [fmsFlex.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Complete flexibility computation without windowing.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: fmsFlex.c,v 1.4 2004/01/06 21:54:16 alanmi Exp $]

***********************************************************************/

#include "fmInt.h"
 
////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static bool fmsFlexPrepareGlobalBdds( Fms_Manager_t * pMan, DdManager * dd, Sh_Network_t * pNet );

static Mvr_Relation_t * fmsFlexReturnTrivial( Sh_Network_t * pNet, Mvr_Relation_t * pMvr );
static bool             fmsFlexComputeGlobalBdds( Fms_Manager_t * pMan, DdManager * dd, Sh_Network_t * pNet );
static DdNode *         fmsFlexComputeGlobal( Fms_Manager_t * pMan, Sh_Network_t * pNet );
static Mvr_Relation_t * fmsFlexComputeLocal( Fms_Manager_t * pMan, DdNode * bFlexGlo, Sh_Network_t * pNet, Mvr_Relation_t * pMvr );
static DdNode *         fmsFlexDeriveGlobalRelation( Fms_Manager_t * pMan, Sh_Network_t * pNet );
static DdNode *         fmsFlexComposeSpecials( Fms_Manager_t * pMan, DdNode * bFlex, Vmx_VarMap_t * pVmx, Sh_Network_t * pNet );
static int *            fmsFlexCreatePermMap( Fms_Manager_t * pMan );
static DdNode *         fmsFlexRemap( DdManager * dd, DdNode * bFlex, DdNode * pbFuncs[], DdNode * pbCodes[], int nValues, DdNode * bCubeChar );
static DdNode *         fmsFlexConvolve( DdManager * dd, DdNode * pbFuncs[], DdNode * pbCodes[], int nValues );
static DdNode *         fmsFlexTransferFromSetOutputs( Fms_Manager_t * pMan, DdNode * bFlexGlo );
static int              fmsFlexGetGlobalBddSize( Sh_Network_t * pNet );

extern int timeGlobal;
extern int timeContain;
extern int timeImage;
extern int timeImagePrep;
extern int timeResub;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Fms_FlexibilityPrepare( Fms_Manager_t * pMan, Sh_Network_t * pNet )
{
    DdManager * dd;
    int i;

    // create the internal list of nodes in the interleaved DFS order
    Sh_NetworkInterleaveNodes( pNet );
    // create the encoded global variable map for the leaves only
    pMan->pVmxGloL = Vmx_VarMapLookup( pMan->pManVmx, pNet->pVmL, -1, NULL );
    // improve the var map by interleaving variables
    pMan->pVmxGloL = Fm_UtilsCreateVarOrdering( pMan->pVmxGloL, pNet );

    // allocate the manager large enough to store this size
    pMan->nVars0 = Vmx_VarMapReadBitsNum( pMan->pVmxGloL );
    pMan->dd = dd = Cudd_Init( pMan->nVars0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    // set the leaves
    Fm_UtilsAssignLeaves( dd, dd->vars, pNet, pMan->pVmxGloL, 0 );

    // compute the global BDDs
    if ( !fmsFlexPrepareGlobalBdds( pMan, dd, pNet ) )
    {
        Cudd_Quit( pMan->dd );
        pMan->dd = NULL;
        return 0;
    }
    assert( dd->size == pMan->nVars0 );

    // set the global vars
    pMan->nVars0  = dd->size + 50;
    pMan->pbVars0 = ALLOC( DdNode *, dd->size + 50 );
    for ( i = 0; i < 50; i++ )
        Cudd_bddNewVar( dd );
    memcpy( pMan->pbVars0, dd->vars, sizeof(DdNode *) * pMan->nVars0 );


    // save the global BDD
    pMan->nGlos = pNet->nOutputs;
    pMan->pbGlos = ALLOC( DdNode *, pNet->nOutputs );
    for ( i = 0; i < pNet->nOutputs; i++ )
        pMan->pbGlos[i] = Fm_DataGetNodeGlo( pNet->ppOutputs[i] );

if ( pMan->fVerbose )
printf( "Global BDD size = %d.\n", Cudd_SharingSize( pMan->pbGlos, pMan->nGlos ) );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Computes the global BDDs for the first time.]

  Description [Assumes the internal nodes are linked in the specialized
  linked list. Returns 0 if the timeout has occurred.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool fmsFlexPrepareGlobalBdds( Fms_Manager_t * pMan, DdManager * dd, Sh_Network_t * pNet )
{
    ProgressBar * pProgress;
    Sh_Node_t * pNode;
    DdNode * bFunc1, * bFunc2, * bFunc;
    int TimeLimit = 0;
    int nNodes, iNode;


    // If we cannot build the global BDDs in 10 seconds, 
    // then "mfs" with windowing should be used
	TimeLimit = (int)(10000 /* in miliseconds*/ * (float)(CLOCKS_PER_SEC) / 1000 ) + clock();

    // enable reordering for the first BDD construction
    Cudd_AutodynEnable( pMan->dd, CUDD_REORDER_SYMM_SIFT );

    // go through the internal nodes in the DFS order
    nNodes = Sh_NetworkDfs( pNet );
    // start the progress var
    if ( !pMan->fVerbose )
        pProgress = Extra_ProgressBarStart( stdout, nNodes );
    iNode = 0;
    Sh_NetworkForEachNodeSpecial( pNet, pNode )
    {
        if ( ++iNode % 50 == 0 && !pMan->fVerbose )
            Extra_ProgressBarUpdate( pProgress, iNode, NULL );
        assert( pNode->pData2 == 0 );

        if ( TimeLimit && clock() > TimeLimit )
        {
            if ( !pMan->fVerbose )
                Extra_ProgressBarStop( pProgress );
            return 0;
        }

//        if ( Sh_NodeIsVar(pNode) )
        if ( pNode->pOne == NULL )
        {   // make sure the elementary BDD is set
            assert( pNode->pData );
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
    }
    if ( !pMan->fVerbose )
        Extra_ProgressBarStop( pProgress );

    // reorder one last time
	Cudd_ReduceHeap( pMan->dd, CUDD_REORDER_SYMM_SIFT, 1 );
    Cudd_AutodynDisable( pMan->dd );
    return 1;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvr_Relation_t * Fms_FlexibilityCompute( Fms_Manager_t * pMan, Sh_Network_t * pNet, Mvr_Relation_t * pMvr )
{
    DdManager * dd = pMan->dd;
    Mvr_Relation_t * pMvrLoc;
    Vm_VarMap_t * pVm;
    DdNode * bFlexGlo, * bTemp;
    int clk;


    assert( pNet->nInputsCore > 0 && pNet->nOutputsCore > 0 );
//    Sh_NodePrintArray( pNet->ppOutputsCore, pNet->nOutputsCore );

    // the computation of flexibility is not needed
    // if the structural hashing has derived constant functions for all i-sets
    // here we check this condition
    if ( pMvrLoc = fmsFlexReturnTrivial( pNet, pMvr ) )
    {
        if ( pMan->fVerbose )
            printf( "Strashing has derived a constant %d-valued node.\n", pNet->nOutputsCore );
        return pMvrLoc;
    }
    // create the internal list of nodes in the interleaved DFS order
//    Sh_NetworkInterleaveNodes( pNet );
    // clean the data fields of the nodes in the list
//    Sh_NetworkCleanData( pNet );

    // create the encoded local variable map
    pVm = Vm_VarMapCreateInputOutput( pNet->pVmLC, pNet->pVmRC );
    pMan->pVmxLoc = Vmx_VarMapLookup( pMan->pManVmx, pVm, -1, NULL );
    // create the encoded global variable map
    pVm = Vm_VarMapCreateInputOutput( pNet->pVmL, pNet->pVmRC );
//    pMan->pVmxGlo = Vmx_VarMapLookup( pMan->pManVmx, pVm, -1, NULL );
    // expand the variable map by including additional variables
    pMan->pVmxGlo = Vmx_VarMapCreateAppended( pMan->pVmxGloL, pVm );
 
    // create the encoded local variable map with sets
    if ( pMan->fSetOutputs )
    { // create the set-output map
        pVm = Vm_VarMapCreateInputOutputSet( pNet->pVmL, pNet->pVmRC, 0, 1 );
        // expand the variable map by including additional variables
        pMan->pVmxGloS = Vmx_VarMapCreateAppended( pMan->pVmxGloL, pVm );
    }

    // resize the manager
    Fms_ManagerResize( pMan, Vmx_VarMapReadBitsNum(pMan->pVmxLoc) );
    // we may also need to resize after introducing the set inputs
//    if ( pMan->pVmxGloS )
//        Fms_ManagerResize( pMan, pNet->nOutputs );

    // set the var encoding cubes at the nodes
    if ( !pMan->fSetOutputs )
        Fm_UtilsAssignLeaves( dd, pMan->pbVars0, pNet, pMan->pVmxGlo, 1 );
    else
        Fm_UtilsAssignLeavesSet( dd, pMan->pbVars0, pNet, pMan->pVmxGloS, 1 );


    // compute the global BDDs
clk = clock();
    if ( !fmsFlexComputeGlobalBdds( pMan, dd, pNet ) )
    {
timeGlobal += clock() - clk;
//        Fm_UtilsDerefNetwork( dd, pNet );
        return NULL;
    }
timeGlobal += clock() - clk;

//printf( "Global BDD size = %d\n", fmsFlexGetGlobalBddSize(pNet) );

    // derive the containment condition from both pairs
clk = clock();
    bFlexGlo = fmsFlexComputeGlobal( pMan, pNet );  
timeContain += clock() - clk;

    if ( bFlexGlo == NULL )
    {
//        Fm_UtilsDerefNetwork( dd, pNet );
        return NULL;
    }
    else
    {
       Cudd_Ref( bFlexGlo );
       if ( pMan->fSetOutputs )
       {
           bFlexGlo = fmsFlexTransferFromSetOutputs( pMan, bTemp = bFlexGlo ); Cudd_Ref( bFlexGlo );
           Cudd_RecursiveDeref( dd, bTemp );
       }
    }

    // derive the flexibility local
    pMvrLoc = fmsFlexComputeLocal( pMan, bFlexGlo, pNet, pMvr );
    Cudd_RecursiveDeref( dd, bFlexGlo );

    assert( Vm_VarMapReadVarsInNum( Mvr_RelationReadVm(pMvr) ) == 
            Vm_VarMapReadVarsInNum( Mvr_RelationReadVm(pMvrLoc) ) );

    // deref the intemediate BDDs
//    Fm_UtilsDerefNetwork( dd, pNet );
    return pMvrLoc;
}


/**Function*************************************************************

  Synopsis    [Computes the global BDDs for the first time.]

  Description [Assumes the internal nodes are linked in the specialized
  linked list. Returns 0 if the timeout has occurred.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool fmsFlexComputeGlobalBdds( Fms_Manager_t * pMan, DdManager * dd, Sh_Network_t * pNet )
{
    Sh_Node_t * pNode;
    DdNode * bFunc1, * bFunc2, * bFunc;
    int TimeLimit = 0;

    // If we cannot build the global BDDs in 1 second, 
    // it is worth stopping and trying another node
	TimeLimit = (int)(100 /* in miliseconds*/ * (float)(CLOCKS_PER_SEC) / 1000 ) + clock();

    // go through the internal nodes in the DFS order
    Sh_NetworkDfs( pNet );
    Sh_NetworkForEachNodeSpecial( pNet, pNode )
    {
        assert( pNode->pData2 == 0 );
        if ( TimeLimit && clock() > TimeLimit )
            return 0;

//        if ( Sh_NodeIsVar(pNode) )
        if ( pNode->pOne == NULL )
        {   // make sure the elementary BDD is set
            assert( pNode->pData );
            continue;
        }
        if ( shNodeIsConst(pNode) )
        {
            if ( pNode->pData )
                continue;
            Fm_DataSetNodeGlo( pNode, dd->one );  Cudd_Ref( dd->one );
            continue;
        }
        // this is the internal node
        // make sure the BDD is not computed
//        assert( pNode->Num < 0 || pNode->pData == 0 );
        // if the BDD is computed, this is fine
        if ( pNode->pData )
            continue;

        // compute the global BDD
        bFunc1 = Fm_DataGetNodeGlo( pNode->pOne );
        bFunc2 = Fm_DataGetNodeGlo( pNode->pTwo );
        // get the result
        bFunc = Cudd_bddAnd( dd, bFunc1, bFunc2 );  Cudd_Ref( bFunc );
        // set it at the node
        Fm_DataSetNodeGlo( pNode, bFunc ); // takes ref
    }
    return 1;
}


/**Function*************************************************************

  Synopsis    [Returns the trivial solution if it is computed by strashing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvr_Relation_t * fmsFlexReturnTrivial( Sh_Network_t * pNet, Mvr_Relation_t * pMvr )
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

  Synopsis    [Get the flexibility in the global space.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * fmsFlexComputeGlobal( Fms_Manager_t * pMan, Sh_Network_t * pNet )
{
    DdManager * dd = pMan->dd;
    Vm_VarMap_t * pVm;
    DdNode * bFunc, * bFuncZ;
    DdNode * bRel, * bCond, * bTemp;
    Sh_Node_t * pNode;
    int * pValues, * pValuesFirst;
    int i, v, iValue, nVarsOut;

    Extra_OperationTimeoutSet( 100 );

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
            bFunc  = pMan->pbGlos[pValuesFirst[i] + v]; 
            bFuncZ = Fm_DataGetNodeGlo( pNode );
            // if this is the same BDD, this output does not depend on the cut var
            if ( bFunc == bFuncZ )
                continue;
            // get the condition  (bFuncZ => bFunc)
//            bCond = Cudd_bddOr( dd, Cudd_Not(bFuncZ), bFunc ); Cudd_Ref( bCond );
            bCond = Extra_bddAnd( dd, bFuncZ, Cudd_Not(bFunc) );  
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
    }
    Extra_OperationTimeoutReset();
    // at this point, the general condition is ready
    // compose the special variables
//    bRel = fmsFlexComposeSpecials( pMan, bTemp = bRel, pVmxGlo, pNet ); Cudd_Ref( bRel );
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
DdNode * fmsFlexDeriveGlobalRelation( Fms_Manager_t * pMan, Sh_Network_t * pNet )
{
    assert( 0 );
    // does not work because we do not store the global BDDs of the cut node
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Get the flexibility in the local space.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvr_Relation_t * fmsFlexComputeLocal( Fms_Manager_t * pMan, 
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
    int clk, clkP;

clkP = clock();

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
//PRB( ddGlo, bFlexGlo );
//PRB( ddGlo, bDontCare );

clk = clock();
        // compute the binary image of the care set
        bImage = Fm_ImageCompute( ddGlo, Cudd_Not(bDontCare), pbFuncs, nFuncs, 
                    pMan->pbVars1, 100 ); 
        Cudd_Ref( bImage );
        bImage = Cudd_Not( bImage );
clk = clock() - clk;

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
clk = clock();
        bImage = Fm_ImageMvCompute( ddGlo, Cudd_Not(bFlexGlo), pbFuncs, nFuncs, 
                    pMan->pVmxGlo, pMan->pVmxLoc, pMan->pbVars1, 100 ); 
        Cudd_Ref( bImage );
        bImage = Cudd_Not( bImage );
clk = clock() - clk;
    }
//PRB( ddGlo, bImage );

    // transfer from the FM's own global manager into the MVR's local manager
    pPermute = fmsFlexCreatePermMap( pMan );
    bImage = Extra_TransferPermute( ddGlo, ddLoc, bTemp = bImage, pPermute );
    Cudd_Ref( bImage );
    Cudd_RecursiveDeref( ddGlo, bTemp );
    FREE( pPermute );
    FREE( pbFuncs );

//PRB( ddLoc, bImage );
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
//PRB( ddLoc, bImage );

    // create the relation
    pMvrLoc = Mvr_RelationCreate( pMan->pManMvr, pMan->pVmxLoc, bImage ); 
    Cudd_Deref( bImage );
    // reorder the relation
    Mvr_RelationReorder( pMvrLoc );

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
int * fmsFlexCreatePermMap( Fms_Manager_t * pMan )
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
//        pPermute[dd->size/2 + i] = iVar++;
        pPermute[pMan->nVars0 + i] = iVar++;

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
DdNode * fmsFlexRemap( DdManager * dd, DdNode * bFlex, 
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
DdNode * fmsFlexConvolve( DdManager * dd, DdNode * pbFuncs[], DdNode * pbCodes[], int nValues )
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
DdNode * fmsFlexTransferFromSetOutputs( Fms_Manager_t * pMan, DdNode * bFlexGlo )
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
int fmsFlexGetGlobalBddSize( Sh_Network_t * pNet )
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



/**CFile****************************************************************

  FileName    [ntkGlobal.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Support for the global functions of the node/network.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkGlobal.c,v 1.31 2004/05/12 04:30:08 alanmi Exp $]

***********************************************************************/

#include "ntkInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

/* 
    The global functions of the network are the functions of the nodes
    of the network, expressed in terms of the primary inputs of the network.
    Some applications may need the global functions of each node, some only
    the global functions of the primary outputs, or only latch inputs.
    The procedures in this file provide a variety of ways to compute global
    functions. Typically, the global functions are computed in a new BDD
    manager, which is created inside the procedure Ntk_NetworkGlobalCompute().
    The interleaved variable ordering is used to order the elementary BDD
    variables before computing the global functions.

    The structure to represent global functions of a node is created for 
    each node on demand, the first time API Ntk_NodeReadFuncGlobal(pNode)
    is called. The structure is cleaned the first time it is created.
    The global manager used to represent the global functions of the nodes
    is stored in each node's global function structure. This allows for 
    dereferencing the global function automatically when the node is deleted.
    It is the responsibility of the user of the global functionality APIs 
    to clean the global functions structures of each node before the global
    manager is delocated. The user should call Ntk_NetworkGlobalDeref() 
    when the global functionality is no longer needed, before deleting
    the manager.
*/


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Returns the global functions of COs of the network.]

  Description [This procedure computes the global functions of the 
  network in the new BDD manager. The global functions are stored at
  the nodes and can be accessed through the API Ntk_NodeReadFuncGlob().
  The global manager is returned by Ntk_NetworkReadDdGlo(). The global manager
  should not be allocated before this functions is called. Flag "fDynEnable"
  enables dynamic variable reordering during the global function computation.
  Flag "fLatchOnly" when set computes only the latch output functions.
  If Flag "fVerbose" is set, this procedure prints the progress bar, which
  describes the progress of computing the global BDDs.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkGlobalComputeCo( Ntk_Network_t * pNet, 
    bool fDynEnable, bool fLatchOnly, bool fVerbose )
{
    DdManager * dd;
    bool fDropInternal = 1;

    // make sure there is no global manager
    assert(  Ntk_NetworkReadDdGlo( pNet ) == NULL );

    // compute the global functions of this network
    // this will start the manager and create global variable map
    Ntk_NetworkGlobalCompute( pNet, fDropInternal, fDynEnable, fLatchOnly, fVerbose );

    // reorder the global functions again 
    // when the intermediate functions are dropped
    dd = Ntk_NetworkReadDdGlo( pNet );
    if ( fDynEnable )
    {
        // perform the last-gasp reordering
        Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT,1 );
        if ( fVerbose )
        {
            fprintf( stdout, "The number of nodes in the shared BDD of the COs after reordering  = %d\n", 
                Cudd_ReadNodeCount(dd) );
            fflush( stdout );
        }
    }
}

/**Function*************************************************************

  Synopsis    [Returns the global functions of COs of the network.]

  Description [This procedure computes the global functions to the 
  network in the new BDD manager. It returns only the global functions 
  of the COs, and cleans all the intermediate BDDs at the nodes. If the flag
  "fBinary" is set to 1, the resulting global BDDs are only i-sets 1 
  of each output. This option is used to compute the global functions of 
  the binary networks. If the flag "fBinary" is set to 0, the function
  returns the array of BDDs for reach i-set of the COs. The global manager
  can be retrieved by calling Ntk_NetworkReadDdGlo(). The global manager
  should not be allocated before this functions is called. Flag "fDynEnable"
  enables dynamic variable reordering during the global function computation.
  Flag "fLatchOnly" when set computes only the latch output functions.
  If Flag "fVerbose" is set, this procedure prints the progress bar, which
  describes the progress of computing the global BDDs.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode ** Ntk_NetworkGlobalComputeCoArray( Ntk_Network_t * pNet, 
    bool fBinary, bool fDynEnable, bool fLatchOnly, bool fVerbose )
{
    DdManager * dd;
    Ntk_Node_t * pNode;
    DdNode ** pbFuncs, ** pbFuncsThis;
    int nValues, iValue;
    bool fDropInternal = 1;

    // make sure there is no global manager
    assert(  Ntk_NetworkReadDdGlo( pNet ) == NULL );

    // compute the global functions of this network
    // this will start the manager and create global variable map
    Ntk_NetworkGlobalCompute( pNet, fDropInternal, fDynEnable, fLatchOnly, fVerbose );

    // count the number of output values
    nValues = 0;
    Ntk_NetworkForEachCo( pNet, pNode )
        nValues += pNode->nValues;

    // collect the functions
    dd = Ntk_NetworkReadDdGlo(pNet);
    pbFuncs = ALLOC( DdNode *, nValues );
    iValue = 0;
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        if ( !fLatchOnly || pNode->Subtype == MV_BELONGS_TO_LATCH )
        {
            pbFuncsThis = Ntk_NodeReadFuncGlob(pNode);
            if ( fBinary )
            {
                pbFuncs[iValue] = pbFuncsThis[1];  Cudd_Ref( pbFuncs[iValue] );
                iValue++;
            }
            else
            {
                Ntk_NodeGlobalCopyFuncs( pbFuncs + iValue, pbFuncsThis, pNode->nValues );
                Ntk_NodeGlobalRefFuncs( pbFuncs + iValue, pNode->nValues );
                iValue += pNode->nValues;
            }
        }
    }

    // undo the global functions of the internal nodes of the network
    // it is important to do this before the final variable reordering
    if ( fDropInternal )
    { // if we dropped the internal nodes, we only need to deref the COs
        Ntk_NetworkGlobalCleanCo( pNet );
        if ( pNet->pNetExdc )
            Ntk_NetworkGlobalCleanCo( pNet->pNetExdc );
    }
    else
    { // otherwise deref all nodes
        Ntk_NetworkGlobalClean( pNet );
        if ( pNet->pNetExdc )
            Ntk_NetworkGlobalClean( pNet->pNetExdc );
    }

    // reorder the global functions again 
    // when the intermediate functions are dropped
    if ( fDynEnable )
    {
        // perform the last-gasp reordering
        Cudd_ReduceHeap(dd, CUDD_REORDER_SYMM_SIFT,1);
        if ( fVerbose )
        {
            fprintf( stdout, "The number of nodes in the shared BDD of the COs after reordering  = %d\n", 
                Cudd_ReadNodeCount(dd) );
            fflush( stdout );
        }
    }
    return pbFuncs;
}


/**Function*************************************************************

  Synopsis    [Computes the global BDDs of the network.]

  Description [Assumes that the global manager does not exist. Creates
  a new global var map and DD manager, orders the CI nodes using interleaving,
  and compute the global BDDs of the network (and EXDC network, if given).
  If flag "fDropInternal" is set, derefereces BDDs at the internal nodes
  as soon as they become unnecessary (this option should be enables for
  efficiency, if only the CO global BDDs are needed). Flag "fDynEnable"
  enables dynamic variable reordering during the global function computation.
  Flag "fLatchOnly" when set computes only the latch output functions.
  If Flag "fVerbose" is set, this procedure prints the progress bar which
  describes the progress of computing the global BDDs.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkGlobalCompute( Ntk_Network_t * pNet, 
    bool fDropInternal, bool fDynEnable, bool fLatchOnly, bool fVerbose )
{
    DdManager * dd;
    DdNode ** pbCodes;
    int * pValuesFirst;
    Ntk_Node_t * pNode;
    Vmx_VarMap_t * pVmx, * pVmxP;
    int * pPermuteBin, * pPermuteInv;
    int nNodesCi, nBits;
    int i;

    // make sure there is no global manager
    assert(  Ntk_NetworkReadDdGlo( pNet ) == NULL );

    // order the variables using the interleaved ordering
    Ntk_NetworkInterleave( pNet );

    // get the permutation array for this variable ordering
    nNodesCi = Ntk_NetworkReadCiNum(pNet);
    pPermuteInv = ALLOC( int, nNodesCi );
    i = 0;
    Ntk_NetworkForEachCi( pNet, pNode )
    {
//printf( "Original level %2d : PI %s\n", i, pNode->pName );
        pNode->pData = (char *)i++;
    }
    i = 0;
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
        if ( pNode->Type == MV_NODE_CI )
        {
//printf( "Interleaved level %2d : PI %s\n", i, pNode->pName );
            pPermuteInv[i++] = (int)pNode->pData;
        }

    // create a variable map with CIs in their natural order
    pVmx = Ntk_NetworkGlobalCreateVmxCi( pNet );
    // permute this array using the above interleave ordering
    pVmxP = Vmx_VarMapCreatePermuted( pVmx, pPermuteInv );
    // get the binary permutation array
    pPermuteBin = Vmx_VarMapReadBitsOrder( pVmxP );
    FREE( pPermuteInv );

    // create the new manager
    nBits = Vmx_VarMapReadBitsNum(pVmx);
    dd = Cudd_Init( nBits, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    // permute the manager according to the static ordering
    Cudd_ShuffleHeap( dd, pPermuteBin );
    // from now on, the manager itself remembers the static variable order
//for ( i = 0; i < dd->size; i++ )
//    printf( "level %2d : var %2d\n", i, dd->invperm[i] );

    // set the global parameters of the network
    Ntk_NetworkSetDdGlo( pNet, dd );
    Ntk_NetworkSetVmxGlo( pNet, pVmx );

    // set the PI variables of the primary network
    pbCodes = Vmx_VarMapEncodeMap( dd, pVmx ); 
    pValuesFirst = Vm_VarMapReadValuesFirstArray( Vmx_VarMapReadVm(pVmx) );
    i = 0;
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        Ntk_NodeWriteFuncGlob( pNode, pbCodes + pValuesFirst[i] );
//printf( "%s\n", pNode->pName );
//PRB( dd, pbCodes[ pValuesFirst[i] ] );
        Ntk_NodeWriteFuncDd( pNode, dd );
        i++;
    }
    // dereference the codes
    Vmx_VarMapEncodeDeref( dd, pVmx, pbCodes );

    // transfer the PI variables by name to the don't-care network 
    // (if we don't do it now, they may be dropped later)
    if ( pNet->pNetExdc )
        Ntk_NetworkGlobalSetCis( pNet, pNet->pNetExdc );

    // compute global functions of the network
    Ntk_NetworkGlobalComputeOne( pNet, 0, fDropInternal, fDynEnable, fLatchOnly, fVerbose );

    // compute global functions of the EXDC network
    if ( pNet->pNetExdc )
        Ntk_NetworkGlobalComputeOne( pNet, 1, fDropInternal, 0, 0, fVerbose );
}

/**Function*************************************************************

  Synopsis    [Computes the global BDDs and assigns them to the nodes.]

  Description [This procedure takes the network and builds the global 
  BDDs of all the nodes in the network using the given BDD manager.
  This procedure assumes that the global BDDs of the PIs are set.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkGlobalComputeOne( Ntk_Network_t * pNet, 
    bool fExdc, int fDropInternal, bool fDynEnable, int fLatchOnly, bool fVerbose )
{
    ProgressBar * pProgress;
    DdManager * dd;
    Ntk_Node_t * pNode;
    int nSizeBdd, nSizeBddMax, nSizeBddTotal;
    int CounterVisited, nNodesTotal, iOutput;

    // count the number of visits
    if ( fDropInternal )
        Ntk_NetworkCountVisits( pNet, fLatchOnly );

    dd = Ntk_NetworkReadDdGlo( pNet );

    // enable variable reordering
    if ( fDynEnable )
        Cudd_AutodynEnable( dd, CUDD_REORDER_SYMM_SIFT );
    else
        Cudd_AutodynDisable( dd );

    CounterVisited = nSizeBddMax = nSizeBddTotal = iOutput = 0;
	nNodesTotal  = Ntk_NetworkReadNodeIntNum(pNet) + Ntk_NetworkReadCoNum(pNet);
	if ( !fVerbose )
        pProgress = Extra_ProgressBarStart( stdout, nNodesTotal );

    // order all the nodes of the network
    if ( !fExdc )
    {
        if ( fLatchOnly )
            Ntk_NetworkDfsLatches( pNet, 1 );
        else
            Ntk_NetworkDfs( pNet, 1 );

        Ntk_NetworkForEachNodeSpecial( pNet, pNode )
        {
            if ( pNode->Type != MV_NODE_CI )
            {
                if ( pNode->Type == MV_NODE_CO )
                    iOutput++;

                if ( Ntk_NodeGlobalCompute( dd, pNode, fDropInternal ) )
                    return 1; // timeout
            
	            // print statistics of global BDD construction
		        ++CounterVisited;
	            if ( fVerbose )
	            {
		            nSizeBdd = Cudd_SharingSize( Ntk_NodeReadFuncGlob(pNode), pNode->nValues );
		            // add to the largest and average BDD size
		            if ( nSizeBddMax < nSizeBdd )
			             nSizeBddMax = nSizeBdd;
		            nSizeBddTotal += nSizeBdd;
		            // mark the progress of building global BDDs
		            if ( CounterVisited % 20 )
		            {
			            float Percent = (float)100.0 * CounterVisited/nNodesTotal;
			            fprintf( stdout, "Out #%d: Global BDDs %3.1f %% complete.  Max BDD = %-4d  Average BDD = %-4d\r", 
				            iOutput, Percent, nSizeBddMax, nSizeBddTotal/CounterVisited );
			            fflush( stdout );
		            }
	            }
                else
                {
                    if ( CounterVisited % 20 == 0 )
                        Extra_ProgressBarUpdate( pProgress, CounterVisited, NULL );
                }
            }
        }
    }
    else
    {
        Ntk_NetworkDfs( pNet->pNetExdc, 1 );
        Ntk_NetworkForEachNodeSpecial( pNet->pNetExdc, pNode )
        {
            if ( pNode->Type != MV_NODE_CI )
            {
                if ( Ntk_NodeGlobalCompute( dd, pNode, fDropInternal ) )
                    return 1; // timeout
            }
        }
    }

	// finish printing statistics of the global BDD computation
	if ( fVerbose )
	{
		fprintf( stdout, "Out #%d: Global BDDs 100.0 %s complete.  Max BDD = %-4d  Average BDD = %-4d\n", 
			iOutput, "%", nSizeBddMax, nSizeBddTotal/CounterVisited  );
		fflush( stdout );
	}
    else
        Extra_ProgressBarStop( pProgress );

    if ( fVerbose )
    {
        fprintf( stdout, "The number of nodes in the shared BDD of internal nodes = %d\n", 
            Ntk_NodeGlobalNumSharedBddNodes(dd,pNet) );
        fflush( stdout );
    }

    // disable variable reordering
    Cudd_AutodynDisable( dd );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Sets the global functions of CIs by name.]

  Description [Transfers the global BDD manager from the old network to
  the new network. Sets the global functions of the CIs of the new network
  to be the same as the node with the same name in the old network.
  For each CI of the new network, there should exist the CI node with 
  the same name in the old network.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkGlobalSetCis( Ntk_Network_t * pNetOld, Ntk_Network_t * pNetNew )
{
    Vmx_VarMap_t * pVmx;
    DdManager * dd;
    DdNode ** pbFuncs;
    Ntk_Node_t * pNodeNew, * pNodeOld;

    // set the manager
    dd = Ntk_NetworkReadDdGlo( pNetOld );
    Ntk_NetworkSetDdGlo( pNetNew, dd );

    // get the variable map
    pVmx = Ntk_NetworkReadVmxGlo( pNetOld );
    Ntk_NetworkSetVmxGlo( pNetNew, pVmx );

    // go through the nodes of the new network
    Ntk_NetworkForEachCi( pNetNew, pNodeNew )
    {
        // find the corresponding CI node of the old network
        pNodeOld = Ntk_NetworkFindNodeByName( pNetOld, pNodeNew->pName );
        // the node should exist, otherwise we do not know how to set its BDD
        assert( pNodeOld );
        // set the global BDDs of the old node
        pbFuncs = Ntk_NodeReadFuncGlob(pNodeOld);
        // set the global BDDs of the new node
        Ntk_NodeWriteFuncGlob( pNodeNew, pbFuncs );
        Ntk_NodeWriteFuncDd( pNodeNew, dd );
    }
}


/**Function*************************************************************

  Synopsis    [Computes the global BDDs of the network.]

  Description [This procedure is called when the global BDDs for the network
  are no longer used and no longer needed. This procedure delocates the
  global BDD manager of the network.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkGlobalDeref( Ntk_Network_t * pNet )
{
    Ntk_NetworkGlobalClean( pNet );
    if ( pNet->pNetExdc )
        Ntk_NetworkGlobalClean( pNet->pNetExdc );
    Ntk_NetworkSetDdGlo( pNet, NULL );
}

/**Function*************************************************************

  Synopsis    [Builds the BDD for the node if the fanin BDDs are available.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NodeGlobalCompute( DdManager * dd, Ntk_Node_t * pNode, int fDropInternal )
{
    Cvr_Cover_t * pCvr, * pCvrNew;
    Mvr_Relation_t * pMvr;
    Vm_VarMap_t * pVm;
    Ntk_Pin_t * pPin;
    Ntk_Node_t * pFanin;
    DdNode ** pbFuncs;
    DdNode ** pbArray, ** pbArray2;
    bool fNonDeterministic = 0;
    int i;

    // check the timeout
    if ( 0 )
        return 1; // timeout

    // compute the global BDD
    if ( pNode->Type == MV_NODE_CO )
    {
        pFanin = Ntk_NodeReadFaninNode(pNode,0);
        pbFuncs = Ntk_NodeReadFuncGlob(pFanin);
        assert( pbFuncs[0] != NULL );
        // set the global BDD of the node
        Ntk_NodeWriteFuncGlob( pNode, pbFuncs );   
        Ntk_NodeWriteFuncDd( pNode, dd ); 

        // count visits to the fanins
        // if this is the last visit, clean the fanins functions
        if ( fDropInternal )
        {
            Ntk_NodeForEachFanin( pNode, pPin, pFanin )
                if ( --pFanin->Level == 0 )
                    Ntk_NodeGlobalClean( pFanin );
        }
        return 0;
    }

    // get the arrays
    pbArray  = (DdNode **)pNode->pNet->pArray1;
    pbArray2 = (DdNode **)pNode->pNet->pArray2;
    // get the variable map
    pVm      = Ntk_NodeReadFuncVm(pNode);

    // collect the fanin BDDs and make sure the fanins are built
    i = 0;
    Ntk_NodeForEachFanin( pNode, pPin, pFanin )
    {
        pbFuncs = Ntk_NodeReadFuncGlob(pFanin);
        Ntk_NodeGlobalCopyFuncs( pbArray + i, pbFuncs, pFanin->nValues );
        i += pFanin->nValues;
        // check whether the input MDDs are non-deterministic
        if ( fNonDeterministic == 0 )
            fNonDeterministic = Ntk_NodeGlobalCheckNDFuncs( dd, pbFuncs, pFanin->nValues );
    }

    // get the cover
    pCvr = Ntk_NodeGetFuncCvr(pNode);
    // compute the cover with the default value if necessary
    pCvrNew = NULL;
    if ( fNonDeterministic && Cvr_CoverReadDefault(pCvr) >= 0 )
    {
        // get hold of Mvr
        pMvr = Ntk_NodeGetFuncMvr( pNode );
        // derive the new Cvr
        pCvrNew = Fnc_FunctionDeriveCvrFromMvr( Ntk_NodeReadManMvc(pNode), pMvr, 0 ); // don't use the default
    }
    if ( pCvrNew )
    {
        Fnc_FunctionDeriveMddFromCvr( dd, pVm, pCvrNew, pbArray, pbArray2 ); 
        Cvr_CoverFree( pCvrNew );
    }
    else
        Fnc_FunctionDeriveMddFromCvr( dd, pVm, pCvr, pbArray, pbArray2 );
    pbFuncs = pbArray2;

    // set the global BDDs of the node
    Ntk_NodeWriteFuncGlob( pNode, pbFuncs );   
    Ntk_NodeWriteFuncDd( pNode, dd ); 

    // deref this global BDD
    Ntk_NodeGlobalDerefFuncs( dd, pbFuncs, pNode->nValues );

    // count visits to the fanins
    // if this is the last visit, clean the fanins functions
    if ( fDropInternal )
    {
        Ntk_NodeForEachFanin( pNode, pPin, pFanin )
            if ( --pFanin->Level == 0 )
                Ntk_NodeGlobalClean( pFanin );
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Builds the BDD for the node if the fanin BDDs are available.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NodeGlobalComputeZ( Ntk_Node_t * pNode, int TimeLimit )
{
    DdManager * dd;
    Cvr_Cover_t * pCvr, * pCvrNew;
    Mvr_Relation_t * pMvr;
    Vm_VarMap_t * pVm;
    Ntk_Pin_t * pPin;
    Ntk_Node_t * pFanin;
    DdNode ** pbFuncs;
    DdNode ** pbArray, ** pbArray2;
    bool fNonDeterministic = 0;
    int i;

    // check the timeout
    if ( TimeLimit && clock() > TimeLimit )
        return 1; // timeout

    dd = Ntk_NodeReadFuncDd( pNode );

    // compute the global BDD
    if ( pNode->Type == MV_NODE_CO )
    {
        pFanin = Ntk_NodeReadFaninNode(pNode,0);
        pbFuncs = Ntk_NodeReadFuncGlobZ(pFanin);
        assert( pbFuncs[0] != NULL );
        // set the global BDD of the node
        Ntk_NodeWriteFuncGlobZ( pNode, pbFuncs );   
        return 0;
    }

    // get the arrays
    pbArray  = (DdNode **)pNode->pNet->pArray1;
    pbArray2 = (DdNode **)pNode->pNet->pArray2;
    // get the variable map
    pVm = Ntk_NodeReadFuncVm(pNode);

    // collect the fanin BDDs and make sure the fanins are built
    i = 0;
    Ntk_NodeForEachFanin( pNode, pPin, pFanin )
    {
        pbFuncs = Ntk_NodeReadFuncGlobZ(pFanin);
        if ( pbFuncs[0] == NULL )
        {
            pbFuncs = Ntk_NodeReadFuncGlob(pFanin);
            assert( pbFuncs[0] );
        }
        Ntk_NodeGlobalCopyFuncs( pbArray + i, pbFuncs, pFanin->nValues );
        i += pFanin->nValues;

        // check whether the input MDDs are non-deterministic
        if ( fNonDeterministic == 0 )
            fNonDeterministic = Ntk_NodeGlobalCheckNDFuncs( dd, pbFuncs, pFanin->nValues );
    }

    // get the cover
    pCvr = Ntk_NodeGetFuncCvr(pNode);
    // compute the cover with the default value if necessary
    pCvrNew = NULL;
    if ( fNonDeterministic && Cvr_CoverReadDefault( pCvr ) >= 0 )
    {
        // get hold of Mvr
        pMvr = Ntk_NodeGetFuncMvr( pNode );
        // derive the new Cvr
        pCvrNew = Fnc_FunctionDeriveCvrFromMvr( Ntk_NodeReadManMvc(pNode), pMvr, 0 ); // don't use the default
    }
    if ( pCvrNew )
    {
        Fnc_FunctionDeriveMddFromCvr( dd, pVm, pCvrNew, pbArray, pbArray2 ); 
        Cvr_CoverFree( pCvrNew );
    }
    else
        Fnc_FunctionDeriveMddFromCvr( dd, pVm, pCvr, pbArray, pbArray2 );
    pbFuncs = pbArray2;

    // set the global BDDs of the node
    Ntk_NodeWriteFuncGlobZ( pNode, pbFuncs );   

    // deref this global BDD
    Ntk_NodeGlobalDerefFuncs( dd, pbFuncs, pNode->nValues );
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NodeGlobalComputeBinary( DdManager * dd, Ntk_Node_t * pNode, int fDropInternal )
{
    Cvr_Cover_t * pCvr;
    Vm_VarMap_t * pVm;
    Ntk_Pin_t * pPin;
    Ntk_Node_t * pFanin;
    DdNode * bFunc;
    DdNode ** pbArray, ** pbArray2;
    int i;

    // check the timeout
    if ( 0 )
        return 1; // timeout

    // compute the global BDD
    if ( pNode->Type == MV_NODE_CO )
    {
        pFanin = Ntk_NodeReadFaninNode(pNode,0);
        bFunc = Ntk_NodeReadFuncBinGlo(pFanin);
        assert( bFunc != NULL );
        // set the global BDD of the node
        Ntk_NodeWriteFuncBinGlo( pNode, bFunc );   
        Ntk_NodeWriteFuncDd( pNode, dd ); 

        // count visits to the fanins
        // if this is the last visit, clean the fanins functions
        if ( fDropInternal )
        {
            Ntk_NodeForEachFanin( pNode, pPin, pFanin )
                if ( --pFanin->Level == 0 )
                    Ntk_NodeGlobalClean( pFanin );
        }
        return 0;
    }

    // get the arrays
    pbArray  = (DdNode **)pNode->pNet->pArray1;
    pbArray2 = (DdNode **)pNode->pNet->pArray2;
    // get the variable map
    pVm      = Ntk_NodeReadFuncVm(pNode);

    // collect the fanin BDDs and make sure the fanins are built
    i = 0;
    Ntk_NodeForEachFanin( pNode, pPin, pFanin )
    {
        bFunc = Ntk_NodeReadFuncBinGlo(pFanin);
        assert( bFunc );
        pbArray[2*i+0] = Cudd_Not(bFunc);
        pbArray[2*i+1] = bFunc;
        i++;
    }

    // get the cover
    pCvr = Ntk_NodeGetFuncCvr(pNode);
    Fnc_FunctionDeriveMddFromCvr( dd, pVm, pCvr, pbArray, pbArray2 );

    // set the global BDDs of the node
    Ntk_NodeWriteFuncBinGlo( pNode, pbArray2[1] );   
    Ntk_NodeWriteFuncDd( pNode, dd ); 

    // deref this global BDD
    Ntk_NodeGlobalDerefFuncs( dd, pbArray2, 2 );

    // count visits to the fanins
    // if this is the last visit, clean the fanins functions
    if ( fDropInternal )
    {
        Ntk_NodeForEachFanin( pNode, pPin, pFanin )
            if ( --pFanin->Level == 0 )
                Ntk_NodeGlobalClean( pFanin );
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NodeGlobalComputeZBinary( Ntk_Node_t * pNode )
{
    DdManager * dd;
    Cvr_Cover_t * pCvr;
    Vm_VarMap_t * pVm;
    Ntk_Pin_t * pPin;
    Ntk_Node_t * pFanin;
    DdNode * bFunc;
    DdNode ** pbArray, ** pbArray2;
    int i;

    // check the timeout
    if ( 0 )
        return 1; // timeout

    dd = Ntk_NodeReadFuncDd( pNode );

    // compute the global BDD
    if ( pNode->Type == MV_NODE_CO )
    {
        pFanin = Ntk_NodeReadFaninNode(pNode,0);
        bFunc = Ntk_NodeReadFuncBinGloZ(pFanin);
        assert( bFunc != NULL );
        // set the global BDD of the node
        Ntk_NodeWriteFuncBinGloZ( pNode, bFunc );   
        Ntk_NodeWriteFuncDd( pNode, dd ); 
        return 0;
    }

    // get the arrays
    pbArray  = (DdNode **)pNode->pNet->pArray1;
    pbArray2 = (DdNode **)pNode->pNet->pArray2;
    // get the variable map
    pVm      = Ntk_NodeReadFuncVm(pNode);

    // collect the fanin BDDs and make sure the fanins are built
    i = 0;
    Ntk_NodeForEachFanin( pNode, pPin, pFanin )
    {
        bFunc = Ntk_NodeReadFuncBinGloZ(pFanin);
        if ( bFunc == NULL )
        {
            bFunc = Ntk_NodeReadFuncBinGlo(pFanin);
            assert( bFunc );
        }
        pbArray[2*i+0] = Cudd_Not(bFunc);
        pbArray[2*i+1] = bFunc;
        i++;
    }

    // get the cover
    pCvr = Ntk_NodeGetFuncCvr(pNode);
    Fnc_FunctionDeriveMddFromCvr( dd, pVm, pCvr, pbArray, pbArray2 );

    // set the global BDDs of the node
    Ntk_NodeWriteFuncBinGloZ( pNode, pbArray2[1] );   
    Ntk_NodeWriteFuncDd( pNode, dd ); 

    // deref this global BDD
    Ntk_NodeGlobalDerefFuncs( dd, pbArray2, 2 );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Cleans the global functionality structure of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeGlobalClean( Ntk_Node_t * pNode )
{
    if ( pNode->pG == NULL )
        return;
    Fnc_GlobalClean( pNode->pG );
}

/**Function*************************************************************

  Synopsis    [Cleans the global structures for all the internal nodes.]

  Description [This functions should be called when some computation
  which involved global BDDs is over, before the global BDD manager 
  is deleted.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkGlobalClean( Ntk_Network_t * pNet )
{
	Ntk_Node_t * pNode;
    Ntk_NetworkForEachCi( pNet, pNode ) 
		Ntk_NodeGlobalClean( pNode );
    Ntk_NetworkForEachNode( pNet, pNode ) 
		Ntk_NodeGlobalClean( pNode );
    Ntk_NetworkForEachCo( pNet, pNode ) 
		Ntk_NodeGlobalClean( pNode );
}

/**Function*************************************************************

  Synopsis    [Cleans the global structures for all the internal nodes.]

  Description [This functions should be called when some computation
  which involved global BDDs is over, before the global BDD manager 
  is deleted.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkGlobalCleanCo( Ntk_Network_t * pNet )
{
	Ntk_Node_t * pNode;
    Ntk_NetworkForEachCo( pNet, pNode ) 
		Ntk_NodeGlobalClean( pNode );
}


/**Function*************************************************************

  Synopsis    [Copies the array of BDDs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeGlobalCopyFuncs( DdNode ** pbDest, DdNode ** pbSource, int nFuncs )
{
    int i;
    for ( i = 0; i < nFuncs; i++ )
    {
        assert( pbSource[i] != NULL );
        pbDest[i] = pbSource[i];  
    }
}

/**Function*************************************************************

  Synopsis    [Adds the given function to other functions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeGlobalAddToFuncs( DdManager * dd, DdNode ** pbFuncs, DdNode * bExdc, int nFuncs )
{
    DdNode * bTemp;
    int i;
    for ( i = 0; i < nFuncs; i++ )
    {
        pbFuncs[i] = Cudd_bddOr( dd, bTemp = pbFuncs[i], bExdc ); Cudd_Ref( pbFuncs[i] );
        Cudd_RecursiveDeref( dd, bTemp );
    }
}

/**Function*************************************************************

  Synopsis    [Copies the array of BDDs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeGlobalRefFuncs( DdNode ** pbFuncs, int nFuncs )
{
    int i;
    for ( i = 0; i < nFuncs; i++ )
        Cudd_Ref( pbFuncs[i] );
}

/**Function*************************************************************

  Synopsis    [Copies the array of BDDs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeGlobalDerefFuncs( DdManager * dd, DdNode ** pbFuncs, int nFuncs )
{
    int i;
    for ( i = 0; i < nFuncs; i++ )
        Cudd_RecursiveDeref( dd, pbFuncs[i] );
}

/**Function*************************************************************

  Synopsis    [Composes one MV variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Ntk_NodeGlobalConvolveFuncs( DdManager * dd, DdNode * pbFuncs1[], DdNode * pbFuncs2[], int nFuncs )
{
    DdNode * bComp, * bRes, * bTemp;
    int i;

    bRes = b0;   Cudd_Ref( bRes );
    for ( i = 0; i < nFuncs; i++ )
    {
        bComp = Cudd_bddAnd( dd, pbFuncs1[i], pbFuncs2[i] );   Cudd_Ref( bComp );
        bRes  = Cudd_bddOr( dd, bTemp = bRes, bComp );         Cudd_Ref( bRes );
        Cudd_RecursiveDeref( dd, bComp );
        Cudd_RecursiveDeref( dd, bTemp );
    }
    Cudd_Deref( bRes );
    return bRes;
}

/**Function*************************************************************

  Synopsis    [Derives the condition when one MV function is contained in another.]

  Description [Implements the formula F1(x) => F2(x). Which is equal to
  Product[F1k(x) => F2k(x)] = Product[Not(F1k(x)) + F2k(x)] = 
  Not( Sum[F1k(x) * Not(F2k(x))] ). ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Ntk_NodeGlobalImplyFuncs( DdManager * dd, DdNode * pbFuncs1[], DdNode * pbFuncs2[], int nFuncs )
{
    DdNode * bComp, * bRes, * bTemp;
    int i;

    bRes = b0;   Cudd_Ref( bRes );
    for ( i = 0; i < nFuncs; i++ )
    {
        bComp = Cudd_bddAnd( dd, pbFuncs1[i], Cudd_Not(pbFuncs2[i]) );   Cudd_Ref( bComp );
        bRes  = Cudd_bddOr( dd, bTemp = bRes, bComp );                   Cudd_Ref( bRes );
        Cudd_RecursiveDeref( dd, bComp );
        Cudd_RecursiveDeref( dd, bTemp );
    }
    bRes = Cudd_Not( bRes );
    Cudd_Deref( bRes );
    return bRes;
}


/**Function*************************************************************

  Synopsis    [Check ND.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Ntk_NodeGlobalCheckNDFuncs( DdManager * dd, DdNode ** pbFuncs, int nValues )
{
    int i, k;
    for ( i = 0; i < nValues; i++ )
        for ( k = i+1; k < nValues; k++ )
            if ( !Cudd_bddLeq( dd, pbFuncs[i], Cudd_Not(pbFuncs[k]) ) ) // they intersect
                return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Returns the number of nodes in the shared global BDDs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NodeGlobalNumSharedBddNodes( DdManager * dd, Ntk_Network_t * pNet )
{
    Ntk_Node_t * pNode;
    DdNode ** pbFuncsAll, ** pbFuncs;
    int nValuesTotal, RetValue;
    // count the total number of values
    nValuesTotal = 0;
    Ntk_NetworkForEachNode( pNet, pNode )
        nValuesTotal += pNode->nValues;
    Ntk_NetworkForEachCo( pNet, pNode )
        nValuesTotal += pNode->nValues;
    // get all the functions into one array
    pbFuncsAll = ALLOC( DdNode *, nValuesTotal );
    nValuesTotal = 0;
    Ntk_NetworkForEachNode( pNet, pNode )
    {
        pbFuncs = Ntk_NodeReadFuncGlob(pNode);
        if ( pbFuncs == NULL || pbFuncs[0] == NULL )
            continue;
        Ntk_NodeGlobalCopyFuncs( pbFuncsAll + nValuesTotal, pbFuncs, pNode->nValues );
        nValuesTotal += pNode->nValues;
    }
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        pbFuncs = Ntk_NodeReadFuncGlob(pNode);
        if ( pbFuncs == NULL || pbFuncs[0] == NULL )
            continue;
        Ntk_NodeGlobalCopyFuncs( pbFuncsAll + nValuesTotal, pbFuncs, pNode->nValues );
        nValuesTotal += pNode->nValues;
    }
    RetValue = Cudd_SharingSize( pbFuncsAll, nValuesTotal );
    FREE( pbFuncsAll );
    return RetValue;
}


/**Function*************************************************************

  Synopsis    [Computes the global relation of the network.]

  Description [Assumes that the global functions are computed. 
  The global relation is returned. The global variables map in the
  network is replaced by the extended global variable map.
  Global variable map should be set up before calling this procedure.
  The global variable map should contain both the CI and the CO 
  variables, even if we are only building the transition relation.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Ntk_NetworkGlobalRelation( Ntk_Network_t * pNet, int fTransRel, int fReorder )
{
    DdManager * dd;
    Ntk_Node_t * pNode;
    Vmx_VarMap_t * pVmxExt;
    DdNode ** pbCodes, ** pbFuncs;
    DdNode * bRel, * bTemp, * bComp;
    int * pValuesFirst;
    int nVarsIn, v;

    // get hold of the current global variable maps
    pVmxExt = Ntk_NetworkReadVmxGlo(pNet);
    dd = Ntk_NetworkReadDdGlo( pNet );

    // get the codes
    pbCodes = Vmx_VarMapEncodeMap( dd, pVmxExt );
    pValuesFirst = Vm_VarMapReadValuesFirstArray( Vmx_VarMapReadVm(pVmxExt) );
    nVarsIn = Ntk_NetworkReadCiNum(pNet);

    // enable reordering
    if ( fReorder )
        Cudd_AutodynEnable( dd, CUDD_REORDER_SYMM_SIFT );
    else
        Cudd_AutodynDisable( dd );

    bRel = b1;   Cudd_Ref( bRel );
    v = 0;
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        // skip the PO if we are only creating the relation 
        // involving the next state variables
        if ( fTransRel && pNode->Subtype != MV_BELONGS_TO_LATCH )
        {
            v++;
            continue;
        }
        else if ( !fTransRel && pNode->Subtype == MV_BELONGS_TO_LATCH )
        {
            v++;
            continue;
        }

        pbFuncs = Ntk_NodeReadFuncGlob(pNode);
        bComp = Ntk_NodeGlobalConvolveFuncs( dd, pbFuncs, 
            pbCodes + pValuesFirst[nVarsIn + v++], pNode->nValues );  Cudd_Ref( bComp ); 
        bRel = Cudd_bddAnd( dd, bTemp = bRel, bComp );                Cudd_Ref( bRel );
        Cudd_RecursiveDeref( dd, bTemp ); 
        Cudd_RecursiveDeref( dd, bComp ); 
    }
//    fprintf( stdout, "BDD nodes in the %s relation before reordering %d.\n", 
//        (fTransRel? "transition": "output"), Cudd_DagSize(bRel) );
    // reorder and diable reordering
    if ( fReorder )
    {
        Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 100 );
        Cudd_AutodynDisable( dd );
    }

    fprintf( stdout, "BDD nodes in the %s relation after reordering %d.\n", 
        (fTransRel? "transition": "output"), Cudd_DagSize(bRel) );

    Vmx_VarMapEncodeDeref( dd, pVmxExt, pbCodes );
    Cudd_Deref( bRel );
    return bRel;
}

/**Function*************************************************************

  Synopsis    [Sets up the global variable map for CIs only.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vmx_VarMap_t * Ntk_NetworkGlobalCreateVmxCi( Ntk_Network_t * pNet )
{
    Ntk_Node_t * pNode;
    Vmx_VarMap_t * pVmx;
    int * pValues;
    int nVarsIn, nVarsOut, i;

    nVarsIn =  Ntk_NetworkReadCiNum(pNet);
//    nVarsOut = fLatchOnly? Ntk_NetworkReadLatchNum(pNet) : Ntk_NetworkReadCoNum(pNet);
    nVarsOut = 0;
    pValues = ALLOC( int, nVarsIn + nVarsOut );

    i = 0;
    Ntk_NetworkForEachCi( pNet, pNode )
        pValues[i++] = pNode->nValues;
    assert( i == nVarsIn + nVarsOut );

    pVmx = Vmx_VarMapCreateSimple( Ntk_NetworkReadManVm(pNet), 
        Ntk_NetworkReadManVmx(pNet), nVarsIn, nVarsOut, pValues );
    FREE( pValues );
    return pVmx;
}

/**Function*************************************************************

  Synopsis    [Sets up the global variable map for CIs only.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vmx_VarMap_t * Ntk_NetworkGlobalCreateVmxCo( Ntk_Network_t * pNet, bool fLatchOnly )
{
    Ntk_Node_t * pNode;
    Vmx_VarMap_t * pVmx;
    int * pValues;
    int nVarsIn, nVarsOut, i;
    DdManager * dd;
    int nBits;

    nVarsIn =  Ntk_NetworkReadCiNum(pNet);
    nVarsOut = fLatchOnly? Ntk_NetworkReadLatchNum(pNet) : Ntk_NetworkReadCoNum(pNet);
    pValues = ALLOC( int, nVarsIn + nVarsOut );

    i = 0;
    Ntk_NetworkForEachCi( pNet, pNode )
        pValues[i++] = pNode->nValues;
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        if ( !fLatchOnly || pNode->Subtype == MV_BELONGS_TO_LATCH )
            pValues[i++] = pNode->nValues;
    }
    assert( i == nVarsIn + nVarsOut );

    pVmx = Vmx_VarMapCreateSimple( Ntk_NetworkReadManVm(pNet), 
        Ntk_NetworkReadManVmx(pNet), nVarsIn, nVarsOut, pValues );
    FREE( pValues );

    // extend the manager if necessary
    nBits = Vmx_VarMapReadBitsNum( pVmx );
    dd = Ntk_NetworkReadDdGlo( pNet );
    if ( dd && dd->size < nBits )
    {
        for ( i = dd->size; i < nBits; i++ )
            Cudd_bddNewVar( dd );
    }
    return pVmx;
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



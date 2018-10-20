/**CFile****************************************************************

  FileName    [ntkiGlobal.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Computation of global MDDs for the network.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkiGlobal.c,v 1.5 2004/01/06 21:02:53 alanmi Exp $]

***********************************************************************/

#include "ntkiInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Ntk_NetworkGlobalMddCompute_rec( DdManager * dd, Ntk_Node_t * pNode );

static void Ntk_NodeGlobalMddCopy( DdNode * pDestin[], DdNode * pSource[], int nValues, bool fRef );
static void Ntk_NodeGlobalMddDeref( DdManager * dd, DdNode * pFuncs[], int nValues );
static void Ntk_NodeGlobalMddSetZero( DdManager * dd, DdNode * pFuncs[], int nValues );
static bool Ntk_NodeGlobalMddCheckND( DdManager * dd, DdNode * pFuncs[], int nValues );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Derives the global MDDs for the primary and the exdc networks.]

  Description [Returns the MDDs computed for the network (pNet) in the manager (dd),
  the variable map that was used to derive the MDDs, and the DFS ordered array of
  CI names used to drived the extended variable map, which will be used to 
  encode the CI variables for the construction of global MDDs. This procedure
  also compute the global MDDs of the EXDC network with the same variable ordering
  that is assumed for the primary network. ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkGlobalMdd( DdManager * dd, Ntk_Network_t * pNet, 
    Vm_VarMap_t ** ppVm, char *** ppCiNames,        // return values (CI var map, and CI names)
    DdNode **** pppMdds, DdNode **** pppMddsExdc )  // return values (global MDDs for net and exdc)
{
    DdNode *** ppMdds, *** ppMddsExdc = NULL;
    char ** psLeavesByName, ** psRootsByName;
    int nLeaves, nRoots;
    Vmx_VarMap_t * pVmx;
    int * pValuesOut;

    // get the CI variable order
    psLeavesByName = Ntk_NetworkOrderArrayByName( pNet, 0 );
    // collect the output names and values
    psRootsByName = Ntk_NetworkGlobalGetOutputsAndValues( pNet, &pValuesOut );
    // get the variable map
    pVmx = Ntk_NetworkGlobalGetVmx( pNet, psLeavesByName );

    // compute the MDDs of both networks (normal and exdc)
    nLeaves = Ntk_NetworkReadCiNum(pNet);
    nRoots  = Ntk_NetworkReadCoNum(pNet);
    ppMdds = Ntk_NetworkGlobalMddCompute( dd, pNet, psLeavesByName, nLeaves, psRootsByName, nRoots, pVmx );
    // derive the MDDs for the EXDC network
    if ( pNet->pNetExdc )
        ppMddsExdc = Ntk_NetworkGlobalMddCompute( dd, pNet->pNetExdc, psLeavesByName, nLeaves, psRootsByName, nRoots, pVmx );
    // undo the table
    FREE( psRootsByName );
    FREE( pValuesOut );

    // return
    *ppVm        = Vmx_VarMapReadVm( pVmx );
    *ppCiNames   = psLeavesByName;
    *pppMdds     = ppMdds;
    *pppMddsExdc = ppMddsExdc;
}

/**Function*************************************************************

  Synopsis    [Computes global MDDs for one network.]

  Description [This procedure derives the global MDDs for the network (pNet),
  when the mapping of the CIs into the MDD variable order is given (tLeaves),
  and the variable map representing the CI space is avaiable. This variable
  map is used to derive the codes of the values of each MV variable.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode *** Ntk_NetworkGlobalMddCompute( DdManager * dd, Ntk_Network_t * pNet, 
    char ** psLeavesByName, int nLeaves, char ** psRootsByName, int nRoots, Vmx_VarMap_t * pVmx )
{
    ProgressBar * pProgress;
    Ntk_Node_t * pNode, * pNodeCo, * pNodeCi, * pDriver;
    DdNode *** ppbOutputs, ** pbCodes, ** pbFuncs;
    int * pValuesFirst;
    int i, nBits, nNodes;

    // extend the global manager if necessary
    nBits = Vmx_VarMapReadBitsNum( pVmx );
    for ( i = dd->size; i < nBits; i++ )
        Cudd_bddIthVar( dd, i );

    // encode the CI variables
    pbCodes = Vmx_VarMapEncodeMap( dd, pVmx );
    // get the pointer to the first values
    pValuesFirst = Vm_VarMapReadValuesFirstArray( Vmx_VarMapReadVm(pVmx) );
    // assign the elementary BDDs to the values of the CIs
    assert( Ntk_NetworkReadCiNum(pNet) <= nLeaves  );
    for ( i = 0; i < nLeaves; i++ )
    {
        pNodeCi = Ntk_NetworkFindNodeByName( pNet, psLeavesByName[i] );
//        assert( pNodeCi );
        // EXDC network may have some CIs missing
        if ( pNodeCi == NULL )
            continue;
        // copy the elementary MDDs into the CIs
        pbFuncs = ALLOC( DdNode *, pNodeCi->nValues );
        Ntk_NodeGlobalMddCopy( pbFuncs, pbCodes + pValuesFirst[i], pNodeCi->nValues, 1 );
        Ntk_NodeWriteFuncGlo( pNodeCi, pbFuncs );
    }
    // deref the codes 
    Vmx_VarMapEncodeDeref( dd, pVmx, pbCodes );


    // start the progress var
    pProgress = Extra_ProgressBarStart( stdout, Ntk_NetworkReadNodeIntNum(pNet) );
    // link internal nodes into the array
    Ntk_NetworkDfs( pNet, 1 );
    // recursively construct the MDDs
    nNodes = 0;
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
        if ( pNode->Type == MV_NODE_INT )
        {
            // compute the global MDDs assuming that 
            // the global MDDs of the fanins are available
            pbFuncs = Ntk_NodeGlobalMdd( dd, pNode );
            // set the global MDDs at the node
            Ntk_NodeWriteFuncGlo( pNode, pbFuncs );
            // update the progress bar
            if ( ++nNodes % 20 == 0 )
                Extra_ProgressBarUpdate( pProgress, nNodes, NULL );
        }
    Extra_ProgressBarStop( pProgress );


    // collect the BDDs of the CO nodes
    assert( Ntk_NetworkReadCoNum(pNet) <= nRoots  );
    ppbOutputs = ALLOC( DdNode **, nRoots );
    for ( i = 0; i < nRoots; i++ )
    {
        pNodeCo = Ntk_NetworkFindNodeByName( pNet, psRootsByName[i] );
//        assert( pNodeCo );
        // EXDC network may have some COs missing
        if ( pNodeCo == NULL )
        {
            ppbOutputs[i] = ALLOC( DdNode *, 2 );
            Ntk_NodeGlobalMddSetZero( dd, ppbOutputs[i], 2 );
            continue;
        }
        // get the CO driver
        pDriver = Ntk_NodeReadFaninNode(pNodeCo,0);
        // get hold of the MDDs for this output
        pbFuncs = Ntk_NodeReadFuncGlo( pDriver );
        // copy the elementary MDDs into the resulting array
        ppbOutputs[i] = ALLOC( DdNode *, pDriver->nValues );
        Ntk_NodeGlobalMddCopy( ppbOutputs[i], pbFuncs, pDriver->nValues, 1 );
    }

    // deref the temporary MDDs at CI nodes
    Ntk_NetworkForEachCi( pNet, pNodeCi )
    {
        pbFuncs = Ntk_NodeReadFuncGlo( pNodeCi );
        if ( pbFuncs )
        {
            Ntk_NodeWriteFuncGlo( pNodeCi, NULL );
            Ntk_NodeGlobalMddDeref( dd, pbFuncs, pNodeCi->nValues );
            FREE( pbFuncs );
        }
    }
    // deref the intermediate MDDs stored at the internal nodes nodes
    Ntk_NetworkForEachNode( pNet, pNode )
    {
        pbFuncs = Ntk_NodeReadFuncGlo( pNode );
        if ( pbFuncs )
        {
            Ntk_NodeWriteFuncGlo( pNode, NULL );
            Ntk_NodeGlobalMddDeref( dd, pbFuncs, pNode->nValues );
            FREE( pbFuncs );
        }
    }
    // return the resulting MDDs
    return ppbOutputs;
}

/**Function*************************************************************

  Synopsis    [Recursively computes global MDDs.]

  Description [Recursively computes global MDDs for a node after
  the trivial global MDDs have been set at the CI nodes of the network.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkGlobalMddCompute_rec( DdManager * dd, Ntk_Node_t * pNode )
{
    Ntk_Node_t * pFanin;
    Ntk_Pin_t * pPin;
    DdNode ** pbFuncs;

    // set the traversal ID to mark that this node is visited
    assert( pNode->TravId != pNode->pNet->nTravIds );
    pNode->TravId = pNode->pNet->nTravIds;

    // quit if this is a PI
    if ( Ntk_NodeIsCi( pNode ) )
        return;

    // compute the global BDDs for the fanins
    Ntk_NodeForEachFanin( pNode, pPin, pFanin )
        if ( pFanin->TravId != pNode->pNet->nTravIds )
            Ntk_NetworkGlobalMddCompute_rec( dd, pFanin );

    // compute the global MDDs assuming that the global MDDs of the fanins are available
    pbFuncs = Ntk_NodeGlobalMdd( dd, pNode );

    // set the global MDDs at the node
    Ntk_NodeWriteFuncGlo( pNode, pbFuncs );
}


/**Function*************************************************************

  Synopsis    [This procedure computes global MDDs for one node.]

  Description [This procedure computes global MDDs for one node
  assuming the the global MDDs for the fanins are already available.
  Note also that this procedure treats correctly the case when
  the global MDDs are non-deterministic. In this case the default
  value (if it is present in the current representation of the node)
  cannot be used. ]

  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode ** Ntk_NodeGlobalMdd( DdManager * dd, Ntk_Node_t * pNode )
{
    Vm_VarMap_t * pVm;
    Cvr_Cover_t * pCvr, * pCvrNew;
    Mvr_Relation_t * pMvr;
    Ntk_Node_t * pFanin;
    Ntk_Pin_t * pPin;
    DdNode ** pbArray;
    DdNode ** pbFuncs;
    bool fNonDeterministic;
    int Index;

    pVm  = Ntk_NodeReadFuncVm( pNode );
    pbArray = ALLOC( DdNode *, Vm_VarMapReadValuesNum(pVm) );

    // compute the global MDD for this node
    Index = 0;
    fNonDeterministic = 0;
    Ntk_NodeForEachFanin( pNode, pPin, pFanin )
    {
        pbFuncs = Ntk_NodeReadFuncGlo( pFanin );
        Ntk_NodeGlobalMddCopy( pbArray + Index, pbFuncs, pFanin->nValues, 0 );
        Index += pFanin->nValues;

        if ( fNonDeterministic == 0 )
        {
            fNonDeterministic = Ntk_NodeGlobalMddCheckND( dd, pbFuncs, pFanin->nValues );
//            if ( fNonDeterministic )
//                fprintf( Ntk_NetworkReadMvsisOut(pNet), "Node <%s> is non-deterministic.\n", Ntk_NodeGetNameLong(pFanin) );
        }
    }

    // here is the problem: in ND networks, default i-set cannot be
    // used to compute global MDDs (for example, when we are
    // collapsing non-deterministic constant a = {0,1} into 
    // F = a (+) b, we get incorrect result, if F has a default value)

    // here if the global MDD are non-deterministic and the def value is used
    // we make sure that Cvr does not have the default value 
    // when we call to construct global MDD
   
    pCvr = Ntk_NodeGetFuncCvr( pNode );
    pCvrNew = NULL;
    if ( fNonDeterministic && Cvr_CoverReadDefault( pCvr ) >= 0 )
    {
        // currently, procedure Mvc_CoverComplement() does not work,
        // so, we compute the default i-set in a round-about way:
        // get hold of Mvr
        pMvr = Ntk_NodeGetFuncMvr( pNode );
        // derive the new Cvr
        pCvrNew = Fnc_FunctionDeriveCvrFromMvr( Ntk_NodeReadManMvc(pNode), pMvr, 0 ); // don't use the default
    }

    // compute the global BDDs for each i-set (the result is returned in pbFuncs)
    pbFuncs = ALLOC( DdNode *, pNode->nValues );
    if ( pCvrNew )
    {
        Fnc_FunctionDeriveMddFromCvr( dd, pVm, pCvrNew, pbArray, pbFuncs );
        Cvr_CoverFree( pCvrNew );
    }
    else
        Fnc_FunctionDeriveMddFromCvr( dd, pVm, pCvr, pbArray, pbFuncs );

    FREE( pbArray );
    return pbFuncs;
}


/**Function*************************************************************

  Synopsis    [This procedure computes global MDDs from the local MDD.]

  Description [This procedure takes the global BDD manager (ddGlo),
  the local function (bFuncLoc) and the node (pNode). This procedure
  assumes that the local function is expressed using the same BDD manager
  as the local relation, currently present at the node (pNode->pF->pMvr).
  It also assumes that the global BDD of the fanins of the node have
  been computed and are currently available (pFanin->pF->pGlo).]

  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Ntk_NodeGlobalMddCollapse( 
    DdManager * ddGlo,    // the global BDD manager
    DdNode * bFuncLoc,    // the local BDD to be collapsed with the global BDDs of the fanins
    Ntk_Node_t * pNode )  // the node, whose fanins have global BDDs 
{
    Ntk_Node_t * pFanin;
    Ntk_Pin_t * pPin;
    Vm_VarMap_t * pVm;
    Mvr_Relation_t * pMvr;
    Mvc_Cover_t * pCover;
    DdNode ** pbFuncs;
    DdNode ** ppMddFaninGlo;
    DdNode * bMddNodeGlo;
    int Index, nFanins;

    // get the number of fanins
    nFanins = Ntk_NodeReadFaninNum( pNode );
    // get hold of the var map and the local relation of the node
    pVm  = Ntk_NodeReadFuncVm( pNode );
    pMvr = Ntk_NodeReadFuncMvr( pNode );

    // create the Mvc cover equivalent to the given local function
    pCover = Fnc_FunctionDeriveSopFromMdd( Ntk_NodeReadManMvc(pNode), pMvr, bFuncLoc, bFuncLoc, nFanins );

    // get the array of global MDDs for the fanins for each value of each fanin
    ppMddFaninGlo = ALLOC( DdNode *, Vm_VarMapReadValuesNum(pVm) );
    // retrieve the global MDD from the fanins
    Index = 0;
    Ntk_NodeForEachFanin( pNode, pPin, pFanin )
    {
        pbFuncs = Ntk_NodeReadFuncGlo( pFanin );
        Ntk_NodeGlobalMddCopy( ppMddFaninGlo + Index, pbFuncs, pFanin->nValues, 0 );
        Index += pFanin->nValues;
    }

    // compose the cover with the global MDDs
    bMddNodeGlo = Fnc_FunctionDeriveMddFromSop( ddGlo, pVm, pCover, ppMddFaninGlo );
    // deallocate the cover
    Mvc_CoverFree( pCover );
    // deallocate storage
    FREE( ppMddFaninGlo );
    return bMddNodeGlo; // returns non-referenced BDD!
}


/**Function*************************************************************

  Synopsis    [Computes empty global MDDs.]

  Description [Useful for empty EXDC networks.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode *** Ntk_NetworkGlobalMddEmpty( DdManager * dd, int * pValues, int nOuts )
{
    DdNode *** ppMdds;
    int i, v;
    ppMdds = ALLOC( DdNode **, nOuts );
    for ( i = 0; i < nOuts; i++ )
    {
        ppMdds[i] = ALLOC( DdNode *, pValues[i] );
        for ( v = 0; v < pValues[i]; v++ )
        {
            ppMdds[i][v] = b0;  Cudd_Ref( ppMdds[i][v] );
        }
    }
    return ppMdds;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode *** Ntk_NetworkGlobalMddDup( DdManager * dd, int * pValues, int nOuts, DdNode *** ppMdds1 )
{
    DdNode *** ppMdds;
    int i, v;
    ppMdds = ALLOC( DdNode **, nOuts );
    for ( i = 0; i < nOuts; i++ )
    {
        ppMdds[i] = ALLOC( DdNode *, pValues[i] );
        for ( v = 0; v < pValues[i]; v++ )
        {
            ppMdds[i][v] = ppMdds1[i][v];  Cudd_Ref( ppMdds[i][v] );
        }
    }
    return ppMdds;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode *** Ntk_NetworkGlobalMddOr( DdManager * dd, int * pValues, int nOuts, DdNode *** ppMdds1, DdNode *** ppMdds2 )
{
    DdNode *** ppMdds;
    int i, v;
    ppMdds = ALLOC( DdNode **, nOuts );
    for ( i = 0; i < nOuts; i++ )
    {
        ppMdds[i] = ALLOC( DdNode *, pValues[i] );
        for ( v = 0; v < pValues[i]; v++ )
        {
            ppMdds[i][v] = Cudd_bddOr( dd, ppMdds1[i][v], ppMdds2[i][v] );  Cudd_Ref( ppMdds[i][v] );
        }
    }
    return ppMdds;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode *** Ntk_NetworkGlobalMddConvertExdc( DdManager * dd, int * pValues, int nOuts, DdNode *** ppMddsExdc )
{
    DdNode *** ppMdds;
    int * pValuesExdc;
    int i, v;
    ppMdds = ALLOC( DdNode **, nOuts );
    for ( i = 0; i < nOuts; i++ )
    {
        ppMdds[i] = ALLOC( DdNode *, pValues[i] );
        for ( v = 0; v < pValues[i]; v++ )
        {
            ppMdds[i][v] = ppMddsExdc[i][1];  Cudd_Ref( ppMdds[i][v] );
        }
    }

    pValuesExdc = ALLOC( int, nOuts );
    for ( i = 0; i < nOuts; i++ )
        pValuesExdc[i] = 2;
    Ntk_NetworkGlobalMddDeref( dd, pValuesExdc, nOuts, ppMddsExdc );
    FREE( pValuesExdc );

    return ppMdds;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkGlobalMddMinimize( DdManager * dd, int * pValues, int nOuts, DdNode *** ppMdds1, DdNode *** ppMdds2 )
{
    DdNode * bTemp;
    int i, v;
    for ( i = 0; i < nOuts; i++ )
        for ( v = 0; v < pValues[i]; v++ )
        {
            ppMdds1[i][v] = Cudd_bddRestrict( dd, bTemp = ppMdds1[i][v], Cudd_Not(ppMdds2[i][v]) );  Cudd_Ref( ppMdds1[i][v] );
            Cudd_RecursiveDeref( dd, bTemp );
        }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkGlobalMddDeref( DdManager * dd, int * pValues, int nOuts, DdNode *** ppMdds )
{
    int i, v;
    for ( i = 0; i < nOuts; i++ )
    {
        for ( v = 0; v < pValues[i]; v++ )
            Cudd_RecursiveDeref( dd, ppMdds[i][v] );
        FREE( ppMdds[i] );
    }
    FREE( ppMdds );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Ntk_NetworkGlobalMddCheckEquality( DdManager * dd, int * pValues, int nOuts, DdNode *** ppMdds1, DdNode *** ppMdds2 )
{
    int i, v;
    for ( i = 0; i < nOuts; i++ )
        for ( v = 0; v < pValues[i]; v++ )
            if ( ppMdds1[i][v] != ppMdds2[i][v] )
                return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description [Teturns 1 if ppMdds1 is contained in ppMdds2.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Ntk_NetworkGlobalMddCheckContainment( DdManager * dd, int * pValues, int nOuts, DdNode *** ppMdds1, DdNode *** ppMdds2 )
{
    int i, v;
    for ( i = 0; i < nOuts; i++ )
        for ( v = 0; v < pValues[i]; v++ )
            if ( !Cudd_bddLeq( dd, ppMdds1[i][v], ppMdds2[i][v] ) )
                return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description [Returns 1 if ppMdds are non-deterministic.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Ntk_NetworkGlobalMddCheckND( DdManager * dd, int * pValues, int nOuts, DdNode *** ppMdds )
{
    int i, v, w;
    for ( i = 0; i < nOuts; i++ )
        for ( v = 0;   v < pValues[i]; v++ )
        for ( w = v+1; w < pValues[i]; w++ )
            if ( !Cudd_bddLeq( dd, ppMdds[i][v], Cudd_Not(ppMdds[i][w]) ) ) // they intersect
                return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Ntk_NetworkGlobalMddErrorTrace( DdManager * dd, int * pValues, int nOuts, DdNode *** ppMdds1, DdNode *** ppMdds2, int * Out, int * Value )
{
    DdNode * bRes;
    int i, v;
    for ( i = 0; i < nOuts; i++ )
        for ( v = 0; v < pValues[i]; v++ )
            if ( !Cudd_bddLeq( dd, ppMdds1[i][v], ppMdds2[i][v] ) )
            {
                bRes = Cudd_bddAnd( dd, ppMdds1[i][v], Cudd_Not(ppMdds2[i][v]) );
                *Out = i;
                *Value = v;
                return bRes;
            }
    return NULL;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkGlobalMddPrintErrorTrace( FILE * pFile, DdManager * dd, Vmx_VarMap_t * pVmx, DdNode * bTrace, int Out, char * pNameOut, int nValue, char * psLeavesByName[] )
{
    Vm_VarMap_t * pVm;
    char * sCube;
    int *  pValues;
    int *  pZddVarsLog;  
    int *  pZddVarsPos;  
    int    nVarsIn, nValuesNum;
    int i, v, Index;

    pVm        = Vmx_VarMapReadVm( pVmx );
    nVarsIn    = Vm_VarMapReadVarsInNum( pVm );
    nValuesNum = Vm_VarMapReadValuesNum( pVm );
    pValues    = Vm_VarMapReadValuesArray( pVm );

    pZddVarsLog = ALLOC( int, nValuesNum );  // logarithmic encoding goes here
    pZddVarsPos = ALLOC( int, nValuesNum );  // positional encoding goes here

    // get the cube
    sCube = ALLOC( char, dd->size );
    Cudd_bddPickOneCube( dd, bTrace, sCube );
    // translate the cube into the log-encoded cube
    for ( i = 0; i < dd->size; i++ )
        pZddVarsLog[i] = sCube[i];
    FREE( sCube );

    // decode the cube
    Vmx_VarMapDecode( pVmx, pZddVarsLog, pZddVarsPos );

    // write out the decoded cube
    fprintf( pFile, "po \"%s\": (", pNameOut );
    Index = 0;
    for ( i = 0; i < nVarsIn; i++ )
    {
        for ( v = 0; v < pValues[i]; v++ )
            if ( pZddVarsPos[Index + v] )
            {
                fprintf( pFile, "%d ", v );
                break;
            }
        Index += pValues[i];
    }
    fprintf( pFile, "-> %d)", nValue );

    FREE( pZddVarsLog );
    FREE( pZddVarsPos );
}

/**Function*************************************************************

  Synopsis    [Returns the extended variable map for the construction of global MDD.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vmx_VarMap_t * Ntk_NetworkGlobalGetVmx( Ntk_Network_t * pNet, char ** psLeavesByName )
{
    Vmx_VarMap_t * pVmx;
    Vm_VarMap_t * pVm;
    Ntk_Node_t * pNodeCi;
    int * pValues;
    int nVars, i;

    // create the global variable map
    nVars = Ntk_NetworkReadCiNum( pNet );
    pValues = ALLOC( int, nVars );
    if ( psLeavesByName )
    {
        for ( i = 0; i < nVars; i++ )
        {
            pNodeCi = Ntk_NetworkFindNodeByName( pNet, psLeavesByName[i] );
            assert( pNodeCi );
            pValues[i] = pNodeCi->nValues;
        }
    }
    else
    {
        i = 0;
        Ntk_NetworkForEachCi( pNet, pNodeCi )
            pValues[i++] = pNodeCi->nValues;
    }
    pVm = Vm_VarMapLookup( Fnc_ManagerReadManVm(pNet->pMan), nVars, 0, pValues );
    pVmx = Vmx_VarMapLookup( Fnc_ManagerReadManVmx(pNet->pMan), pVm, -1, NULL );
    FREE( pValues );
    return pVmx;
}

/**Function*************************************************************

  Synopsis    [Collects the output values.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int * Ntk_NetworkGlobalGetOutputValues( Ntk_Network_t * pNet )
{
    Ntk_Node_t * pNode;
    int * pValuesOut;
    int iOutput;

    pValuesOut = ALLOC( int, Ntk_NetworkReadCoNum(pNet) );
    iOutput = 0;
    Ntk_NetworkForEachCo( pNet, pNode )
        pValuesOut[iOutput++] = pNode->nValues;

    return pValuesOut;
}

/**Function*************************************************************

  Synopsis    [Collects the output names.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char ** Ntk_NetworkGlobalGetOutputs( Ntk_Network_t * pNet )
{
    char ** psRoots;
    Ntk_Node_t * pNode;
    int i;
    // put the nodes in the support into the hash table
    psRoots    = ALLOC( char *, Ntk_NetworkReadCoNum(pNet) );
    i = 0;
    Ntk_NetworkForEachCo( pNet, pNode )
        psRoots[i++]= pNode->pName;
    assert( i == Ntk_NetworkReadCoNum(pNet) ); 
    return psRoots;
}


/**Function*************************************************************

  Synopsis    [Collects the output names.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char ** Ntk_NetworkGlobalGetOutputsAndValues( Ntk_Network_t * pNet, int ** ppValues )
{
    char ** psLeaves;
    int * pValuesOut;
    Ntk_Node_t * pNode;
    int i;
    // put the nodes in the support into the hash table
    psLeaves    = ALLOC( char *, Ntk_NetworkReadCoNum(pNet) );
    pValuesOut = ALLOC( int, Ntk_NetworkReadCoNum(pNet) );
    i = 0;
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        psLeaves[i]     = pNode->pName;
        pValuesOut[i++] = pNode->nValues;
    }
    assert( i == Ntk_NetworkReadCoNum(pNet) ); 
    *ppValues = pValuesOut;
    return psLeaves;
}


/**Function*************************************************************

  Synopsis    [Copies the array of BDD with or without referencing them.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeGlobalMddCopy( DdNode * pDestin[], DdNode * pSource[], int nValues, bool fRef )
{
    int i;
    for ( i = 0; i < nValues; i++ )
    {
        assert( pSource[i] ); // the fanin global MDDs should be available
        pDestin[i] = pSource[i];
        if ( fRef )
            Cudd_Ref( pDestin[i] );
    }
}

/**Function*************************************************************

  Synopsis    [Copies the array of BDD with or without referencing them.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeGlobalMddDeref( DdManager * dd, DdNode * pFuncs[], int nValues )
{
    int i;
    for ( i = 0; i < nValues; i++ )
        Cudd_RecursiveDeref( dd, pFuncs[i] );
}

/**Function*************************************************************

  Synopsis    [Copies the array of BDD with or without referencing them.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeGlobalMddSetZero( DdManager * dd, DdNode * pFuncs[], int nValues )
{
    int i;
    for ( i = 0; i < nValues; i++ )
    {
        pFuncs[i] = b0;  Cudd_Ref( pFuncs[i] );
    }
}

/**Function*************************************************************

  Synopsis    [Copies the array of BDD with or without referencing them.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Ntk_NodeGlobalMddCheckND( DdManager * dd, DdNode * pFuncs[], int nValues )
{
    int i, k;
    for ( i = 0; i < nValues; i++ )
        for ( k = i+1; k < nValues; k++ )
            if ( !Cudd_bddLeq( dd, pFuncs[i], Cudd_Not(pFuncs[k]) ) ) // they intersect
                return 1;
    return 0;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



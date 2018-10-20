/**CFile****************************************************************

  FileName    [dcmnGlobal.c]

  PackageName [New don't care manager.]

  Synopsis    [Global BDD computation.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - July 13, 2003.]

  Revision    [$Id: fmnGlobal.c,v 1.2 2003/11/18 18:55:12 alanmi Exp $]

***********************************************************************/

#include "fmnInt.h"


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     EXPORTED FUNCTIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes the global BDDs and assigns them to the nodes.]

  Description [Used the manager to reorder the BDDs. If the flag is 0, does not 
  perform dynamic variable reordering.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dcmn_NetworkBuildGlo( Dcmn_Man_t * pMan, Ntk_Network_t * pNet, bool fExdc, int fUseNsc )
{
	Ntk_Node_t * pNode;
    DdNode ** pbFuncs;
    int Num, clk;
    int * pValuesFirst;

    pValuesFirst = Vm_VarMapReadValuesFirstArray(pMan->pVm);

    // assign the elementary BDDs to the PI nodes
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        Num    = Ntk_NodeReadFuncNumber( pNode );
        pbFuncs = pMan->pbCodes + pValuesFirst[Num]; 
        Ntk_NodeWriteFuncGlob( pNode, pbFuncs );
        Ntk_NodeWriteFuncDd( pNode, pMan->ddGlo );
    }

    // assign the symbolic variables to the ND nodes
    Ntk_NetworkForEachNode( pNet, pNode )
    {
        if ( !Ntk_NodeReadFuncNonDet(pNode) )
            continue;
        Num = Ntk_NodeReadFuncNumber( pNode );
        pbFuncs = pMan->pbCodes + pValuesFirst[Num]; 
        Ntk_NodeWriteFuncGlobS( pNode, pbFuncs );
        Ntk_NodeWriteFuncDd( pNode, pMan->ddGlo );
    }

    // enable variable reordering
    if ( !fExdc && pMan->pPars->fDynEnable )
        Cudd_AutodynEnable( pMan->ddGlo, CUDD_REORDER_SYMM_SIFT );

    // order all the nodes of the network
    Ntk_NetworkDfs( pNet, 1 );
    // collect the special nodes
    pMan->nSpecials = 0;
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
        if ( pNode->Type != MV_NODE_CI )
        {
            if ( Dcmn_NodeBuildGlo( pMan, pNode, fUseNsc ) )
                return 1; // timeout has 
        }

	if ( pMan->pPars->fVerbose )
	{
//		fprintf( Ntk_NetworkReadMvsisOut(pNet), "The number of nodes in the shared BDD = %d\n", Dcmn_UtilsNumSharedBddNodes(pMan) );
//		fflush( Ntk_NetworkReadMvsisOut(pNet) );
	}
    // reorder the BDDs one more time
    if ( !fExdc && pMan->pPars->fDynEnable )
    {
		clk = clock();
		// perform the last-gasp reordering
		if ( pMan->pPars->fVerbose )
			fprintf( Ntk_NetworkReadMvsisOut(pNet), "Reordering variables...\n" );
		Cudd_ReduceHeap( pMan->ddGlo, CUDD_REORDER_SYMM_SIFT, 1 );
		pMan->timeReorder += clock() - clk;
		if ( pMan->pPars->fVerbose )
		{
//			fprintf( Ntk_NetworkReadMvsisOut(pNet), "The number of nodes in the shared BDD after reordering  = %d\n", Dcmn_UtilsNumSharedBddNodes(pMan) );
//			fflush( Ntk_NetworkReadMvsisOut(pNet) );
		}
        // disable variable reordering
        Cudd_AutodynDisable( pMan->ddGlo );
    }
    return 0;
}


/**Function*************************************************************

  Synopsis    [Builds the BDD for the node if the fanin BDDs are available.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dcmn_NodeBuildGlo( Dcmn_Man_t * pMan, Ntk_Node_t * pNode, int fUseNsc )
{
    Cvr_Cover_t * pCvr, * pCvrNew;
    Mvr_Relation_t * pMvr;
    Vm_VarMap_t * pVm;
    Ntk_Pin_t * pPin;
    Ntk_Node_t * pFanin;
    DdNode ** pbFuncs;
    bool fNonDeterministic = 0;
    int i, nValues;

    // check the timeout
    if ( 0 )
        return 1; // timeout


    // compute the global BDD
    if ( pNode->Type == MV_NODE_CO )
    {
//        pbFuncs = Ntk_NodeReadFuncGlob( Ntk_NodeReadFaninNode(pNode,0) );
        pFanin = Ntk_NodeReadFaninNode(pNode,0);
        if ( fUseNsc )
        { // if this is an ND node, use symbolic variable
            if ( Ntk_NodeReadFuncNonDet(pFanin) )
                pbFuncs = Ntk_NodeReadFuncGlobS(pFanin);
            else
                pbFuncs = Ntk_NodeReadFuncGlob(pFanin);
        }
        else
            pbFuncs = Ntk_NodeReadFuncGlob(pFanin);

        // set the global BDD of the node
        Ntk_NodeWriteFuncGlob( pNode, pbFuncs );   
        Ntk_NodeWriteFuncDd( pNode, pMan->ddGlo ); 
        return 0;
    }

    // realloc array if necessary
    pVm = Ntk_NodeReadFuncVm(pNode);
    nValues = Vm_VarMapReadValuesNum(pVm);
    if ( pMan->nArrayAlloc < nValues )
    {
        assert( 0 );
        pMan->pbArray  = REALLOC( DdNode *, pMan->pbArray,  2 * nValues );
        pMan->pbArray2 = REALLOC( DdNode *, pMan->pbArray2, 2 * nValues );
        pMan->nArrayAlloc = 2 * nValues;
    }

    // collect the fanin BDDs and make sure the fanins are built
    i = 0;
    Ntk_NodeForEachFanin( pNode, pPin, pFanin )
    {
        if ( fUseNsc )
        { // if this is an ND node, use symbolic variable
            if ( Ntk_NodeReadFuncNonDet(pFanin) )
                pbFuncs = Ntk_NodeReadFuncGlobS(pFanin);
            else
                pbFuncs = Ntk_NodeReadFuncGlob(pFanin);
        }
        else
            pbFuncs = Ntk_NodeReadFuncGlob(pFanin);

        Dcmn_UtilsCopyFuncs( pMan->pbArray + i, pbFuncs, pFanin->nValues );
        i += pFanin->nValues;

        // check whether the input MDDs are non-deterministic
        if ( fNonDeterministic == 0 )
            fNonDeterministic = Dcmn_UtilsCheckNDFuncs( pMan->ddGlo, pbFuncs, pFanin->nValues );
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
        Fnc_FunctionDeriveMddFromCvr( pMan->ddGlo, pVm, pCvrNew, pMan->pbArray, pMan->pbArray2 ); 
        Cvr_CoverFree( pCvrNew );
    }
    else
        Fnc_FunctionDeriveMddFromCvr( pMan->ddGlo, pVm, pCvr, pMan->pbArray, pMan->pbArray2 );
    pbFuncs = pMan->pbArray2;

    // set the global BDDs of the node
    Ntk_NodeWriteFuncGlob( pNode, pbFuncs );   
    Ntk_NodeWriteFuncDd( pNode, pMan->ddGlo ); 

    // deref this global BDD
    Dcmn_UtilsDerefFuncs( pMan->ddGlo, pbFuncs, pNode->nValues );

    // if the node is ND, add it to the list of special nodes
    if ( Ntk_NodeReadFuncNonDet(pNode) )
    {
        assert( pMan->nSpecials < pMan->nSpecialsAlloc );
        pMan->ppSpecials[ pMan->nSpecials++ ] = pNode;
    }

    return 0;
}

/**Function*************************************************************

  Synopsis    [Builds the BDD for the node if the fanin BDDs are available.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dcmn_NodeBuildGloZ( Dcmn_Man_t * pMan, Ntk_Node_t * pNode, int fUseNsc )
{
    Cvr_Cover_t * pCvr, * pCvrNew;
    Mvr_Relation_t * pMvr;
    Vm_VarMap_t * pVm;
    Ntk_Pin_t * pPin;
    Ntk_Node_t * pFanin;
    DdNode ** pbFuncs;
    bool fNonDeterministic = 0;
    int i;

    // check the timeout
    if ( 0 )
        return 1; // timeout

    // compute the global BDD
    if ( pNode->Type == MV_NODE_CO )
    {
//        pbFuncs = Ntk_NodeReadFuncGlobZ( Ntk_NodeReadFaninNode(pNode,0) );
        pFanin = Ntk_NodeReadFaninNode(pNode,0);

        if ( fUseNsc )
        { // if this is an ND node, use symbolic variable
            if ( Ntk_NodeReadFuncNonDet(pFanin) )
                pbFuncs = Ntk_NodeReadFuncGlobS(pFanin);
            else
            {
                pbFuncs = Ntk_NodeReadFuncGlobZ(pFanin);
                assert( pbFuncs[0] != NULL );

//                if ( pbFuncs[0] == NULL )
//                {
//                    pbFuncs = Ntk_NodeReadFuncGlob(pFanin);
//                    assert( pbFuncs[0] );
//                }
            }
        }
        else
        {
            pbFuncs = Ntk_NodeReadFuncGlobZ(pFanin);
            assert( pbFuncs[0] != NULL );
//            if ( pbFuncs[0] == NULL )
//            {
//                pbFuncs = Ntk_NodeReadFuncGlob(pFanin);
//                assert( pbFuncs[0] );
//            }
        }

        // set the global BDD of the node
        Ntk_NodeWriteFuncGlobZ( pNode, pbFuncs );   
//printf( "setting %s\n", pNode->pName );
        return 0;
    }

    // realloc array if necessary
    pVm = Ntk_NodeReadFuncVm(pNode);
    assert ( pMan->nArrayAlloc >= Vm_VarMapReadValuesNum(pVm) );

    // collect the fanin BDDs and make sure the fanins are built
    i = 0;
    Ntk_NodeForEachFanin( pNode, pPin, pFanin )
    {
        if ( fUseNsc )
        { // if this is an ND node, use symbolic variable
            if ( Ntk_NodeReadFuncNonDet(pFanin) )
                pbFuncs = Ntk_NodeReadFuncGlobS(pFanin);
            else
            {
                pbFuncs = Ntk_NodeReadFuncGlobZ(pFanin);
                if ( pbFuncs[0] == NULL )
                {
                    pbFuncs = Ntk_NodeReadFuncGlob(pFanin);
                    assert( pbFuncs[0] );
                }
            }
        }
        else
        {
            pbFuncs = Ntk_NodeReadFuncGlobZ(pFanin);
            if ( pbFuncs[0] == NULL )
            {
                pbFuncs = Ntk_NodeReadFuncGlob(pFanin);
                assert( pbFuncs[0] );
            }
        }
        Dcmn_UtilsCopyFuncs( pMan->pbArray + i, pbFuncs, pFanin->nValues );
        i += pFanin->nValues;

        // check whether the input MDDs are non-deterministic
        if ( fNonDeterministic == 0 )
        {
            fNonDeterministic = Dcmn_UtilsCheckNDFuncs( pMan->ddGlo, pbFuncs, pFanin->nValues );
/*            
            if ( fNonDeterministic )
            {
                PRB( pMan->ddGlo, pbFuncs[0] );
                PRB( pMan->ddGlo, pbFuncs[1] );
            }
            assert( fNonDeterministic == 0 );
*/
        }
    }
//    fNonDeterministic = 0;


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
        Fnc_FunctionDeriveMddFromCvr( pMan->ddGlo, pVm, pCvrNew, pMan->pbArray, pMan->pbArray2 ); 
        Cvr_CoverFree( pCvrNew );
    }
    else
        Fnc_FunctionDeriveMddFromCvr( pMan->ddGlo, pVm, pCvr, pMan->pbArray, pMan->pbArray2 );
    pbFuncs = pMan->pbArray2;

    // set the global BDDs of the node
    Ntk_NodeWriteFuncGlobZ( pNode, pbFuncs );   
//printf( "setting %s\n", pNode->pName );

    // deref this global BDD
    Dcmn_UtilsDerefFuncs( pMan->ddGlo, pbFuncs, pNode->nValues );
    return 0;
}



////////////////////////////////////////////////////////////////////////
///                         END OF FILE                              ///
////////////////////////////////////////////////////////////////////////



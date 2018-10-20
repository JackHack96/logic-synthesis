/**CFile****************************************************************

  FileName    [dcmnUtils.c]

  PackageName [New don't care manager.]

  Synopsis    [Various utilities.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - July 13, 2003.]

  Revision    [$Id: fmnUtils.c,v 1.2 2003/11/18 18:55:13 alanmi Exp $]

***********************************************************************/

#include "fmnInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     EXPORTED FUNCTIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Returns the Vmx object that includes CI, interm, and CO nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vmx_VarMap_t * Dcmn_UtilsGetNetworkVmx( Ntk_Network_t * pNet )
{
    Vm_VarMap_t * pVm;
    Mvr_Relation_t * pMvr;
    Ntk_Node_t * pNode;
    int * pVarValues;
    int nVars, nOuts;

    pVarValues = ALLOC( int, Ntk_NetworkReadNodeTotalNum(pNet) );

    // add the CI nodes
    nVars = 0;
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        pVarValues[nVars] = pNode->nValues;
        Ntk_NodeWriteFuncNumber( pNode, nVars );
        Ntk_NodeWriteFuncNonDet( pNode, 0 );
        nVars++;
    }

    // check whether the nodes are non-deterministic, and mark them
    Ntk_NetworkForEachNode( pNet, pNode )
    {
        pMvr = Ntk_NodeGetFuncMvr( pNode );
        if ( Mvr_RelationIsND( pMvr ) ) 
        {
            pVarValues[nVars] = pNode->nValues;
            Ntk_NodeWriteFuncNumber( pNode, nVars );
            Ntk_NodeWriteFuncNonDet( pNode, 1 );
            nVars++;
        }
        else
            Ntk_NodeWriteFuncNonDet( pNode, 0 );

    }

    // add the CO nodes
    nOuts = Ntk_NetworkReadCoNum(pNet);
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        pVarValues[nVars] = pNode->nValues;
        Ntk_NodeWriteFuncNumber( pNode, nVars );
        Ntk_NodeWriteFuncNonDet( pNode, 0 );
        nVars++;
    }

    // create the maps
    pVm = Vm_VarMapLookup( Ntk_NetworkReadManVm(pNet), nVars - nOuts, nOuts, pVarValues );
    FREE( pVarValues );
    return Vmx_VarMapLookup( Ntk_NetworkReadManVmx(pNet), pVm, -1, NULL );
}

/**Function*************************************************************

  Synopsis    [Computes the number of nodes in the shared BDD of the POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dcmn_UtilsNumSharedBddNodes( Dcmn_Man_t * pMan )
{
	Ntk_Node_t * pNode;
	DdNode ** pbOutputs, ** pbFuncs;
	int Counter, RetValue, i;
    int nOutputsAlloc;

    // allocate storage for the PO BDDs
    nOutputsAlloc = 1000;
	pbOutputs = ALLOC( DdNode *, nOutputsAlloc );
	// start the DFS search from the POs of this slice
	Counter = 0;
    Ntk_NetworkForEachNode( pMan->pNet, pNode )
    {
        pbFuncs = Ntk_NodeReadFuncGlob(pNode);
        for ( i = 0; i < pNode->nValues; i++ )
        {
            pbOutputs[Counter] = pbFuncs[Counter];
            Counter++;
            if ( Counter == nOutputsAlloc )
            {
                pbOutputs = REALLOC( DdNode *, pbOutputs, nOutputsAlloc * 2 );
                nOutputsAlloc *= 2;
            }
        }
    }
//    assert( Counter == pMan->nExdcs );
	RetValue = Cudd_SharingSize( pbOutputs, Counter );
	FREE( pbOutputs );
	return RetValue;
}

/**Function*************************************************************

  Synopsis    [Removes intermediate variables.]

  Description [Quantifies the intermediate variables in the order
  that is determined by the simulation model. For SS, the order is 
  topological, for NSC the order is reverse topological.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Dcmn_FlexQuantifyFunction( Dcmn_Man_t * pMan, DdNode * bFunc ) 
{
    DdManager * dd = pMan->ddGlo;
    DdNode * bCof, * bProd, * bSum, * bRes, * bTemp, * bCube;
    DdNode ** pbGlos;
    int * pValuesFirst;
    int Num, nValues, v, i, iReal;

    pValuesFirst = Vm_VarMapReadValuesFirstArray( pMan->pVm );

    bRes = bFunc;   Cudd_Ref( bRes );
    for ( i = 0; i < pMan->nSpecials; i++ )
    {
        // consider nodes in the reverse topological order if this is NS or NSC
//        iReal = (pMan->pPars->fUseNsc | pMan->pPars->fUseNs)? pMan->nSpecials - 1 - i: i;
        iReal   = pMan->nSpecials - 1 - i;
        nValues = pMan->ppSpecials[iReal]->nValues;
        Num     = Ntk_NodeReadFuncNumber( pMan->ppSpecials[iReal] ); 

//       pbGlos  = Ntk_NodeReadFuncGlob( pMan->ppSpecials[iReal] );
        // this is the last bug fixed for NSC!!! August 5, 2003 3pm.

        pbGlos  = Ntk_NodeReadFuncGlobZ( pMan->ppSpecials[iReal] );
        if ( pbGlos[0] == NULL )
            pbGlos  = Ntk_NodeReadFuncGlob( pMan->ppSpecials[iReal] );


        bCube   = Vmx_VarMapCharCube( dd, pMan->pVmx, Num );  Cudd_Ref( bCube );


        // cofactor w.r.t this variable
        bSum = b0;  Cudd_Ref( bSum );
        for ( v = 0; v < nValues; v++ )
        {
            bCof = Cudd_bddAndAbstract( dd, bRes, pMan->pbCodes[pValuesFirst[Num]+v], bCube ); Cudd_Ref( bCof );
            bProd = Cudd_bddAnd( dd, bCof, pbGlos[v] );     Cudd_Ref( bProd );
            bSum = Cudd_bddOr( dd, bTemp = bSum, bProd );   Cudd_Ref( bSum );
            Cudd_RecursiveDeref( dd, bTemp );
            Cudd_RecursiveDeref( dd, bCof );
            Cudd_RecursiveDeref( dd, bProd );
        }
        Cudd_RecursiveDeref( dd, bCube );

//PRB( dd, bRes );

        // update the current function
        Cudd_RecursiveDeref( dd, bRes );
        bRes = bSum; // takes ref
    }
//PRB( dd, bRes );

    Cudd_Deref( bRes );
    return bRes;

//    return bFunc;
}

/**Function*************************************************************

  Synopsis    [Removes intermediate variables from the array of BDDs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dcmn_FlexQuantifyFunctionArray( Dcmn_Man_t * pMan, DdNode ** pbFuncs, int nFuncs )
{
    DdNode * bTemp;
    int v;

    // do not need quantification for SS
//    if ( !(pMan->pPars->fUseNs || pMan->pPars->fUseNsc) )
//        return;

//printf( "Quantifying vars.\n" );

    for ( v = 0; v < nFuncs; v++ )
    {
        pbFuncs[v] = Dcmn_FlexQuantifyFunction( pMan, bTemp = pbFuncs[v] ); Cudd_Ref( pbFuncs[v] );
        Cudd_RecursiveDeref( pMan->ddGlo, bTemp );
    }
}


/**Function*************************************************************

  Synopsis    [Derives the complete relation for the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Dcmn_UtilsNetworkDeriveRelation( Dcmn_Man_t * pMan, int fUseGloZ )
{
    DdManager * dd = pMan->ddGlo;
    Ntk_Node_t * pNode;
    DdNode * bRel, * bProd, * bSum, * bTemp;
    DdNode ** pbFunc;
    int * pValuesFirst;
    int v, Num;

    pValuesFirst = Vm_VarMapReadValuesFirstArray( pMan->pVm );

    bRel = b1;    Cudd_Ref( bRel );
    Ntk_NetworkForEachCo( pMan->pNet, pNode )
    {
        if ( fUseGloZ )
        {
            pbFunc = Ntk_NodeReadFuncGlobZ( pNode );
            if ( pbFunc[0] == NULL )
                pbFunc = Ntk_NodeReadFuncGlob( pNode );
        }
        else if ( pMan->pPars->TypeSpec == DCMN_TYPE_NS )
            pbFunc = Ntk_NodeReadFuncGlob( pNode ); // global functions with interm. vars
        else
            pbFunc = Ntk_NodeReadFuncGlobS( pNode ); // global functions w/o interm vars

        Num    = Ntk_NodeReadFuncNumber( pNode ); 
        // combine the cofactors
        bSum = b0;  Cudd_Ref( bSum );
        for ( v = 0; v < pNode->nValues; v++ )
        {
            bProd = Cudd_bddAnd( dd, pMan->pbCodes[pValuesFirst[Num]+v], pbFunc[v] );     Cudd_Ref( bProd );
            bSum  = Cudd_bddOr( dd, bTemp = bSum, bProd );   Cudd_Ref( bSum );
            Cudd_RecursiveDeref( dd, bTemp );
            Cudd_RecursiveDeref( dd, bProd );
        }

        bRel = Cudd_bddAnd( dd, bTemp = bRel, bSum );   Cudd_Ref( bRel );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bSum );
    }
//PRB( dd, pbFunc[0] );
//PRB( dd, pbFunc[1] );
//PRB( dd, bRel );
    // quantify the interm vars
    if ( pMan->pPars->TypeSpec == DCMN_TYPE_NS && !fUseGloZ )
        Dcmn_FlexQuantifyFunctionArray( pMan, &bRel, 1 );
//PRB( dd, bRel );

    Cudd_Deref( bRel );
    return bRel;
}

/**Function*************************************************************

  Synopsis    [Derives the complete relation for the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
double Dcmn_UtilsNetworkGetRelationRatio( Dcmn_Man_t * pMan, DdNode * bRel )
{
    double volFull, volTotal, volBasic;
    DdManager * dd = pMan->ddGlo;

    // count the number of minterms
    volFull  = Extra_Power2( dd->size );
    volTotal = Cudd_CountMinterm( dd, bRel, dd->size );
    volBasic = Extra_Power2( dd->size - Vmx_VarMapReadBitsOutNum(pMan->pVmx) );

    return 100.0 * (volTotal - volBasic) / (volFull - volBasic);
}

/**Function*************************************************************

  Synopsis    [Derives the complete relation for the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
double Dcmn_UtilsNetworkGetAverageNdRatio( Dcmn_Man_t * pMan )
{
    Ntk_Node_t * pNode;
    double TotalRatio = 0;
    Ntk_NetworkForEachNode( pMan->pNet, pNode )
        TotalRatio += Mvr_RelationFlexRatio( Ntk_NodeReadFuncMvr(pNode) );
    return TotalRatio / Ntk_NetworkReadNodeIntNum(pMan->pNet);
}

/**Function*************************************************************

  Synopsis    [Derives the complete relation for the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Dcmn_UtilsNetworkDeriveRelationForFanins( Dcmn_Man_t * pMan, Ntk_Node_t * ppNodes[], int nNodes )
{
    DdManager * dd = pMan->ddGlo;
    Ntk_Node_t * pNode;
    DdNode * bRel, * bProd, * bSum, * bTemp;
    DdNode ** pbFunc;
    int v, i;
    DdNode ** pbCodesLocal;
    int iValue;

    // get the codes of the local space
    pbCodesLocal = Vmx_VarMapEncodeMapUsingVars( dd, pMan->pbVars1, pMan->pVmxL );

    bRel = b1;    Cudd_Ref( bRel );
    iValue = 0;
    for ( i = 0; i < nNodes; i++ )
    {
        pNode = ppNodes[i];

//        pbFunc = Ntk_NodeReadFuncGlob( pNode );
        if ( pMan->pPars->TypeFlex != DCMN_TYPE_SS )
        {
            if ( Ntk_NodeReadFuncNonDet(pNode) )
                pbFunc = Ntk_NodeReadFuncGlobS(pNode);
            else
                pbFunc = Ntk_NodeReadFuncGlob(pNode);
        }
        else
            pbFunc = Ntk_NodeReadFuncGlob(pNode);


        // combine the cofactors
        bSum = b0;  Cudd_Ref( bSum );
        for ( v = 0; v < pNode->nValues; v++ )
        {
            bProd = Cudd_bddAnd( dd, pbCodesLocal[iValue+v], pbFunc[v] ); Cudd_Ref( bProd );
            bSum  = Cudd_bddOr( dd, bTemp = bSum, bProd );                Cudd_Ref( bSum );
            Cudd_RecursiveDeref( dd, bTemp );
            Cudd_RecursiveDeref( dd, bProd );
        }

        bRel = Cudd_bddAnd( dd, bTemp = bRel, bSum );   Cudd_Ref( bRel );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bSum );

        iValue += pNode->nValues;
    }

    // deref the codes of the local space
    Vmx_VarMapEncodeDeref( dd, pMan->pVmxL, pbCodesLocal );
    Cudd_Deref( bRel );
    return bRel;
}

/**Function*************************************************************

  Synopsis    [Computes the NSC image into the lobal space.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
/*
DdNode * Dcmn_UtilsNetworkImageNs( Dcmn_Man_t * pMan, DdNode * bCare, 
                                Ntk_Node_t * ppNodes[], int nNodes )
{
    DdNode * bRel, * bCubeIn, * bImage;
    DdManager * dd;
    dd = pMan->ddGlo;

    // get the relation
    bRel = Dcmn_UtilsNetworkDeriveRelationForFanins( pMan, ppNodes, nNodes );
    Cudd_Ref( bRel );
PRB( dd, bRel );

    // quantify the intermediate vars
    Dcmn_FlexQuantifyFunctionArray( pMan, &bRel, 1 );
PRB( dd, bRel );

    // get the cube of the input vars
    bCubeIn = Vmx_VarMapCharCubeInput( dd, pMan->pVmxG );  Cudd_Ref( bCubeIn );
PRB( dd, bCare );
PRB( dd, bCubeIn );

    // compute the monolythic image
	bImage = Cudd_bddAndAbstract( dd, bCare, bRel, bCubeIn ); Cudd_Ref( bImage );
PRB( dd, bImage );
	Cudd_RecursiveDeref( dd, bRel );
	Cudd_RecursiveDeref( dd, bCubeIn );
	Cudd_Deref( bImage );
	return bImage;
}
*/

/**Function*************************************************************

  Synopsis    [Computes the NSC image into the lobal space.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Dcmn_UtilsNetworkImageNs( Dcmn_Man_t * pMan, DdNode * bCare, 
                                Ntk_Node_t * ppNodes[], int nNodes )
{
    DdNode * bRel, * bCubeIn, * bProd, * bImage;
    DdManager * dd;
    dd = pMan->ddGlo;

    // get the relation
    bRel = Dcmn_UtilsNetworkDeriveRelationForFanins( pMan, ppNodes, nNodes );
    Cudd_Ref( bRel );
//PRB( dd, bRel );

    // get the product of the relation with the care set
    bProd = Cudd_bddAnd( dd, bRel, bCare );   Cudd_Ref( bProd );
	Cudd_RecursiveDeref( dd, bRel );

    // quantify the intermediate vars
    if ( pMan->pPars->TypeFlex != DCMN_TYPE_SS )
        Dcmn_FlexQuantifyFunctionArray( pMan, &bProd, 1 );
    // should not be used with SS

    // get the cube of the input vars
    bCubeIn = Vmx_VarMapCharCubeInput( dd, pMan->pVmxG );  Cudd_Ref( bCubeIn );
//PRB( dd, bCare );
//PRB( dd, bCubeIn ); 

    // compute the monolythic image
	bImage = Cudd_bddExistAbstract( dd, bProd, bCubeIn ); Cudd_Ref( bImage );
//PRB( dd, bImage );
	Cudd_RecursiveDeref( dd, bProd );
	Cudd_RecursiveDeref( dd, bCubeIn );

    bImage = Cudd_Not( bImage );

//PRB( pMan->ddGlo, bImage );

	Cudd_Deref( bImage );
	return bImage;
}

/**Function*************************************************************

  Synopsis    [Computes the NSC image into the lobal space.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Dcmn_UtilsNetworkImageNsUseProduct( Dcmn_Man_t * pMan, 
           Ntk_Node_t * ppNodes[], int nNodes, Ntk_Node_t * pNode )
{
    DdNode * bRel, * bCubeIn, * bProd, * bImage, * bTemp, * bFlex, * bImageProd;
    DdManager * dd = pMan->ddGlo;
    Ntk_Node_t * pNodeCo;//, * pNodeSpec;
    Ntk_Node_t * pTemp;

    // get the relation
    bRel = Dcmn_UtilsNetworkDeriveRelationForFanins( pMan, ppNodes, nNodes );
    Cudd_Ref( bRel );
//PRB( dd, bRel );

    bImageProd = b1;  Cudd_Ref( bImageProd );
    Ntk_NetworkForEachCo( pMan->pNet, pNodeCo )
    {
        // check if this output is involved (to be added later)
        // go the same with the TFO of pNode
//        Ntk_NetworkComputeNodeTfo( pMan->pNet, &pNode, 1, 10000, 1, 1 );
        // check if pNodeCo is among these nodes
//        Ntk_NetworkForEachNodeSpecial( pMan->pNet, pNodeSpec )
//            if ( pNodeSpec == pNodeCo )
//                continue;


        bFlex = Dcmn_ManagerComputeGlobalDcNodeUseProduct( pMan, pNode, pNodeCo ); Cudd_Ref( bFlex );

        // get the product of the relation with the care set
        bProd = Cudd_bddAnd( dd, bRel, Cudd_Not(bFlex) );   Cudd_Ref( bProd );
	    Cudd_RecursiveDeref( dd, bFlex );

        // quantify the intermediate vars
        if ( pMan->pPars->TypeFlex != DCMN_TYPE_SS )
            Dcmn_FlexQuantifyFunctionArray( pMan, &bProd, 1 );
    //PRB( dd, bRel );


        // deref GloZ BDDs
        Ntk_NetworkForEachNodeSpecialStart( pNode, pTemp )
            Ntk_NodeDerefFuncGlobZ( pTemp );


        // get the cube of the input vars
        bCubeIn = Vmx_VarMapCharCubeInput( dd, pMan->pVmxG );  Cudd_Ref( bCubeIn );
    //PRB( dd, bCare );
    //PRB( dd, bCubeIn );

        // compute the monolythic image
	    bImage = Cudd_bddExistAbstract( dd, bProd, bCubeIn ); Cudd_Ref( bImage );
        bImage = Cudd_Not( bImage );
    //PRB( dd, bImage );
	    Cudd_RecursiveDeref( dd, bProd );
	    Cudd_RecursiveDeref( dd, bCubeIn );


        // make the product of all images
        bImageProd = Cudd_bddAnd( dd, bTemp = bImageProd, bImage ); Cudd_Ref( bImageProd );
	    Cudd_RecursiveDeref( dd, bTemp );
	    Cudd_RecursiveDeref( dd, bImage );
    }
	Cudd_RecursiveDeref( dd, bRel );

//PRB( pMan->ddGlo, bImageProd );

	Cudd_Deref( bImageProd );
	return bImageProd;
}


/**Function*************************************************************

  Synopsis    [Copies the array of BDDs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dcmn_UtilsCopyFuncs( DdNode ** pbDest, DdNode ** pbSource, int nFuncs )
{
    int i;
    for ( i = 0; i < nFuncs; i++ )
    {
        assert( pbSource[i] != NULL );
        pbDest[i] = pbSource[i];  
    }
}

/**Function*************************************************************

  Synopsis    [Copies the array of BDDs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dcmn_UtilsRefFuncs( DdNode ** pbFuncs, int nFuncs )
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
void Dcmn_UtilsDerefFuncs( DdManager * dd, DdNode ** pbFuncs, int nFuncs )
{
    int i;
    for ( i = 0; i < nFuncs; i++ )
        Cudd_RecursiveDeref( dd, pbFuncs[i] );
}


/**Function*************************************************************

  Synopsis    [Check ND.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Dcmn_UtilsCheckNDFuncs( DdManager * dd, DdNode ** pbFuncs, int nValues )
{
    int i, k;
    for ( i = 0; i < nValues; i++ )
        for ( k = i+1; k < nValues; k++ )
            if ( !Cudd_bddLeq( dd, pbFuncs[i], Cudd_Not(pbFuncs[k]) ) ) // they intersect
                return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Check ND.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vmx_VarMap_t * Dcmn_UtilsCreateGlobalVmx( Dcmn_Man_t * p, Ntk_Node_t * pNode )
{
//    Ntk_Node_t * pNodeCi;
    Vm_VarMap_t * pVm;
    Vmx_VarMap_t * pVmx;
    int * pValues;
    int * pVarValues;
    int nInputs, i;
//    nInputs = Ntk_NetworkReadCiNum( p->pNet );
//    nInputs = Vm_VarMapReadVarsInNum(p->pVm);  /////// RECENT CHANGE
    nInputs = Vm_VarMapReadVarsNum(p->pVm);
    pVarValues = ALLOC( int, nInputs + 1 );
//    Ntk_NetworkForEachCi( p->pNet, pNodeCi )
//        pVarValues[i++] = pNodeCi->nValues;
    pValues = Vm_VarMapReadValuesArray(p->pVm);
    i = 0;
    for ( i = 0; i < nInputs; i++ )
        pVarValues[i] = pValues[i];
    pVarValues[i++] = pNode->nValues;

    pVm = Vm_VarMapLookup( Ntk_NodeReadManVm(pNode), nInputs, 1, pVarValues );
    pVmx = Vmx_VarMapLookup( Ntk_NodeReadManVmx(pNode), pVm, -1, NULL );
    FREE( pVarValues );
    return pVmx;


//    Vm_VarMap_t * pVm;
//    pVm = Vm_VarMapCreateOnePlus( p->pVm, pNode->nValues );
//    return Vmx_VarMapLookup( Ntk_NodeReadManVmx(pNode), pVm, -1, NULL );
}

/**Function*************************************************************

  Synopsis    [Check ND.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vmx_VarMap_t * Dcmn_UtilsCreateGlobalSetVmx( Dcmn_Man_t * p, Ntk_Node_t * pNode )
{
//    Ntk_Node_t * pNodeCi;
    Vm_VarMap_t * pVm;
    Vmx_VarMap_t * pVmx;
    int * pValues;
    int * pVarValues;
    int nInputs, i, k;
//    nInputs = Ntk_NetworkReadCiNum( p->pNet );
//    nInputs = Vm_VarMapReadVarsInNum(p->pVm);  /////// RECENT CHANGE
    nInputs = Vm_VarMapReadVarsNum(p->pVm);
    pVarValues = ALLOC( int, nInputs + pNode->nValues );
//    Ntk_NetworkForEachCi( p->pNet, pNodeCi )
//        pVarValues[i++] = pNodeCi->nValues;
    pValues = Vm_VarMapReadValuesArray(p->pVm);
    i = 0;
    for ( i = 0; i < nInputs; i++ )
        pVarValues[i] = pValues[i];
    for ( k = 0; k < pNode->nValues; k++ )
        pVarValues[i + k] = 2;

    pVm = Vm_VarMapLookup( Ntk_NodeReadManVm(pNode), nInputs, pNode->nValues, pVarValues );
    pVmx = Vmx_VarMapLookup( Ntk_NodeReadManVmx(pNode), pVm, -1, NULL );
    FREE( pVarValues );
    return pVmx;
}

/**Function*************************************************************

  Synopsis    [Check ND.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vmx_VarMap_t * Dcmn_UtilsCreateLocalVmx( Dcmn_Man_t * p, Ntk_Node_t * pNode, Ntk_Node_t ** ppFanins, int nFanins )
{
    Vm_VarMap_t * pVm;
    Vmx_VarMap_t * pVmx;
    int * pVarValues, i;
    pVarValues = ALLOC( int, nFanins + 1 );
    for ( i = 0; i < nFanins; i++ )
        pVarValues[i] = ppFanins[i]->nValues;
    pVarValues[i] = pNode->nValues;
    pVm  = Vm_VarMapLookup( Ntk_NodeReadManVm(pNode), nFanins, 1, pVarValues );
    pVmx = Vmx_VarMapLookup( Ntk_NodeReadManVmx(pNode), pVm, -1, NULL );
    FREE( pVarValues );
    return pVmx;
}

/**Function*************************************************************

  Synopsis    [This procedure transfers from the set output to normal outputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Dcmn_UtilsTransferFromSetOutputs( Dcmn_Man_t * pMan, DdNode * bFlexGlo )
{
    Vm_VarMap_t * pVm, * pVmInit;
    DdManager * dd;
    DdNode ** pbFuncs;
    DdNode ** pbCodes;
    DdNode * bResult;
    int nVarsIn, nValuesIn, nBits, i;
    int * pBitsFirst;

    assert( pMan->pVmxGS );

    // get the parameters
    dd         = pMan->ddGlo;
    pVmInit    = Vmx_VarMapReadVm( pMan->pVmxG );
    pVm        = Vmx_VarMapReadVm( pMan->pVmxGS );
    nVarsIn    = Vm_VarMapReadVarsInNum( pVm );
    nValuesIn  = Vm_VarMapReadValuesInNum( pVm );
    nBits      = Vmx_VarMapReadBitsNum( pMan->pVmxGS );
    pBitsFirst = Vmx_VarMapReadBitsFirst( pMan->pVmxGS );

    assert( nBits - pBitsFirst[nVarsIn] == Vm_VarMapReadValuesOutNum(pVmInit) );

    // set up the vector-compose problem
    pbFuncs = ALLOC( DdNode *, dd->size );
    for ( i = 0; i < dd->size; i++ )
        pbFuncs[i] = dd->vars[i];

    // encode the variables of the normal map (to get codes for the set vars)
    pbCodes  = Vmx_VarMapEncodeMap( dd, pMan->pVmxG );
    // modify by setting the composition relations
    for ( i = pBitsFirst[nVarsIn]; i < nBits; i++ )
    {
        pbFuncs[i] = pbCodes[nValuesIn + (i - pBitsFirst[nVarsIn])];
//printf( "i = %d\n", i );
//PRB( dd, pbCodes[nValuesIn + (i - pBitsFirst[nVarsIn])] );
    }

    // perform the composition
    bResult = Cudd_bddVectorCompose( dd, bFlexGlo, pbFuncs );  Cudd_Ref( bResult );

    // deref the codes
    Vmx_VarMapEncodeDeref( dd, pMan->pVmxG, pbCodes );
    FREE( pbFuncs );

    Cudd_Deref( bResult );
    return bResult;
}

////////////////////////////////////////////////////////////////////////
///                         END OF FILE                              ///
////////////////////////////////////////////////////////////////////////



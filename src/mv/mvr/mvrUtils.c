/**CFile****************************************************************

  FileName    [mvrUtils.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Functionality of the package to manipulate MV relations.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvrUtils.c,v 1.29 2004/10/06 03:33:47 alanmi Exp $]

***********************************************************************/

#include "mvrInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates MV relation.]

  Description [The first argument (nVars) is the number of input variables.
  The second argument (pValues) in the number of values for all variables, 
  the input and the output. Note that array pValues contains nVars + 1
  entries, where the last entry in the number of output values.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvr_Relation_t * Mvr_RelationCreate( Mvr_Manager_t * pMan, Vmx_VarMap_t * pVmx, DdNode * bRel )
{
    Mvr_Relation_t * pMvr;
    pMvr = Mvr_RelationAlloc();
    pMvr->pMan = pMan;
    pMvr->pVmx = pVmx;
    if ( bRel )
    {
        pMvr->bRel = bRel;   Cudd_Ref( bRel );
    }
    return pMvr;
}

/**Function*************************************************************

  Synopsis    [Creates constant MV relation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvr_Relation_t * Mvr_RelationCreateConstant( Mvr_Manager_t * pMan, Vmx_Manager_t * pManVmx, Vm_Manager_t * pManVm, int nValues, unsigned Pol )
{
    DdManager * dd = pMan->pDdLoc;
    Mvr_Relation_t * pMvr;
    Vm_VarMap_t * pVm;
    DdNode ** pbIsets;
    int i;
    // create the variable map
    pVm = Vm_VarMapLookup( pManVm, 0, 1, &nValues );
    // create the relation
    pMvr = Mvr_RelationAlloc();
    pMvr->pMan  = pMan;
    pMvr->pVmx  = Vmx_VarMapLookup( pManVmx, pVm, -1, NULL );
    // create the constant i-sets according to Polarity
    pbIsets = ALLOC( DdNode *, nValues );
    for ( i = 0; i < nValues; i++ )
    {
        if ( Pol & (1<<i) )
            pbIsets[i] = b1;
        else
            pbIsets[i] = b0;
        Cudd_Ref( pbIsets[i] );
    }
    // create the relation from cofactors
    Mvr_RelationCofactorsDeriveRelation( pMvr, pbIsets, 0, nValues );
    Mvr_RelationCofactorsDeref( pMvr, pbIsets, 0, nValues );
    FREE( pbIsets );
    return pMvr;
}

/**Function*************************************************************

  Synopsis    [Creates relation of the single-output node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvr_Relation_t * Mvr_RelationCreateOneInputOneOutput( Mvr_Manager_t * pMan, Vmx_Manager_t * pManVmx, Vm_Manager_t * pManVm, int nValuesIn, int nValuesOut, unsigned Pols[] )
{
    DdManager * dd = pMan->pDdLoc;
    Mvr_Relation_t * pMvr;
    Vm_VarMap_t * pVm;
    DdNode ** ppCodes;
    DdNode ** pbIsets;
    DdNode * bIset, * bTemp;
    int pValues[2];
    int i, k;

    // create the variable map
    pValues[0] = nValuesIn;
    pValues[1] = nValuesOut;
    pVm = Vm_VarMapLookup( pManVm, 1, 1, pValues );
    // create the relation

    pMvr = Mvr_RelationAlloc();
    pMvr->pMan = pMan;
    pMvr->pVmx = Vmx_VarMapLookup( pManVmx, pVm, -1, NULL );

    // get storage for i-sets
    pbIsets = ALLOC( DdNode *, nValuesOut );
    // get the input encoding cubes
    ppCodes = Vmx_VarMapEncodeVar( dd, pMvr->pVmx, 0 );
    // create the i-sets according to Polarities
    for ( i = 0; i < nValuesOut; i++ )
    {
        bIset = b0;   Cudd_Ref( bIset );
        for ( k = 0; k < nValuesIn; k++ )
            if ( Pols[i] & (1<<k) )
            {
                bIset = Cudd_bddOr( dd, bTemp = bIset, ppCodes[k] );  Cudd_Ref( bIset );
                Cudd_RecursiveDeref( dd, bTemp );
            }
        // set the i-set
        pbIsets[i] = bIset; // takes ref
    }
    // deref the cubes
    Vmx_VarMapEncodeDeref( dd, pMvr->pVmx, ppCodes );

    // create the relation from cofactors
    Mvr_RelationCofactorsDeriveRelation( pMvr, pbIsets, 1, nValuesOut );
    Mvr_RelationCofactorsDeref( pMvr, pbIsets, 1, nValuesOut );
    FREE( pbIsets );
    return pMvr;
}


/**Function*************************************************************

  Synopsis    [Creates relation of the decoder.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvr_Relation_t * Mvr_RelationCreateTwoInputBinary( Mvr_Manager_t * pMan, Vmx_Manager_t * pManVmx, Vm_Manager_t * pManVm, unsigned TruthTable )
{
    DdManager * dd = pMan->pDdLoc;
    Mvr_Relation_t * pMvr;
    Vm_VarMap_t * pVm;
    DdNode ** ppCodes;
    DdNode * bOnSet, * bMint, * bVar0, * bVar1, * bTemp;
    int pValues[3];
    int i;
    // create the variable map
    pValues[0] = 2;
    pValues[1] = 2;
    pValues[2] = 2;
    pVm        = Vm_VarMapLookup( pManVm, 2, 1, pValues ); 
    pMvr       = Mvr_RelationAlloc();
    pMvr->pMan = pMan;
    pMvr->pVmx = Vmx_VarMapLookup( pManVmx, pVm, -1, NULL ); 

    // get the input encoding cubes
    ppCodes = Vmx_VarMapEncodeMap( dd, pMvr->pVmx );

    // derive the on-set of the two-input gate
    bOnSet = b0; Cudd_Ref( bOnSet );
    for ( i = 0; i < 4; i++ )
        if ( TruthTable & (1 << i) )
        {
            bVar0 = Cudd_NotCond( ppCodes[1], (i & 1)==0 );
            bVar1 = Cudd_NotCond( ppCodes[3], (i & 2)==0 );
            bMint = Cudd_bddAnd( dd, bVar0, bVar1 );           Cudd_Ref( bMint );
            bOnSet = Cudd_bddOr( dd, bTemp = bOnSet, bMint );  Cudd_Ref( bOnSet );
            Cudd_RecursiveDeref( dd, bTemp );
            Cudd_RecursiveDeref( dd, bMint );
        }
    // derive the relation
    pMvr->bRel = Cudd_bddXnor( dd, bOnSet, ppCodes[5] );  Cudd_Ref( pMvr->bRel );
    Cudd_RecursiveDeref( dd, bOnSet );
    // deref the cubes
    Vmx_VarMapEncodeDeref( dd, pMvr->pVmx, ppCodes );
    return pMvr;
}

/**Function*************************************************************

  Synopsis    [Creates relation of the decoder.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvr_Relation_t * Mvr_RelationCreateSimpleBinary( Mvr_Manager_t * pMan, Vmx_Manager_t * pManVmx, Vm_Manager_t * pManVm, int pCompls[], int nInputs, int fGateOr )
{
    DdManager * dd = pMan->pDdLoc;
    Mvr_Relation_t * pMvr;
    DdNode ** ppCodes;
    DdNode * bOnSet, * bTemp;
    int i;
    // create the variable map
    pMvr       = Mvr_RelationAlloc();
    pMvr->pMan = pMan;
    pMvr->pVmx = Vmx_VarMapCreateBinary( pManVm, pManVmx, nInputs, 1 ); 

    // get the input encoding cubes
    ppCodes = Vmx_VarMapEncodeMap( dd, pMvr->pVmx );

    // derive the on-set of the n-input OR gate
    if ( fGateOr )
        bOnSet = b0; 
    else
        bOnSet = b1; 
    Cudd_Ref( bOnSet );
    for ( i = 0; i < nInputs; i++ )
    {
        if ( fGateOr )
            bOnSet = Cudd_bddOr( dd, bTemp = bOnSet, ppCodes[2*i+1-pCompls[i]] );  
        else
            bOnSet = Cudd_bddAnd( dd, bTemp = bOnSet, ppCodes[2*i+1-pCompls[i]] );  
        Cudd_Ref( bOnSet );
        Cudd_RecursiveDeref( dd, bTemp );
    }
    // derive the relation
    pMvr->bRel = Cudd_bddXnor( dd, bOnSet, ppCodes[2*nInputs+1] );  Cudd_Ref( pMvr->bRel );
    Cudd_RecursiveDeref( dd, bOnSet );
    // deref the cubes
    Vmx_VarMapEncodeDeref( dd, pMvr->pVmx, ppCodes );
    return pMvr;
}

/**Function*************************************************************

  Synopsis    [Creates relation of the decoder.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvr_Relation_t * Mvr_RelationCreateMuxBinary( Mvr_Manager_t * pMan, Vmx_Manager_t * pManVmx, Vm_Manager_t * pManVm )
{
    DdManager * dd = pMan->pDdLoc;
    Mvr_Relation_t * pMvr;
    DdNode * bOnSet;
    int pValues[4] = { 2, 2, 2, 2};
    // create the variable map
    pMvr       = Mvr_RelationAlloc();
    pMvr->pMan = pMan;
    pMvr->pVmx = Vmx_VarMapCreateSimple( pManVm, pManVmx, 3, 1, pValues ); 
    // derive the on-set of the MUX
    bOnSet = Cudd_bddIte( dd, dd->vars[0], dd->vars[1], dd->vars[2] ); Cudd_Ref( bOnSet );
    // derive the relation
    pMvr->bRel = Cudd_bddXnor( dd, bOnSet, dd->vars[3] );              Cudd_Ref( pMvr->bRel );
    Cudd_RecursiveDeref( dd, bOnSet );
    return pMvr;
}


/**Function*************************************************************

  Synopsis    [Detects constant-1 i-sets of the relation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Mvr_RelationGetConstant( Mvr_Relation_t * pMvr )
{
    DdManager * dd;
    DdNode **   pbCodes;
    unsigned    iPos;
    int         i, nValues;
    // get the manager
    dd = pMvr->pMan->pDdLoc;
    // get the number of output values
    nValues = pMvr->pVmx->pVm->nValuesOut;
    assert( nValues <= 32 );
    // get the cubes encoding output values
    pbCodes = Vmx_VarMapEncodeVar( dd, pMvr->pVmx, pMvr->pVmx->pVm->nVarsIn );
    // check the containment of output encoding cubes in the relation
    // if some cube is contained, it means that this value is constant-1
    iPos = 0;
    for ( i = 0; i < nValues; ++i )
        if ( Cudd_bddLeq( dd, pbCodes[i], pMvr->bRel ) )
            iPos |= (1<<i);
    // deref the codes
    Vmx_VarMapEncodeDeref( dd, pMvr->pVmx, pbCodes );
    return iPos;
}


/**Function*************************************************************

  Synopsis    [Creates relation of the decoder.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvr_Relation_t * Mvr_RelationCreateDecoder( Mvr_Manager_t * pMan, Vmx_Manager_t * pManVmx, Vm_Manager_t * pManVm, int nValues1, int nValues2 )
{
    DdManager * dd = pMan->pDdLoc;
    Mvr_Relation_t * pMvr;
    Vm_VarMap_t * pVm;
    DdNode ** ppCodes;
    DdNode ** pbIsets;
    DdNode * bValue1, * bValue2;
    int pValues[3];
    int i, k;
    // create the variable map
    pValues[0] = nValues1;
    pValues[1] = nValues2;
    pValues[2] = nValues1 * nValues2;
    pVm        = Vm_VarMapLookup( pManVm, 2, 1, pValues );
    pMvr       = Mvr_RelationAlloc();
    pMvr->pMan = pMan;
    pMvr->pVmx = Vmx_VarMapLookup( pManVmx, pVm, -1, NULL );

    // get storage for i-sets
    pbIsets = ALLOC( DdNode *, nValues1 * nValues2 );
    // get the input encoding cubes
    ppCodes = Vmx_VarMapEncodeMap( dd, pMvr->pVmx );
    // create the i-sets according to Polarities
    for ( i = 0; i < nValues1; i++ )
    for ( k = 0; k < nValues2; k++ )
    {
        bValue1 = ppCodes[ pVm->pValuesFirst[0] + i ];
        bValue2 = ppCodes[ pVm->pValuesFirst[1] + k ];
        pbIsets[nValues2*i+k] = Cudd_bddAnd( dd, bValue1, bValue2 );  
        Cudd_Ref( pbIsets[nValues2*i+k] );
    }
    // deref the cubes
    Vmx_VarMapEncodeDeref( dd, pMvr->pVmx, ppCodes );

    // create the relation from cofactors
    Mvr_RelationCofactorsDeriveRelation( pMvr, pbIsets, 2, nValues1 * nValues2 );
    Mvr_RelationCofactorsDeref( pMvr, pbIsets, 2, nValues1 * nValues2 );
    FREE( pbIsets );
    return pMvr;
}

/**Function*************************************************************

  Synopsis    [Creates the relation of the decoder node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvr_Relation_t * Mvr_RelationCreateDecoderGeneral( 
    Mvr_Manager_t * pMan, Vmx_Manager_t * pManVmx, Vm_Manager_t * pManVm, 
    int CodeWidth, int * pValueAssign, int nTotalValues, int nOutputValues )
{
    DdManager * dd = pMan->pDdLoc;
    Mvr_Relation_t * pMvr;
    DdNode ** ppCodes, ** pbIsets;
    DdNode * bMint, * bVar, * bTemp;
    Vmx_VarMap_t * pVmx;
    Vm_VarMap_t * pVm;
    int * pValues;
    int i, k, b;

    // create the var map
    pValues = ALLOC( int, CodeWidth + 1 );
    for ( i = 0; i < CodeWidth; i++ )
        pValues[i] = 2;
    pValues[CodeWidth] = nOutputValues;

    pVm = Vm_VarMapLookup( pManVm, CodeWidth, 1, pValues );
    pVmx = Vmx_VarMapLookup( pManVmx, pVm, -1, NULL ); 
    FREE( pValues );

    // create the relation
    pMvr       = Mvr_RelationAlloc();
    pMvr->pMan = pMan;
    pMvr->pVmx = pVmx;

    // get storage for i-sets
    pbIsets = ALLOC( DdNode *, nOutputValues );
    // initialize i-sets of the decoder to constant-0 BDDs
    for ( k = 0; k < nOutputValues; k++ )
    {
        pbIsets[k] = b0; Cudd_Ref( pbIsets[k] );
    }

    // get the input encoding cubes
    ppCodes = Vmx_VarMapEncodeMap( dd, pMvr->pVmx );
    // create the i-sets according to pValueAssign
    for ( i = 0; i < nTotalValues; i++ )
    {
        // build the BDD minterm representing code minterm i
        bMint = b1; Cudd_Ref( bMint );
        for ( b = 0; b < CodeWidth; b++ )
        {
            if ( i & ( 1 << b ) )
                bVar = ppCodes[2*b+1];
            else
                bVar = ppCodes[2*b];
            bMint = Cudd_bddAnd( dd, bTemp = bMint, bVar );    Cudd_Ref( bMint );
            Cudd_RecursiveDeref( dd, bTemp );
        }

        if ( pValueAssign[i] >= 0 )
        { // add the minterm to one i-set
            k = pValueAssign[i];
            assert( k < nOutputValues );
            pbIsets[k] = Cudd_bddOr( dd, bTemp = pbIsets[k], bMint ); Cudd_Ref( pbIsets[k] );
            Cudd_RecursiveDeref( dd, bTemp );
        }
        else
        { // add this minterm to all i-sets
            for ( k = 0; k < nOutputValues; k++ )
            {
                pbIsets[k] = Cudd_bddOr( dd, bTemp = pbIsets[k], bMint ); Cudd_Ref( pbIsets[k] );
                Cudd_RecursiveDeref( dd, bTemp );
            }
        }

        // deref this minterm
        Cudd_RecursiveDeref( dd, bMint );
    }
    // deref the cubes
    Vmx_VarMapEncodeDeref( dd, pMvr->pVmx, ppCodes );

    // create the relation from cofactors
    Mvr_RelationCofactorsDeriveRelation( pMvr, pbIsets, CodeWidth, nOutputValues );
    Mvr_RelationCofactorsDeref( pMvr, pbIsets, CodeWidth, nOutputValues );
    FREE( pbIsets );
    return pMvr;
}    

/**Function*************************************************************

  Synopsis    [Encodes the relation using the given encoding.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvr_RelationCreateEncoded( 
    Mvr_Manager_t * pMan, Vmx_Manager_t * pManVmx, Vm_Manager_t * pManVm, 
    int CodeWidth, int * pValueAssign, int nTotalValues, int nOutputValues,
    Mvr_Relation_t ** ppMvrsRes, Mvr_Relation_t * pMvrInit )
{
    DdManager * dd = pMan->pDdLoc;
    Vm_VarMap_t * pVm, * pVmInit;
    Vmx_VarMap_t * pVmx;
    DdNode * pbIsetsInit[32], * pbIsetsRes[5][2];
    DdNode * bTemp;
    int * pValues, * pTransMap;
    int i, v;

    // get the original var map
    pVmInit = pMvrInit->pVmx->pVm;

    // create the new var map with the same input values as the init map
    // except for the output, which is binary
    pValues = ALLOC( int, pVmInit->nVarsIn + 1 );
    for ( i = 0; i < pVmInit->nVarsIn; i++ )
        pValues[i] = pVmInit->pValues[i]; // the input values
    pValues[pVmInit->nVarsIn] = 2; // the output value

    pVm = Vm_VarMapLookup( pManVm, pVmInit->nVarsIn, 1, pValues );
    pVmx = Vmx_VarMapLookup( pManVmx, pVm, -1, NULL );
    FREE( pValues );


    // cofactor the original relation
    Mvr_RelationCofactorsDerive( pMvrInit, pbIsetsInit, pVmInit->nVarsIn, nOutputValues );

    // remap the cofactors to the new variable map
    pTransMap = ALLOC( int, pVmInit->nVarsIn + 1 );
    for ( i = 0; i < pVmInit->nVarsIn; i++ )
        pTransMap[i] = i;
    pTransMap[pVmInit->nVarsIn] = -1; // unused

    for ( i = 0; i < nOutputValues; i++ )
    {
        pbIsetsInit[i] = Mvr_RelationRemap( pMvrInit, bTemp = pbIsetsInit[i], 
            pMvrInit->pVmx, pVmx, pTransMap );
        Cudd_Ref( pbIsetsInit[i] );
        Cudd_RecursiveDeref( dd, bTemp );
    }
    FREE( pTransMap );

    // clean the cofactors of the resulting relations
    for ( i = 0; i < CodeWidth; i++ )
    {
        pbIsetsRes[i][0] = NULL;
        pbIsetsRes[i][1] = b0;   Cudd_Ref( b0 );
    }

    // add the original i-sets to the corresponding cofactors of the res relations
    for ( v = 0; v < nTotalValues; v++ )
    {
        // skip the unused code
        if ( pValueAssign[v] == -1 )
            continue;
        // assign this i-set to all the res relations that have 1 in the code
        for ( i = 0; i < CodeWidth; i++ )
            if ( v & (1 << i) )
            {
                pbIsetsRes[i][1] = Cudd_bddOr( dd, bTemp = pbIsetsRes[i][1], pbIsetsInit[pValueAssign[v]] ); 
                Cudd_Ref( pbIsetsRes[i][1] );
                Cudd_RecursiveDeref( dd, bTemp );
            }
    }

    // derive the negative cofactors by complementing the positive
    for ( i = 0; i < CodeWidth; i++ )
    {
        pbIsetsRes[i][0] = Cudd_Not( pbIsetsRes[i][1] );
        Cudd_Ref( pbIsetsRes[i][0] );
    }

    // dereference the original i-sets
    Mvr_RelationCofactorsDeref( pMvrInit, pbIsetsInit, pVmInit->nVarsIn, nOutputValues );

    // create the resulting relations
    for ( i = 0; i < CodeWidth; i++ )
    {
        ppMvrsRes[i]       = Mvr_RelationAlloc();
        ppMvrsRes[i]->pMan = pMan;
        ppMvrsRes[i]->pVmx = pVmx;
        // create the relation from cofactors
        Mvr_RelationCofactorsDeriveRelation( ppMvrsRes[i], pbIsetsRes[i], pVmInit->nVarsIn, 2 );
        Mvr_RelationCofactorsDeref( ppMvrsRes[i], pbIsetsRes[i], pVmInit->nVarsIn, 2 );
    }
}


/**Function*************************************************************

  Synopsis    [Creates relation of the decoder.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvr_Relation_t * Mvr_RelationCreateCollector( Mvr_Manager_t * pMan, Vmx_Manager_t * pManVmx, Vm_Manager_t * pManVm, int nValues, int DefValue )
{
    DdManager * dd = pMan->pDdLoc;
    Mvr_Relation_t * pMvr;
    Vm_VarMap_t * pVm;
    DdNode ** ppCodes;
    DdNode ** pbIsets;
    DdNode * bTemp;
    int * pValues;
    bool fDefIsUsed;
    int i, k;
    // create the variable map
    fDefIsUsed = (bool)(DefValue >= 0);
    pValues = ALLOC( int, nValues + 1 );
    for ( i = 0; i < nValues - fDefIsUsed; i++ )
        pValues[i] = 2;
    pValues[i] = nValues;
    pVm        = Vm_VarMapLookup( pManVm, nValues - fDefIsUsed, 1, pValues ); 
    pMvr       = Mvr_RelationAlloc();
    pMvr->pMan = pMan;
    pMvr->pVmx = Vmx_VarMapLookup( pManVmx, pVm, -1, NULL );
    FREE( pValues );

    // get storage for i-sets
    pbIsets = ALLOC( DdNode *, nValues );
    // get the input encoding cubes
    ppCodes = Vmx_VarMapEncodeMap( dd, pMvr->pVmx );

    // create the i-sets 
    k = 0;
    for ( i = 0; i < nValues; i++ )
        if ( i != DefValue )
        {
            pbIsets[i] = ppCodes[2*(k++) + 1];  Cudd_Ref( pbIsets[i] );
        }
        else
            pbIsets[i] = NULL;

    // get the default i-set
    if ( fDefIsUsed )
    {
        k = 0;
        assert( pbIsets[DefValue] == NULL );
        pbIsets[DefValue] = b1;  Cudd_Ref( b1 );
        for ( i = 0; i < nValues; i++ )
            if ( i != DefValue )
            {
                pbIsets[DefValue] = Cudd_bddAnd( dd, bTemp = pbIsets[DefValue], ppCodes[2*(k++)] ); 
                Cudd_Ref( pbIsets[DefValue] );
                Cudd_RecursiveDeref( dd, bTemp );
            }
    }
    // deref the cubes
    Vmx_VarMapEncodeDeref( dd, pMvr->pVmx, ppCodes );

    // create the relation from cofactors
    Mvr_RelationCofactorsDeriveRelation( pMvr, pbIsets, nValues - fDefIsUsed, nValues );
    Mvr_RelationCofactorsDeref( pMvr, pbIsets, nValues - fDefIsUsed, nValues );
    FREE( pbIsets );
    return pMvr;
}

/**Function*************************************************************

  Synopsis    [Relation make minimum base.]

  Description [Creates the minimum base relation.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvr_Relation_t * Mvr_RelationCreateMinimumBase( Mvr_Relation_t * pMvr, int * pSuppMv )
{
    Mvr_Relation_t * pMvrNew;
    int * pVarTrans;
    Vm_VarMap_t * pVmOld, * pVmNew;
    int nVmOld, nVmNew;
    int nSuppNew, i;

    pMvrNew = Mvr_RelationAlloc();
    pMvrNew->pMan   = pMvr->pMan;
    pMvrNew->nBddNodes = pMvr->nBddNodes;
    pMvrNew->pVmx   = Vmx_VarMapCreateReduced( pMvr->pVmx, pSuppMv );
//Vmx_VarMapPrint( pMvr->pVmx );
//Vmx_VarMapPrint( pMvrNew->pVmx );

    // get the parameters
    pVmOld    = pMvr->pVmx->pVm;
    pVmNew    = pMvrNew->pVmx->pVm;
    nVmOld    = pVmOld->nVarsIn + pVmOld->nVarsOut;
    nVmNew    = pVmNew->nVarsIn + pVmNew->nVarsOut;

    // create the variable transition map, which shows where each of the 
    // currently present MV vars goes in the reduced relation
    pVarTrans = Vm_VarMapGetStorageArray1( pVmOld );
    nSuppNew  = 0;
    for ( i = 0; i < nVmOld; i++ )
        if ( pSuppMv[i] )
            pVarTrans[i] = nSuppNew++;
        else
            pVarTrans[i] = -1;
    assert( nSuppNew == nVmNew );

    pMvrNew->bRel = Mvr_RelationRemap( pMvr, pMvr->bRel, pMvr->pVmx, pMvrNew->pVmx, pVarTrans ); 
    Cudd_Ref( pMvrNew->bRel );
    return pMvrNew;
}


/**Function*************************************************************

  Synopsis    [Derive the collapsed relation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvr_Relation_t * Mvr_RelationCreateCollapsed( Mvr_Relation_t * pMvrN, Mvr_Relation_t * pMvrF, 
        Vm_VarMap_t * pVmNew, int * pTransMapNInv, int * pTransMapF )
{
    Mvr_Relation_t * pMvrNew;
    Vm_VarMap_t * pVmN, * pVmF;
    Vmx_VarMap_t * pVmxN, * pVmxF;
    DdManager * dd = pMvrN->pMan->pDdLoc;
    DdNode ** pbCofsN, ** pbCofsF;
    DdNode * bRelCol, * bComp, * bTemp;
    DdNode * bRelF;
    int nValuesF;
    int i;

    // get the maps
    pVmN  = Mvr_RelationReadVm( pMvrN );
    pVmF  = Mvr_RelationReadVm( pMvrF );
    pVmxN = Mvr_RelationReadVmx( pMvrN );
    pVmxF = Mvr_RelationReadVmx( pMvrF );
    nValuesF = Vm_VarMapReadValuesOutput( pVmF );
    
    // create the new relation
    pMvrNew = Mvr_RelationAlloc();
    pMvrNew->pMan = pMvrN->pMan;

    // create the variable map of the relation after collapsing 
    // by extending the variable map for the node (pNode) in such a way
    // the remapping of the BDD of the relation is not necessary
//Vmx_VarMapPrint( pVmxN );
//Vmx_VarMapPrint( pVmxF );
    pMvrNew->pVmx = Vmx_VarMapCreateExpanded( pVmxN, pVmNew, pTransMapNInv );
//Vmx_VarMapPrint( pMvrNew->pVmx );

    // resize the manager if necessary
    if ( dd->size < pMvrNew->pVmx->nBits )
    {
        for ( i = dd->size; i < pMvrNew->pVmx->nBits + 10; i++ )
            Cudd_bddIthVar( dd, i );
        Cudd_zddVarsFromBddVars( dd, 2 );
    }

    // remap the relation of the fanin (pFanin) to match the new map
    bRelF = Mvr_RelationRemap( pMvrF, pMvrF->bRel,  pVmxF, pMvrNew->pVmx, pTransMapF ); Cudd_Ref( bRelF );

    // get the i-sets of the remapped relation of pFanin
    // set the relation of pFanin into the new relation, to compute the cofactors
    pbCofsF = ALLOC( DdNode *, nValuesF );
    pMvrNew->bRel = bRelF;
    Mvr_RelationCofactorsDerive( pMvrNew, pbCofsF, pTransMapF[pVmF->nVarsIn], nValuesF );
    Cudd_RecursiveDeref( dd, bRelF );
    pMvrNew->bRel = NULL;


    // get the i-set of pNode
    // set the relation of pNode into the new relation, to compute the cofactors
    pbCofsN = ALLOC( DdNode *, nValuesF );
    pMvrNew->bRel = pMvrN->bRel;
    Mvr_RelationCofactorsDerive( pMvrNew, pbCofsN, pTransMapF[pVmF->nVarsIn], nValuesF );
    pMvrNew->bRel = NULL;

    // create the resulting relation
    bRelCol = b0;    Cudd_Ref( bRelCol );
    for ( i = 0; i < nValuesF; i++ )
    {
        bComp = Cudd_bddAnd( dd, pbCofsN[i], pbCofsF[i] );  Cudd_Ref( bComp );
        bRelCol = Cudd_bddOr( dd, bTemp = bRelCol, bComp );   Cudd_Ref( bRelCol );
        Cudd_RecursiveDeref( dd, bComp );
        Cudd_RecursiveDeref( dd, bTemp );
    }

    // deref the intermediate cofactors
    Mvr_RelationCofactorsDeref( pMvrNew, pbCofsN, pTransMapF[pVmF->nVarsIn], nValuesF );
    FREE( pbCofsN );
    Mvr_RelationCofactorsDeref( pMvrNew, pbCofsF, pTransMapF[pVmF->nVarsIn], nValuesF );
    FREE( pbCofsF );

    // set the new relation
    pMvrNew->bRel = bRelCol;  // comes referenced
    return pMvrNew;
}


/**Function*************************************************************

  Synopsis    [Creates the relation with the permuted MV vars.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvr_Relation_t * Mvr_RelationCreatePermuted( Mvr_Relation_t * pMvr, int * pPermuteInv )
{
    Mvr_Relation_t * pMvrNew;
    pMvrNew = Mvr_RelationAlloc();
    // copy the relation
    pMvrNew->pMan   = pMvr->pMan;
    pMvrNew->nBddNodes = pMvr->nBddNodes;
    pMvrNew->bRel   = pMvr->bRel;      Cudd_Ref( pMvr->bRel );
    // get the new variable map
    pMvrNew->pVmx   = Vmx_VarMapCreatePermuted( pMvr->pVmx, pPermuteInv );
//Vmx_VarMapPrint( pMvr->pVmx );
//Vmx_VarMapPrint( pMvrNew->pVmx );
    return pMvrNew;
}


/**Function*************************************************************

  Synopsis    [Derives the power of the given relation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvr_Relation_t * Mvr_RelationCreatePower( Mvr_Relation_t * pMvr, int Degree )
{
    Mvr_Relation_t * pMvrRes;
    DdManager * dd = pMvr->pMan->pDdLoc;
    DdNode * bRelBase, * bRelShift, * bRelRes, * bTemp;
    int i;

    assert( Degree > 1 );

    // resize the manager if necessary
    if ( pMvr->pMan->pDdLoc->size < pMvr->pVmx->nBits * Degree )
    {
        for ( i = pMvr->pMan->pDdLoc->size; i < pMvr->pVmx->nBits * Degree + 10; i++ )
            Cudd_bddIthVar( dd, i );
        Cudd_zddVarsFromBddVars( dd, 2 );
    }

    // allocate the relation and create the power var maps
    pMvrRes = Mvr_RelationAlloc();
    pMvrRes->pMan = pMvr->pMan;
    pMvrRes->pVmx = Vmx_VarMapCreatePower( pMvr->pVmx, Degree );

    // create the new relation
    bRelBase = Extra_bddStretch( dd, pMvr->bRel, Degree );  Cudd_Ref( bRelBase );
    bRelRes  = bRelBase;   Cudd_Ref( bRelRes );
    for ( i = 1; i < Degree; i++ )
    {
        bRelShift = Extra_bddShift( dd, bRelBase, 0 );             Cudd_Ref( bRelShift );
        bRelRes   = Cudd_bddAnd( dd, bTemp = bRelRes, bRelShift ); Cudd_Ref( bRelRes );
        Cudd_RecursiveDeref( dd, bTemp );
        // update the base
        Cudd_RecursiveDeref( dd, bRelBase );
        bRelBase = bRelShift; // takes ref
    }
    Cudd_RecursiveDeref( dd, bRelBase );
    pMvrRes->bRel = bRelRes;  // takes ref
    return pMvrRes;
}

/**Function*************************************************************

  Synopsis    [Derives the relation of the local function from the global MDD.]

  Description [The parameters are: the global BDD manager (ddGlo), the local 
  manager (ddLoc), the ext var map manager (pManVmx), the CI variable map (pVm),
  the MDD of the node in the global manager (ddGlo). and the number of values.
  This function computes the new variable map (pVmNew), the new extended variable
  map (pVmxNew), remaps the BDDs into the local manager and finally derives the
  relation (the return value).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvr_Relation_t * Mvr_RelationCreateFromGlobal( DdManager * ddGlo, Mvr_Manager_t * pManMvr, 
        Vmx_Manager_t * pManVmx, Vm_VarMap_t * pVm, DdNode ** pbMdds, int nValues )
{
    DdManager * ddLoc = pManMvr->pDdLoc;
    Mvr_Relation_t * pMvr;
    DdNode ** pbMddsNew;
    Vm_VarMap_t * pVmNew;
    Vmx_VarMap_t * pVmxNew;
    int * pBitOrder;
    int nBitsOld, nBitsNew, i;

    // derive the new variable map
    pVmNew = Vm_VarMapCreateExtended( pVm, nValues );

    // get the number of bits in the new map
    nBitsOld = 0;
    for ( i = 0; i < pVm->nVarsIn; i++ )
        nBitsOld += Extra_Base2Log( pVm->pValues[i] );
    // get the ordering of bits in the extended variable map
    pBitOrder = ALLOC( int, ddGlo->size + nValues );
    for ( i = 0; i < nBitsOld; i++ )
        pBitOrder[i] = ddGlo->perm[i];
    // add the remaining bits for the last variable
    nBitsNew = nBitsOld + Extra_Base2Log( nValues );
    for ( i = nBitsOld; i < nBitsNew; i++ )
        pBitOrder[i] = i;
    // HERE THERE IS THE PROBLEM WITH THE MAPPING OF VARS IN THE dd MANAGER
    // IF THERE ARE MORE VARIABLES THAN nBits, IT WILL NOT WORK!!!


    // derive the new extended variable map
    pVmxNew = Vmx_VarMapLookup( pManVmx, pVmNew, nBitsNew, pBitOrder );
    FREE( pBitOrder );

    // extend the local manager if necessary
    if ( ddLoc->size < nBitsNew )
    {
        for ( i = ddLoc->size; i < nBitsNew + 10; i++ )
            Cudd_bddIthVar( ddLoc, i );
        Cudd_zddVarsFromBddVars( ddLoc, 2 );
    }

    // remap the i-sets into the local manager
    pbMddsNew = ALLOC( DdNode *, nValues );
    for ( i = 0; i < nValues; i++ )
    {
        pbMddsNew[i] = Extra_TransferPermute( ddGlo, ddLoc, pbMdds[i], ddGlo->perm );
        Cudd_Ref( pbMddsNew[i] );
    }

    // create the new relation
    pMvr = Mvr_RelationAlloc();
    pMvr->pMan = pManMvr;
    pMvr->pVmx = pVmxNew;

    // create the relation from i-sets
    Mvr_RelationCofactorsDeriveRelation( pMvr, pbMddsNew, pVmNew->nVarsIn, nValues );
    // deref the i-sets
    for ( i = 0; i < nValues; i++ )
        Cudd_RecursiveDeref( ddLoc, pbMddsNew[i] );
    FREE( pbMddsNew );

    return pMvr;
}


/**Function*************************************************************

  Synopsis    [Remaps BEMDD from the old map to the new map.]

  Description [Takes a BEMDD of any function that depends on the old
  variable map (pMapOld) (this function can be an i-set or a relation itself). 
  Assuming that this BEMDD is expressed using the old variable map (pMapOld), 
  this procedure remaps it to the new variable map (pMapNew). Additional 
  information for the remapping is given in the array pVarTrans. For each 
  MV variable in the old var map, this array contains the corresponding 
  MV variable in the new var map.  Array pVarTrans may contain entry -1,
  if this variable is not used in the function to be remapped. Obviously, 
  the number of values in the corresponding pairs of MV variables (one
  from the old map, another from the new map) should be the same.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Mvr_RelationRemap( Mvr_Relation_t * pMvr, DdNode * bFunc,
    Vmx_VarMap_t * pVmxOld, Vmx_VarMap_t * pVmxNew, int * pVarTrans )
{
    DdManager * dd = pMvr->pMan->pDdLoc;
    DdNode * bRes;
    Vm_VarMap_t * pVmOld, * pVmNew;
    int nVmOld, v, b, index;
    int * pPermute, i;

    // get the parameters
    pVmOld = pVmxOld->pVm;
    pVmNew = pVmxNew->pVm;
    nVmOld = pVmOld->nVarsIn + pVmOld->nVarsOut;

    // extend the local manager if necessary
    if ( dd->size < pVmxNew->nBits )
    {
        for ( i = dd->size; i < pVmxNew->nBits + 10; i++ )
            Cudd_bddIthVar( dd, i );
        Cudd_zddVarsFromBddVars( dd, 2 );
    }

    // set up the permutation array
    pPermute = ALLOC( int, dd->size );
    for ( v = 0; v < nVmOld; v++ )
        if ( pVarTrans[v] != -1 )
        {
            assert( pVmOld->pValues[v] == pVmNew->pValues[pVarTrans[v]] );
            for ( b = 0; b < pVmxOld->pBits[v]; b++ )
            {
                index = pVmxOld->pBitsOrder[ pVmxOld->pBitsFirst[v] + b ];
                assert( index >= 0 && index < pVmxOld->nBits );
                pPermute[index] = pVmxNew->pBitsOrder[ pVmxNew->pBitsFirst[pVarTrans[v]] + b ];
            }
        }
    // remap the function
//  bRes = Cudd_bddPermute( dd, bFunc, pPermute ); // returns non-referenced
    bRes = Extra_Permute( pMvr->pMan->pPerm, dd, bFunc, pPermute );  
    free( pPermute );
    return bRes;
}

/**Function*************************************************************

  Synopsis    [Forces the equality of the two MV variables in Mvr.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvr_RelationMakeVarsEqual( Mvr_Relation_t * pMvr, int iVar1, int iVar2 )
{
    DdManager * dd;
    DdNode ** pbCofs, ** pbCodes;
    DdNode * bRelCol, * bComp, * bTemp;
    int nValues, i;

    // get the DD manager
    dd = pMvr->pMan->pDdLoc;

    // get the number of values
    nValues = pMvr->pVmx->pVm->pValues[iVar1];
    assert( nValues == pMvr->pVmx->pVm->pValues[iVar2] );

    // cofactor the relation w.r.t. the first var
    pbCofs = ALLOC( DdNode *, nValues );
    Mvr_RelationCofactorsDerive( pMvr, pbCofs, iVar1, nValues );
    // get the codes w.r.t. the second var
    pbCodes = Vmx_VarMapEncodeVar( dd, pMvr->pVmx, iVar2 );

    // create the resulting relation
    bRelCol = b0;    Cudd_Ref( bRelCol );
    for ( i = 0; i < nValues; i++ )
    {
        bComp = Cudd_bddAnd( dd, pbCofs[i], pbCodes[i] );     Cudd_Ref( bComp );
        bRelCol = Cudd_bddOr( dd, bTemp = bRelCol, bComp );   Cudd_Ref( bRelCol );
        Cudd_RecursiveDeref( dd, bComp );
        Cudd_RecursiveDeref( dd, bTemp );
    }

    // deref the cofactors
    Mvr_RelationCofactorsDeref( pMvr, pbCofs, iVar1, nValues );
    FREE( pbCofs );
    // deref the codes
    Vmx_VarMapEncodeDeref( dd, pMvr->pVmx, pbCodes );

    // deref the old relation
    Cudd_RecursiveDeref( dd, pMvr->bRel );
    pMvr->nBddNodes = BDD_NODES_UNKNOWN;
    // set the new relation
    pMvr->bRel = bRelCol;  // comes referenced
}


/**Function*************************************************************

  Synopsis    [Reorders the BEMDD representing the relation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Mvr_RelationReorder( Mvr_Relation_t * pMvr )
{
    DdManager * dd;
    Vmx_VarMap_t * pVmx;
    Vmx_VarMap_t * pVmxNew;
    Vm_VarMap_t  * pVm;
    DdNode * bRelNew;
    int * pOrder, * pOrderInv, * pBitOrderNew;
    int i;

    if ( pMvr == NULL )
        return 0;

    // start the order array
    dd = pMvr->pMan->pDdLoc;
    pVmx = pMvr->pVmx;
    pVm = pVmx->pVm;
    pOrder = Vm_VarMapGetStorageArray1( pVm );
    for ( i = 0; i < pVmx->nBits; i++ )
        pOrder[i] = -1;

    // perform variable reordering
    bRelNew = Extra_Reorder( pMvr->pMan->pReo, dd, pMvr->bRel, pOrder );  Cudd_Ref( bRelNew );

    // invert the variable order
    pOrderInv = Vm_VarMapGetStoragePermute( pVm );
    for ( i = 0; i < pVmx->nBits; i++ )
        if ( pOrder[i] >= 0 )
            pOrderInv[ pOrder[i] ] = i;
        else
            pOrderInv[i] = i;

    // create the new bit order
    // start with the original order
    pBitOrderNew = Vm_VarMapGetStorageArray2( pVm );
    for ( i = 0; i < pVmx->nBits; i++ )
        if ( pOrderInv[i] >= 0 )
            pBitOrderNew[i] = pOrderInv[ pVmx->pBitsOrder[i] ];
        else
            pBitOrderNew[i] = pVmx->pBitsOrder[i];

    // create the new ext var map
    pVmxNew = Vmx_VarMapLookup( pVmx->pMan, pVm, pVmx->nBits, pBitOrderNew );
    if ( pVmxNew == pVmx )
    { // no change
        assert( bRelNew == pMvr->bRel );
        Cudd_RecursiveDeref( dd, bRelNew );
        return 0;
    }

    // update the variable map
    pMvr->pVmx = pVmxNew;
    // update the BDD
    Cudd_RecursiveDeref( dd, pMvr->bRel );
    pMvr->bRel = bRelNew; // takes ref
    pMvr->nBddNodes = BDD_NODES_UNKNOWN;
    return 1;
}


/**Function*************************************************************

  Synopsis    [Returns the number of binary variables in the support.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvr_RelationSupportBinarySize( Mvr_Relation_t * pMvr )
{
    int nSuppSize;
    // get the number of nodes and the support size of the relation
    pMvr->nBddNodes = Extra_DagSizeSuppSize( pMvr->bRel, &nSuppSize );
    assert( nSuppSize <= pMvr->pVmx->nBits );
    return nSuppSize;
}

/**Function*************************************************************

  Synopsis    [Check if the binary variables form a contiguous range.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Mvr_RelationSupportBinaryIsRange( Mvr_Relation_t * pMvr )
{
    int * pSupport;
    int i;

    pSupport = ALLOC( int, ddMax(pMvr->pMan->pDdLoc->size, pMvr->pMan->pDdLoc->sizeZ) );
    Extra_SupportArray( pMvr->pMan->pDdLoc, pMvr->bRel, pSupport );

    for ( i = 0; i < pMvr->pVmx->nBits; i++ )
        if ( pSupport[i] == 0 )
        {
            free( pSupport );
            return 0;
        }
    free( pSupport );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the relation's support is okay.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Mvr_RelationCheckSupport( Mvr_Relation_t * pMvr )
{
    int * pSupport;
    int i;

    pSupport = ALLOC( int, ddMax(pMvr->pMan->pDdLoc->size, pMvr->pMan->pDdLoc->sizeZ) );
    Extra_SupportArray( pMvr->pMan->pDdLoc, pMvr->bRel, pSupport );

    // makes sure the support does not depend on variables 
    // below the lowest allowed variable for the given relation
    for ( i = pMvr->pVmx->nBits; i < pMvr->pMan->pDdLoc->size; i++ )
        if ( pSupport[i] )
        {
            fprintf( stdout, "Relation's support is not correct.\n" );
            free( pSupport );
            return 0;
        }
    free( pSupport );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Computes the support of MV relation in terms of MV variables.]

  Description [Takes an MV relation and the array where the resulting 
  support is written. Entry 1 means var is in the support. Entry 0 means
  var is not in the support. This procedure returns the total number of 
  MV variables in the support. In many ways, this procedure is similar 
  to Extra_SupportArray, except that it considers MV variables.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvr_RelationSupport( Mvr_Relation_t * pMvr, int * pSuppMv )
{
    Vm_VarMap_t * pVm;
    Vmx_VarMap_t * pVmx;
    int * pSuppBin;
    int nSuppMv;
    int nVars;
    int v, b;

    // compute the binary support of the relation
    pSuppBin = ALLOC( int, ddMax(pMvr->pMan->pDdLoc->size, pMvr->pMan->pDdLoc->sizeZ) );
    Extra_SupportArray( pMvr->pMan->pDdLoc, pMvr->bRel, pSuppBin );

    // find MV variables that are not in the binary support
    pVm   = pMvr->pVmx->pVm;
    pVmx  = pMvr->pVmx;
    nVars = pVm->nVarsIn + pVm->nVarsOut;
    memset( pSuppMv, 0, sizeof(int) * nVars );

    // go through the input MV variables
    nSuppMv = 0;
    for ( v = 0; v < pVm->nVarsIn; v++ )
        for ( b = 0; b < pVmx->pBits[v]; b++ ) // go though every bit of this MV variable
            if ( pSuppBin[ pVmx->pBitsOrder[ pVmx->pBitsFirst[v] + b ] ] )
            {
                pSuppMv[v] = 1;
                nSuppMv++;
                break;
            }
    free( pSuppBin );
    // the output vars are always present
    for ( v = pVm->nVarsIn; v < nVars; v++ )
    {
        pSuppMv[v] = 1;
        nSuppMv++;
    }
    return nSuppMv;
}

/**Function*************************************************************

  Synopsis    [Computes the support of MV relation in terms of MV variables.]

  Description [Returns the cube composed of the binary variables selected
  in the following way. If some *input* MV variable is present in the current 
  support of the relation (bRel), the first binary variable used to encode
  this MV variable is included in the resulting cube. This function is useful 
  for image computation.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Mvr_RelationSupportCube( DdManager * dd, DdNode * bFuncs, Vmx_VarMap_t * pVmx )
{
    DdNode * bSupp, * bVar, * bTemp;
    Vm_VarMap_t * pVm;
    int * pSuppBin;
    int v, b;

    // compute the binary support of the relation
    pSuppBin = ALLOC( int, ddMax(dd->size, dd->sizeZ) );
    Extra_SupportArray( dd, bFuncs, pSuppBin );

    // find MV variables that are not in the binary support
    pVm   = pVmx->pVm;
    // start the MV support
    bSupp = b1;  Cudd_Ref( bSupp );
    // go through the input MV variables
    for ( v = 0; v < pVm->nVarsIn; v++ )
        for ( b = 0; b < pVmx->pBits[v]; b++ ) // go though every bit of this MV variable
            if ( pSuppBin[ pVmx->pBitsOrder[ pVmx->pBitsFirst[v] + b ] ] )
            {
                assert( pVmx->pBitsOrder[ pVmx->pBitsFirst[v] ] < dd->size );
                bVar  = dd->vars[ pVmx->pBitsOrder[ pVmx->pBitsFirst[v] ] ];
                bSupp = Cudd_bddAnd( dd, bTemp = bSupp, bVar );  Cudd_Ref( bSupp );
                Cudd_RecursiveDeref( dd, bTemp );
                break;
            }
    free( pSuppBin );
    Cudd_Deref( bSupp );
    return bSupp;
}

/**Function*************************************************************

  Synopsis    [Computes the support of MV relation in terms of MV variables.]

  Description [Returns the cube composed of the binary variables selected
  in the following way. If some *input* MV variable is present in the current 
  support of the relation (bRel), the first binary variable used to encode
  this MV variable is included in the resulting cube. This function is useful 
  for image computation.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Mvr_RelationSupportCubeUsingVars( DdManager * dd, DdNode * pbVars[], DdNode * bFuncs, Vmx_VarMap_t * pVmx )
{
    DdNode * bSupp, * bVar, * bTemp;
    Vm_VarMap_t * pVm;
    int * pSuppBin;
    int v, b;

    // compute the binary support of the relation
    pSuppBin = ALLOC( int, ddMax(dd->size, dd->sizeZ) );
    Extra_SupportArray( dd, bFuncs, pSuppBin );

    // find MV variables that are not in the binary support
    pVm   = pVmx->pVm;
    // start the MV support
    bSupp = b1;  Cudd_Ref( bSupp );
    // go through the input MV variables
    for ( v = 0; v < pVm->nVarsIn; v++ )
        for ( b = 0; b < pVmx->pBits[v]; b++ ) // go though every bit of this MV variable
            if ( pSuppBin[ pbVars[pVmx->pBitsOrder[ pVmx->pBitsFirst[v] + b ]]->index ] )
            {
                bVar  = pbVars[ pVmx->pBitsOrder[ pVmx->pBitsFirst[v] ] ];
                bSupp = Cudd_bddAnd( dd, bTemp = bSupp, bVar );  Cudd_Ref( bSupp );
                Cudd_RecursiveDeref( dd, bTemp );
                break;
            }
    free( pSuppBin );
    Cudd_Deref( bSupp );
    return bSupp;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvr_RelationAddDontCare( Mvr_Relation_t * pMvr, DdNode * bDc )
{
    DdNode * bTemp;
    pMvr->bRel = Cudd_bddOr( pMvr->pMan->pDdLoc, bTemp = pMvr->bRel, bDc );
    Cudd_Ref( pMvr->bRel );
    Cudd_RecursiveDeref( pMvr->pMan->pDdLoc, bTemp );
    pMvr->nBddNodes = BDD_NODES_UNKNOWN;
}

/**Function*************************************************************

  Synopsis    [Determinizes the relation.]

  Description [Returns 1, if determinization was applied. Returns 0
  if determinization was not necessary.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Mvr_RelationDeterminize( Mvr_Relation_t * pMvr )
{
    DdManager * dd = pMvr->pMan->pDdLoc;
    Vm_VarMap_t * pVm;
    DdNode ** pbIsets, * bSum, * bTemp;
    int nOutputValues, i, k;
    bool fNonDeterminstic;

    // get the MV var map of this relation
    pVm = pMvr->pVmx->pVm;
    // get the number of output values
    nOutputValues = Vm_VarMapReadValuesOutput( pVm );

    // allocate room for cofactors
    pbIsets = ALLOC( DdNode *, nOutputValues );
    // set the i-sets w.r.t the last variable
    Mvr_RelationCofactorsDerive( pMvr, pbIsets, pVm->nVarsIn, nOutputValues );

    // check the i-sets for overlapping
    fNonDeterminstic = 0;
    for ( i = 0; i < nOutputValues; i++ )
        for ( k = i+1; k < nOutputValues; k++ )
            if ( !Cudd_bddLeq( dd, pbIsets[i], Cudd_Not(pbIsets[k]) ) ) // they intersect
            {
                fNonDeterminstic = 1;
                break;
            }

    if ( fNonDeterminstic )
    { // apply greedy determinization
        // start the sum of all i-sets by adding the first i-set
        bSum = pbIsets[0]; Cudd_Ref( bSum );
        // go through other i-sets
        for ( i = 1; i < nOutputValues; i++ )
        {
            if ( !Cudd_bddLeq( dd, pbIsets[i], Cudd_Not(bSum) ) ) 
            { // i-set number i intersects with the sum
                // sharp this i-set with the sum of other i-sets
                pbIsets[i] = Cudd_bddAnd( dd, bTemp = pbIsets[i], Cudd_Not(bSum) ); Cudd_Ref( pbIsets[i] );
                Cudd_RecursiveDeref( dd, bTemp );
            }
            // add this i-set to the sum
            bSum = Cudd_bddOr( dd, bTemp = bSum, pbIsets[i] ); Cudd_Ref( bSum );
            Cudd_RecursiveDeref( dd, bTemp );
        }
        assert( bSum == b1 );
        // complete domain should be covered
        // otherwise, the relation was not well-defined (should never happen)
        // or some other bug crept in
        Cudd_RecursiveDeref( dd, bSum );
    }

    // derive the relation from i-sets
    Mvr_RelationCofactorsDeriveRelation( pMvr, pbIsets, pVm->nVarsIn, nOutputValues );
    Mvr_RelationCofactorsDeref( pMvr, pbIsets, pVm->nVarsIn, nOutputValues );
    FREE( pbIsets );

    // in any case, the relation became deterministic
    pMvr->NonDet = MVR_DETERM;
    return fNonDeterminstic;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



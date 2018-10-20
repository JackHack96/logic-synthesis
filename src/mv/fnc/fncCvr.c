/**CFile****************************************************************

  FileName    [fncCvrMvr.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Procedures to convert among the MV relation and MV SOP.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: fncCvr.c,v 1.10 2003/11/18 18:55:03 alanmi Exp $]

***********************************************************************/

#include "fncInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Mvc_Cover_t * Fnc_FunctionDeriveSopFromZdd( Mvc_Manager_t * pManMvc, DdManager * dd, DdNode * zIsop, Vmx_VarMap_t * pVmx, int nVarsUsed );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Derives the MV SOP from the relation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cvr_Cover_t * Fnc_FunctionDeriveCvrFromMvr( Mvc_Manager_t * pManMvc, Mvr_Relation_t * pMvr, int fUseDefault )
{
    DdManager * dd = pMvr->pMan->pDdLoc;
    Cvr_Cover_t * pCvr;
    Vm_VarMap_t * pVm;
    DdNode ** pbIsets;
    DdNode ** pzIsets;
    Mvc_Cover_t ** pCovers;
    Mvc_Cover_t * pMvc;
    int nCubesMax, nCubesCur, iCoverMax, i;
    int nOutputValues;
    Mvc_Data_t * pData;

        
    // get the MV var map of this relation
    pVm = pMvr->pVmx->pVm;
    // get the number of output values
    nOutputValues = Vm_VarMapReadValuesOutput( pVm );

    // if the cover has MV vars, perform distance-1 merge
    pData = NULL;
    if ( !Vm_VarMapIsBinaryInput( pVm ) )
    {
        pMvc = Mvc_CoverAlloc( pManMvc, pVm->nValuesIn );
        pData = Mvc_CoverDataAlloc( pVm, pMvc );
    }

    // allocate room for cofactors
    pbIsets = ALLOC( DdNode *, nOutputValues );
    // get the i-sets w.r.t the last variable
    Mvr_RelationCofactorsDerive( pMvr, pbIsets, pVm->nVarsIn, nOutputValues );

    // assign the default value if needed
    if ( !fUseDefault )
    {
        // create the covers
        pCovers = ALLOC( Mvc_Cover_t *, nOutputValues );
        for ( i = 0; i < nOutputValues; i++ )
        {
            pCovers[i] = Fnc_FunctionDeriveSopFromMdd( pManMvc, pMvr, pbIsets[i], pbIsets[i], pVm->nVarsIn );
            if ( pData )
                Mvc_CoverDist1Merge( pData, pCovers[i] );
        }
        // start the Cvr object
        pCvr = Cvr_CoverCreate( pVm, pCovers );
    }
    else
    {
        // create ZDDs for each i-set
        pzIsets = ALLOC( DdNode *, nOutputValues );
        if ( nOutputValues == 2 )
        {
            pzIsets[0] = NULL;
            pzIsets[1] = Extra_zddIsopCoverAlt( dd, pbIsets[1], pbIsets[1] );  
            Cudd_Ref( pzIsets[1] );
            iCoverMax = 0;
        }
        else
            iCoverMax = Fnc_FunctionDeriveZddFromMvr( dd, pbIsets, pzIsets, nOutputValues );

        // if the default value is not assigned, do it now
        if ( iCoverMax == -1 )
        { // find the largest i-set
            nCubesMax = -1;
            for ( i = 0; i < nOutputValues; i++ )
            {
                nCubesCur = Cudd_zddCount( dd, pzIsets[i] );
                if ( nCubesMax < nCubesCur )
                {
                    nCubesMax = nCubesCur;
                    iCoverMax = i;
                }
            }
            assert( nCubesMax >= 0 );
        }

        // make sure this i-set does not overlap with other i-sets
        for ( i = 0; i < nOutputValues; i++ )
            if ( i != iCoverMax )
            {
                if ( !Cudd_bddLeq( dd, pbIsets[iCoverMax], Cudd_Not(pbIsets[i]) ) )
                { // there is overlap -> we cannot use the default value
                    // compute the missing i-set 
                    if ( iCoverMax >= 0 )
                    {
                        if ( pzIsets[iCoverMax] == NULL )
                        {
                            pzIsets[iCoverMax] = Extra_zddIsopCoverAlt( dd, pbIsets[iCoverMax], pbIsets[iCoverMax] );  
                            Cudd_Ref( pzIsets[iCoverMax] );
                        }
                    }
                    iCoverMax = -1;
                    break;
                }
            }

        // create the covers
        pCovers = ALLOC( Mvc_Cover_t *, nOutputValues );
        for ( i = 0; i < nOutputValues; i++ )
            if ( i != iCoverMax )
            {
                pCovers[i] = Fnc_FunctionDeriveSopFromZdd( pManMvc, dd, pzIsets[i], pMvr->pVmx, pVm->nVarsIn );
                if ( pData )
                    Mvc_CoverDist1Merge( pData, pCovers[i] );
            }
            else
                pCovers[i] = NULL;
        // start the Cvr object
        pCvr = Cvr_CoverCreate( pVm, pCovers );

        // deref the ZDDs
        for ( i = 0; i < nOutputValues; i++ )
            if ( pzIsets[i] )
                Cudd_RecursiveDerefZdd( dd, pzIsets[i] );
        FREE( pzIsets );
    }

    // deallocate the MV data structure
    if ( pData )
    {
        Mvc_CoverDataFree( pData, pMvc );
        Mvc_CoverFree( pMvc );
        pData = NULL;
    }


    Mvr_RelationCofactorsDeref( pMvr, pbIsets, pVm->nVarsIn, nOutputValues );
    FREE( pbIsets );
    return pCvr;
}



/**Function*************************************************************

  Synopsis    [Returns the default i-set.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fnc_FunctionDeriveZddFromMvr( DdManager * dd, DdNode * pbIsets[], DdNode * pzIsets[], int nIsets )
{
    int nBTrackLimit, nIters;
    int Default, CounterZero;
    int i;

    // set the limit on the number of backtracks
    nBTrackLimit = 100;
    nIters = 0;
    while ( 1 )
    {
        nIters++;
        CounterZero = 0;
        Default = -1;
        for ( i = 0; i < nIsets; i++ )
        {
            pzIsets[i] = Extra_zddIsopCoverAltLimited( dd, pbIsets[i], pbIsets[i], nBTrackLimit );
            if ( pzIsets[i] )
            {
                Cudd_Ref( pzIsets[i] ); 
                continue;
            }
            // set the current zero cover (the default candidate)
            Default = i;
            // increment the counter of zero covers
            CounterZero++;
        }
        if ( CounterZero <= 1 )
            break;
        // deref the results
        for ( i = 0; i < nIsets; i++ )
            if ( pzIsets[i] )
            {
                Cudd_RecursiveDerefZdd( dd, pzIsets[i] );
                pzIsets[i] = NULL;
            }
        // update the backtrack limit
        if ( nIters % 4 )
            nBTrackLimit *= 2;
    }
//    printf( "%d ", nIters );
    return Default;
}


/**Function*************************************************************

  Synopsis    [Derives the MV SOP from the relation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cvr_Cover_t * Fnc_FunctionDeriveCvrFromMvr2( Mvc_Manager_t * pManMvc, Mvr_Relation_t * pMvr, int fUseDefault )
{
    Cvr_Cover_t * pCvr;
    Vm_VarMap_t * pVm;
    DdNode ** pbIsets;
    int * pnCubesInIsets;
    Mvc_Cover_t * * pCovers;
    int nCubesMax, iCoverMax, i;
    int nOutputValues;
    
    // get the MV var map of this relation
    pVm = pMvr->pVmx->pVm;
    // get the number of output values
    nOutputValues = Vm_VarMapReadValuesOutput( pVm );

    // allocate room for cofactors
    pbIsets = ALLOC( DdNode *, nOutputValues );
    // set the i-sets w.r.t the last variable
    Mvr_RelationCofactorsDerive( pMvr, pbIsets, pVm->nVarsIn, nOutputValues );

    // assign the default value if needed
    if ( !fUseDefault )
    {
        // create the covers
        pCovers = ALLOC( Mvc_Cover_t *, nOutputValues );
        for ( i = 0; i < nOutputValues; i++ )
            pCovers[i] = Fnc_FunctionDeriveSopFromMdd( pManMvc, pMvr, pbIsets[i], pbIsets[i], pVm->nVarsIn );
        // start the Cvr object
        pCvr = Cvr_CoverCreate( pVm, pCovers );
    }
    else
    {
        // without computing ZDDs of ISOPs for each i-set, find the number of cubes
        pnCubesInIsets = ALLOC( int, nOutputValues );
        for ( i = 0; i < nOutputValues; i++ )
            pnCubesInIsets[i] = Extra_zddIsopCubeNum( pMvr->pMan->pDdLoc, pbIsets[i], pbIsets[i] );   

        // find the largest i-set
        nCubesMax = -1;
        for ( i = 0; i < nOutputValues; i++ )
            if ( nCubesMax < pnCubesInIsets[i] )
            {
                nCubesMax = pnCubesInIsets[i];
                iCoverMax = i;
            }
        assert( nCubesMax >= 0 );

        // make sure this i-set does not overlap with other i-sets
        for ( i = 0; i < nOutputValues; i++ )
            if ( i != iCoverMax )
            {
                if ( !Cudd_bddLeq( pMvr->pMan->pDdLoc, pbIsets[iCoverMax], Cudd_Not(pbIsets[i]) ) )
                { // there is an overlap -> we cannot use the default value
                    iCoverMax = -1;
                    break;
                }
            }

        // create the covers
        pCovers = ALLOC( Mvc_Cover_t *, nOutputValues );
        for ( i = 0; i < nOutputValues; i++ )
            if ( i != iCoverMax )
                pCovers[i] = Fnc_FunctionDeriveSopFromMdd( pManMvc, pMvr, pbIsets[i], pbIsets[i], pVm->nVarsIn );
            else
                pCovers[i] = NULL;
        // start the Cvr object using these covers
        pCvr = Cvr_CoverCreate( pVm, pCovers );
        FREE( pnCubesInIsets );
    }

    Mvr_RelationCofactorsDeref( pMvr, pbIsets, pVm->nVarsIn, nOutputValues );
    FREE( pbIsets );
    return pCvr;
}


/**Function*************************************************************

  Synopsis    [Derives Mvc from an MDD representing one i-set of the relation.]

  Description [Takes the Mvc manager (pManMvc), the relation (pMvr), the 
  MDDs representing the on-set and the on-plu-dc sets of the cover, and the
  number of the first variables (nVarsUsed) of the map (pMvr->pVmx) that 
  should be included into the cover.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Cover_t * Fnc_FunctionDeriveSopFromMdd( Mvc_Manager_t * pManMvc, Mvr_Relation_t * pMvr, DdNode * bMddOn, DdNode * bMddOnDc, int nVarsUsed )
{
    DdManager * dd = pMvr->pMan->pDdLoc;
    DdNode * zIsopLog;
    Mvc_Cover_t * Cover;
    // compute the binary ISOP
    zIsopLog = Extra_zddIsopCoverAlt( dd, bMddOn, bMddOnDc );   Cudd_Ref( zIsopLog );  
    // derive the SOP representation
    Cover = Fnc_FunctionDeriveSopFromZdd( pManMvc, dd, zIsopLog, pMvr->pVmx, nVarsUsed );
    Cudd_RecursiveDerefZdd( dd, zIsopLog );
    // the resulting cover may be not SCC-free
    return Cover;
}

/**Function*************************************************************

  Synopsis    [Derives Mvc from an MDD.]

  Description [Takes the Mvc manager (pManMvc), the BDD manager storing
  the MDD (which manager should have ZDD variables allocated), the 
  MDDs representing the on-set and the on-plu-dc sets of the cover, and the
  number of the first variables (nVarsUsed) of the map (pVmx) that 
  should be included into the cover.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Cover_t * Fnc_FunctionDeriveSopFromMddSpecial( Mvc_Manager_t * pManMvc, DdManager * dd, DdNode * bMddOn, DdNode * bMddOnDc, Vmx_VarMap_t * pVmx, int nVarsUsed )
{
    DdNode * zIsopLog;
    Mvc_Cover_t * Cover;
    // compute the binary ISOP
    zIsopLog = Extra_zddIsopCoverAlt( dd, bMddOn, bMddOnDc );   Cudd_Ref( zIsopLog );  
    // derive the SOP representation
    Cover = Fnc_FunctionDeriveSopFromZdd( pManMvc, dd, zIsopLog, pVmx, nVarsUsed );
    Cudd_RecursiveDerefZdd( dd, zIsopLog );
    // the resulting cover may be not SCC-free
    return Cover;
}

/**Function*************************************************************

  Synopsis    [Derives a single Mvc from one i-set of the relation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Cover_t * Fnc_FunctionDeriveSopFromMddLimited( Mvc_Manager_t * pManMvc, Mvr_Relation_t * pMvr, DdNode * bMddOn, DdNode * bMddOnDc, int nVarsUsed )
{
    DdManager * dd = pMvr->pMan->pDdLoc;
    DdNode * zIsopLog;
    Mvc_Cover_t * Cover;
    // compute the binary ISOP
    zIsopLog = Extra_zddIsopCoverAltLimited( dd, bMddOn, bMddOnDc, 1000 );   
    if ( zIsopLog == NULL )
        return NULL;
    Cudd_Ref( zIsopLog );  
    // derive the SOP representation
    Cover = Fnc_FunctionDeriveSopFromZdd( pManMvc, dd, zIsopLog, pMvr->pVmx, nVarsUsed );
    Cudd_RecursiveDerefZdd( dd, zIsopLog );
    // the resulting cover may be not SCC-free
    return Cover;
}

/**Function*************************************************************

  Synopsis    [Derives a single Mvc from one i-set of the relation using Espresso.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Cover_t * Fnc_FunctionDeriveSopFromMddEspresso( Mvc_Manager_t * pManMvc, Mvr_Relation_t * pMvr, DdNode * bOn, DdNode * bOnDc, int nVarsUsed )
{
    DdManager * dd = pMvr->pMan->pDdLoc;
    Vm_VarMap_t * pVm = pMvr->pVmx->pVm;
    Mvc_Cover_t * pMvcOn, * pMvcDc;
    Mvc_Cover_t * pCover;
    DdNode * bDc, * zCover;

    // get the on-set as an ISOP
    zCover = Extra_zddIsopCoverAltLimited( dd, bOn, bOn, 1000 );
    if ( zCover == NULL )
        return NULL;
    Cudd_Ref( zCover );

    // cube out if the cover is too large
    if ( Cudd_zddCount(dd, zCover) > 500 )
    {
        Cudd_RecursiveDerefZdd( dd, zCover );
        return NULL;
    }
    pMvcOn = Fnc_FunctionDeriveSopFromZdd( pManMvc, dd, zCover, pMvr->pVmx, nVarsUsed );
    Cudd_RecursiveDerefZdd( dd, zCover );

    // get the dc-set as an ISOP
    bDc = Cudd_bddAnd( dd, bOnDc, Cudd_Not(bOn) );  Cudd_Ref( bDc );
    zCover = Extra_zddIsopCoverAltLimited( dd, bDc, bDc, 1000 );
    if ( zCover == NULL )
    {
        Cudd_RecursiveDeref( dd, bDc );
        Mvc_CoverFree( pMvcOn );
        return NULL;
    }
    Cudd_Ref( zCover );
    Cudd_RecursiveDeref( dd, bDc );

    // cube out if the cover is too large
    if ( Cudd_zddCount(dd, zCover) > 500 )
    {
        Cudd_RecursiveDerefZdd( dd, zCover );
        Mvc_CoverFree( pMvcOn );
        return NULL;
    }
    pMvcDc = Fnc_FunctionDeriveSopFromZdd( pManMvc, dd, zCover, pMvr->pVmx, nVarsUsed );
    Cudd_RecursiveDerefZdd( dd, zCover );

    // call Espresso
//    pCover = Cvr_IsetEspresso( pVm, pMvcOn, pMvcDc, 2, 1, 0 ); // SNOCOMP
    pCover = Cvr_IsetEspresso( pVm, pMvcOn, pMvcDc, 0, 1, 0 ); // ESPRESSO

    // remove temporary covers
    Mvc_CoverFree( pMvcOn );
    Mvc_CoverFree( pMvcDc );
    return pCover;
}

/**Function*************************************************************

  Synopsis    [Derive MV ISOP in pos notation using the MDD and the variable map.]

  Description [Takes the variable map (pMap) and the MDD represented using
  this variable map (bMdd). Computes the binary ISOP for the binary encoded
  MDD and then translates it into the MV SOP and represents it using ZDD.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Fnc_FunctionDeriveZddFromMdd( Mvr_Relation_t * pMvr, DdNode * bMddOn, DdNode * bMddOnDc )
{
    Vmx_VarMap_t * pVmx = pMvr->pVmx;
    Vm_VarMap_t * pVm = pVmx->pVm;
    DdManager * dd = pMvr->pMan->pDdLoc;
    DdNode * zIsopLog, * zIsopPos, * zCubeLog, * zCubePos, * zTemp;
    int *  pZddVarsLog = pMvr->pVmx->pMan->pArray;  // logarithmic encoding goes here
    int *  pZddVarsPos = pMvr->pVmx->pVm->pMan->pArray1;   // positional encoding goes here
    int nValuesTotal, i;

    // get the parameters
    nValuesTotal = Vm_VarMapReadValuesNum( pVm );

    // compute the binary ISOP
    zIsopLog = Extra_zddIsopCover( dd, bMddOn, bMddOnDc );   Cudd_Ref( zIsopLog );  
    // start the cover in positional notation
    zIsopPos = z0;  Cudd_Ref( zIsopPos );

    // translate the ISOP from the logarithmic into the positional notation
    while ( zIsopLog != z0 )
    {
        // get a cube
        zCubeLog = Extra_zddSelectOneCube( dd, zIsopLog );         Cudd_Ref( zCubeLog );
        // subtract the cube from the cover
        zIsopLog = Cudd_zddDiff( dd, zTemp = zIsopLog, zCubeLog ); Cudd_Ref( zIsopLog );
        Cudd_RecursiveDerefZdd( dd, zTemp );

        // translate the cube into the array
        for ( i = 0; i < pVmx->nBits; i++ )
            pZddVarsLog[i] = 2;
        for ( zTemp = zCubeLog; zTemp != z1; zTemp = cuddT(zTemp) )
        {
            assert( zTemp->index < 2 * pVmx->nBits );
            if ( zTemp->index % 2 == 0 )
                pZddVarsLog[ zTemp->index / 2 ] = 1;
            else
                pZddVarsLog[ zTemp->index / 2 ] = 0;
        }
        Cudd_RecursiveDerefZdd( dd, zCubeLog );

        // convert logarithmic notation to positional notation
        Vmx_VarMapDecode( pVmx, pZddVarsLog, pZddVarsPos );

        // create the cube in the positional notation and add it to the cover
        zCubePos = Extra_zddCombination( dd, pZddVarsPos, nValuesTotal ); Cudd_Ref( zCubePos );
        zIsopPos = Cudd_zddUnion( dd, zTemp = zIsopPos, zCubePos );            Cudd_Ref( zIsopPos );
        Cudd_RecursiveDerefZdd( dd, zTemp );
        Cudd_RecursiveDerefZdd( dd, zCubePos );
    }
    Cudd_RecursiveDerefZdd( dd, zIsopLog );

    // ensure single-cube containment
    zIsopPos = Extra_zddMaximal( dd, zTemp = zIsopPos ); Cudd_Ref( zIsopPos );
    Cudd_RecursiveDerefZdd( dd, zTemp );

    Cudd_Deref( zIsopPos );
    return zIsopPos;
}

/**Function*************************************************************

  Synopsis    [Derives Mvc from an MDD representing one i-set of the relation.]

  Description [Takes the Mvc manager (pManMvc), the BDD manager with ZDD variables
  allocated (dd), the ZDD representing the cover, the extended variable map (pVmx)
  which characterizes the variable mapping in the cover, and the number of first
  variables in the map (nVarsUsed) that should be included into the cover.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Cover_t * Fnc_FunctionDeriveSopFromZdd( Mvc_Manager_t * pManMvc, DdManager * dd, DdNode * zIsop, Vmx_VarMap_t * pVmx, int nVarsUsed )
{
    DdNode * zIsopLog, * zCubeLog, * zTemp;
    Vm_VarMap_t * pVm = pVmx->pVm;
    int *  pZddVarsLog = pVmx->pMan->pArray;  // logarithmic encoding goes here
    int *  pZddVarsPos = pVm->pMan->pArray1;   // positional encoding goes here
    Mvc_Cover_t * pCover;
    Mvc_Cube_t * pCube;
    int nCubeBits, i;

    // start the cover in positional notation
    nCubeBits = pVm->pValuesFirst[nVarsUsed];
    pCover    = Mvc_CoverAlloc( pManMvc, nCubeBits );

    // translate the binary ISOP from the logarithmic into the positional notation
    zIsopLog = zIsop;      Cudd_Ref( zIsopLog );
    while ( zIsopLog != z0 )
    {
        // get a cube
        zCubeLog = Extra_zddSelectOneCube( dd, zIsopLog );         Cudd_Ref( zCubeLog );
        // subtract the cube from the cover
        zIsopLog = Cudd_zddDiff( dd, zTemp = zIsopLog, zCubeLog ); Cudd_Ref( zIsopLog );
        Cudd_RecursiveDerefZdd( dd, zTemp );

        // translate the cube into the array
//        for ( i = 0; i < pVmx->nBits; i++ )
        for ( i = 0; i < dd->size; i++ )
            pZddVarsLog[i] = 2;
        for ( zTemp = zCubeLog; zTemp != z1; zTemp = cuddT(zTemp) )
        {
//            assert( zTemp->index < 2 * pVmx->nBits );
            // disabled this very useful assert to be able to work with automata
            // which are not mapped into the topmost variables of the BDD manager

            if ( zTemp->index % 2 == 0 )
                pZddVarsLog[ zTemp->index / 2 ] = 1;
            else
                pZddVarsLog[ zTemp->index / 2 ] = 0;
        }
        Cudd_RecursiveDerefZdd( dd, zCubeLog );

        // convert logarithmic notation to positional notation
        Vmx_VarMapDecode( pVmx, pZddVarsLog, pZddVarsPos );

        // create the cube in the positional notation and add it to the cover
        pCube = Mvc_CubeAlloc( pCover );
        Mvc_CubeBitFill( pCube );
        for ( i = 0; i < nCubeBits; i++ )
            if ( pZddVarsPos[i] == 0 )
                Mvc_CubeBitRemove( pCube, i );
        // add the cube to the cover
        Mvc_CoverAddCubeTail( pCover, pCube );
    }
    Cudd_RecursiveDerefZdd( dd, zIsopLog );
    // make sure the cover is SCC-free
    Mvc_CoverContain( pCover );
    return pCover;
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



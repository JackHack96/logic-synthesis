/**CFile****************************************************************

  FileName    [fncMvr.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Procedures to derive Mvr.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: fncMvr.c,v 1.8 2003/11/18 18:55:04 alanmi Exp $]

***********************************************************************/

#include "fncInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Derives MV relation from MV SOP.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvr_Relation_t * Fnc_FunctionDeriveMvrFromCvr( Mvr_Manager_t * pManMvr, Vmx_Manager_t * pManVmx, Cvr_Cover_t * pCvr )
{
    DdManager * dd = pManMvr->pDdLoc;
    Vm_VarMap_t * pVm;
    Vmx_VarMap_t * pVmx;
    Mvr_Relation_t * pMvr;
    DdNode ** pbIsets;
    DdNode ** pbCodes;
    int nOutputValues;

    // get the variable map
    pVm = Cvr_CoverReadVm( pCvr );
    // get the number of output values
    nOutputValues = Vm_VarMapReadValuesOutput( pVm );
    // create the extended map
    pVmx = Vmx_VarMapLookup( pManVmx, pVm, -1, NULL );
    // create the relation
    pMvr = Mvr_RelationCreate( pManMvr, pVmx, NULL );

    // get the codes of MV vars in terms of the bin vars
    pbCodes = Vmx_VarMapEncodeMap( dd, pMvr->pVmx );
    // allocate room for i-sets
    pbIsets = ALLOC( DdNode *, nOutputValues );
    // derive the i-sets
    if ( Fnc_FunctionDeriveMddFromCvr( dd, pMvr->pVmx->pVm, pCvr, pbCodes, pbIsets ) )
        pMvr->fMark = 1;
    // deref the value codes
    Vmx_VarMapEncodeDeref( dd, pMvr->pVmx, pbCodes );

    // create the relation from i-sets
    Mvr_RelationCofactorsDeriveRelation( pMvr, pbIsets, pVm->nVarsIn, nOutputValues );
    Mvr_RelationCofactorsDeref( pMvr, pbIsets, pVm->nVarsIn, nOutputValues );
    FREE( pbIsets );
    return pMvr;
}

/**Function*************************************************************

  Synopsis    [Derives the i-sets from MV SOP.]

  Description [Returns 1 if the i-sets are found to be not well-defined.
  In this case, the non-defined area is assumed to be a don't-care and 
  added to all i-sets. If the relation is well-defined, returns 0.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fnc_FunctionDeriveMddFromCvr( DdManager * dd, Vm_VarMap_t * pVm, Cvr_Cover_t * pCvr, DdNode ** pbCodes, DdNode ** pbResults )
{
    Mvc_Cover_t * * pCovers;
    DdNode * bSum, * bTemp;
    int nOutputValues, DefValue, i;
    int RetValue;

    // get the covers
    pCovers = Cvr_CoverReadIsets( pCvr );
    // get the number of output values
    nOutputValues = Vm_VarMapReadValuesOutput( pVm );
    // derive the i-sets
    DefValue = -1;
    for ( i = 0; i < nOutputValues; i++ )
        if ( pCovers[i] )
        {
            pbResults[i] = Fnc_FunctionDeriveMddFromSop( dd, pVm, pCovers[i], pbCodes );
            Cudd_Ref( pbResults[i] );
        }
        else
        {
            pbResults[i] = NULL;
            DefValue = i;
        }
    // derive the default i-set
    RetValue = 0;
    if ( DefValue >= 0 )
    {
        assert( pbResults[DefValue] == NULL );
        bSum = b0;  Cudd_Ref( bSum );
        for ( i = 0; i < nOutputValues; i++ )
            if ( i != DefValue )
            {
                bSum = Cudd_bddOr( dd, bTemp = bSum, pbResults[i] ); Cudd_Ref( bSum );
                Cudd_RecursiveDeref( dd, bTemp );
            }
        pbResults[DefValue] = Cudd_Not( bSum );  // takes ref
    }
    else
    {
        // the default i-set is not given
        // here we test the relation for being non-well defined
        bSum = b0;  Cudd_Ref( bSum );
        for ( i = 0; i < nOutputValues; i++ )
        {
            bSum = Cudd_bddOr( dd, bTemp = bSum, pbResults[i] ); Cudd_Ref( bSum );
            Cudd_RecursiveDeref( dd, bTemp );
        }
        if ( bSum != b1 )
        {
            // the relation is not well-defined
            // add the non-defined area to the don't-care
            for ( i = 0; i < nOutputValues; i++ )
            {
                pbResults[i] = Cudd_bddOr( dd, bTemp = pbResults[i], Cudd_Not(bSum) ); 
                Cudd_Ref( pbResults[i] );
                Cudd_RecursiveDeref( dd, bTemp );
            }
            RetValue = 1;
        }
        Cudd_RecursiveDeref( dd, bSum );
    }
    return RetValue;
}


/**Function*************************************************************

  Synopsis    [Derives one i-set from MV-input binary-output SOP.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Fnc_FunctionDeriveMddFromSop( DdManager * dd, Vm_VarMap_t * pVm, Mvc_Cover_t * Cover, DdNode ** pInputMdds )
{
    DdNode * bRes, * bCube, * bValues, * bTemp;
    Mvc_Cube_t * pCube;
    int fDontCare, iValues, i, v;

    if ( Cover == NULL )
        return NULL;

    // start the MDD
    bRes = Cudd_Not( dd->one );  Cudd_Ref( bRes );

    // add cubes, one by one
    Mvc_CoverForEachCube( Cover, pCube ) 
    {
        // start the cube
        bCube = dd->one;  Cudd_Ref( bCube );
        // create the cube
        iValues = 0;
        for ( i = 0; i < pVm->nVarsIn; i++ )
        {
            assert( iValues == pVm->pValuesFirst[i] );
            // check it is true that the value set is a don't-care 
            fDontCare = 1;
            for ( v = 0; v < pVm->pValues[i]; v++ )
                if ( !Mvc_CubeBitValue( pCube, iValues + v) ) 
                {
                    fDontCare = 0;
                    break;
                }
            if ( !fDontCare )
            {
                // start the value set of this variable
                bValues = Cudd_Not( dd->one );  Cudd_Ref( bValues );
                // build the value set
                for ( v = 0; v < pVm->pValues[i]; v++ )
                    if ( Mvc_CubeBitValue( pCube, iValues + v ) ) 
                    {
                        bValues = Cudd_bddOr( dd, bTemp = bValues, pInputMdds[iValues + v] ); Cudd_Ref( bValues );
                        Cudd_RecursiveDeref( dd, bTemp );
                    }

                // add the value set to the cube
                bCube = Cudd_bddAnd( dd, bTemp = bCube, bValues );  Cudd_Ref( bCube );
                Cudd_RecursiveDeref( dd, bTemp );
                Cudd_RecursiveDeref( dd, bValues );
            }

            // add to the total number of values
            iValues += pVm->pValues[i];
        }

        // add the cube to the MDD
        bRes = Cudd_bddOr( dd, bTemp = bRes, bCube );  Cudd_Ref( bRes );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bCube );
    }

    // return the cover
    Cudd_Deref( bRes );
    return bRes;
}


/**Function*************************************************************

  Synopsis    [Derives one i-set from ZDD representing MV-input binary-output 
  SOP in positional notation.]

  Description [Takes the variable map (pMap) and the MV ISOP represented
  using ZDD with this variable map. Converts the ZDD into the MDD using
  a studip method of extracting cubes, creating the MDDs of these cubes
  and adding all the MDD cubes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Fnc_FunctionDeriveMddFromZdd( DdManager * dd, Vm_VarMap_t * pVm, DdNode * zIsop, DdNode ** pInputMdds )
{
    DdNode * zCover, * zCube, * zTemp;
    DdNode * bRes, * bCube, * bLiteral, * bTemp;
    int * pZddVars = pVm->pMan->pArray1;
    int nZddVars = dd->sizeZ;
    int iValue, i, v, fFullLiteral;

    // iterate through the cubes of the ZDD
    bRes = b0;        Cudd_Ref( bRes );
    zCover = zIsop;   Cudd_Ref( zCover );
    while ( zCover != z0 )
    {
        // get a cube
        zCube = Extra_zddSelectOneCube( dd, zCover );       Cudd_Ref( zCube );
        // subtract the cube from the cover
        zCover = Cudd_zddDiff( dd, zTemp = zCover, zCube ); Cudd_Ref( zCover );
        Cudd_RecursiveDerefZdd( dd, zTemp );

        // copy the cube into the temporary array
        memset( pZddVars, 0, sizeof(int) * nZddVars );
        for ( zTemp = zCube; zTemp != z1; zTemp = cuddT(zTemp) )
            pZddVars[ zTemp->index ] = 1;
        Cudd_RecursiveDerefZdd( dd, zCube );

        // compute the MDD of this cube
        iValue = 0;
        bCube = b1;  Cudd_Ref( bCube );
        for ( i = 0; i < pVm->nVarsIn; i++ )
        {
            // check if the literal is full
            fFullLiteral = 1;
            for ( v = 0; v < pVm->pValues[i]; v++ )
                if ( pZddVars[iValue+v] == 0 )
                {
                    fFullLiteral = 0;
                    break;
                }
            if ( fFullLiteral )
            { // no need to add the constant 1 literal to the cube
                iValue += pVm->pValues[i];
                continue;
            }

            // compute the literal
            bLiteral = b0;  Cudd_Ref( bLiteral );
            for ( v = 0; v < pVm->pValues[i]; v++ )
            {
                if ( pZddVars[iValue] )
                {
                    bLiteral = Cudd_bddOr( dd, bTemp = bLiteral, pInputMdds[iValue+v] ); Cudd_Ref( bLiteral );
                    Cudd_RecursiveDeref( dd, bTemp );
                }
                iValue++;
            }
            // add the literal to the cube
            bCube = Cudd_bddAnd( dd, bTemp = bCube, bLiteral ); Cudd_Ref( bCube );
            Cudd_RecursiveDeref( dd, bTemp );
            Cudd_RecursiveDeref( dd, bLiteral );
        }
        assert( iValue == pVm->nValuesIn );
        // add the cube to the MDD
        bRes = Cudd_bddOr( dd, bTemp = bRes, bCube ); Cudd_Ref( bRes );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bCube );

    }
    Cudd_RecursiveDerefZdd( dd, zCover );
    Cudd_Deref( bRes );
    return bRes;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



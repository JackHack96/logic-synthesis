/**CFile****************************************************************

  FileName    [cvrFunc.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [The APIs of the Cvr package.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - March 15, 2003.]

  Revision    [$Id: cvrFunc.c,v 1.17 2003/05/27 23:14:57 alanmi Exp $]

***********************************************************************/

#include "cvrInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************
  Synopsis    [Compute the default cover by complementing other i-sets.]
  Description []
  SideEffects []
  SeeAlso     []
***********************************************************************/
void
Cvr_CoverComputeDefault (Cvr_Cover_t *cvr)
{
    Mvc_Cover_t *pCunion, *pCtmp;
    int i, iDef, nVal;
    
    assert(cvr);
    iDef = Cvr_CoverReadDefault(cvr);
    if (iDef < 0) return;
    
    nVal = Vm_VarMapReadValuesOutNum(cvr->pVm);
    
    pCunion = NULL;
    for (i=0; i<nVal; i++) {
        if (cvr->ppCovers[i]==NULL) continue;
        if (pCunion==NULL) {
            pCunion = Mvc_CoverDup(cvr->ppCovers[i]);
        }
        else {
            pCtmp   = pCunion;
            pCunion = Mvc_CoverBooleanOr(pCtmp, cvr->ppCovers[i]);
            Mvc_CoverFree(pCtmp);
        }
    }
    
    /* should use filtering for complementation here */
    cvr->ppCovers[iDef] = Cvr_CoverComplement( cvr->pVm, pCunion );
    Mvc_CoverFree(pCunion);
    return;
}

 
/**Function*************************************************************
  Synopsis    [Recompute the default iset as the one with largest cost]
  Description []
  SideEffects []
  SeeAlso     []
***********************************************************************/
void
Cvr_CoverResetDefault (Cvr_Cover_t *cvr)
{
    int i, nVal, nWst, iWst, nTmp;
    
    assert(cvr);
    Cvr_CoverComputeDefault(cvr);
    
    nWst = -1;
    nVal = Vm_VarMapReadValuesOutNum(cvr->pVm);
    for (i=0; i<nVal; i++) {
        nTmp = Mvc_CoverReadCubeNum(cvr->ppCovers[i]);
        if (nWst < nTmp) {
            nWst = nTmp;
            iWst = i;
        }
    }
    Mvc_CoverFree(cvr->ppCovers[iWst]);
    cvr->ppCovers[iWst] = NULL;

    if ( cvr->pTree )
    {
        if ( cvr->pTree->pRoots[iWst] )
        {
            // update the literal counter
            cvr->pTree->nNodes -= Ft_FactorCountLeavesOne( cvr->pTree->pRoots[iWst] );
            // remove the root
            Ft_TreeFreeRoot( cvr->pTree, iWst );
        }
    }
    return;
}


/**Function*************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
***********************************************************************/
void
Cvr_CoverSetDefault  ( Cvr_Cover_t * pCvr, int DefValue ) 
{
    assert(DefValue>=0 && DefValue<Vm_VarMapReadValuesOutNum(pCvr->pVm));
    if (pCvr->ppCovers[DefValue] == NULL) return;
    
    Cvr_CoverComputeDefault(pCvr);
    Mvc_CoverFree(pCvr->ppCovers[DefValue]);
    pCvr->ppCovers[DefValue] = NULL;
    
    return;
}


/**Function*************************************************************
  Synopsis    [Test if a cover is constant]
  Description [Return ture is the cover is constant. Bit vector bPos contain
  the output values that are const. (non-deterministic const)]
  SideEffects []
  SeeAlso     []
***********************************************************************/
bool
Cvr_CoverIsConst (
    Cvr_Cover_t *pCvr,
    unsigned    *bPos) 
{
    int i, nVal, nCount=0;
    
    assert(pCvr);
    nVal = Vm_VarMapReadValuesOutNum(pCvr->pVm);
    *bPos = 0;
    
    for (i=0; i<nVal; ++i) {
        if (pCvr->ppCovers[i]==NULL) continue;
        
        if (Mvc_CoverIsTautology(pCvr->ppCovers[i])) {
            nCount++;
            (*bPos) |= (1<<i);
        }
        else if (!Mvc_CoverIsEmpty(pCvr->ppCovers[i])) {
            return FALSE;
        }
    }
    return (nCount);
}

/**Function*************************************************************
  Synopsis    [Adjust the covers to a new variable map.]
  Description [pnPos[i] tells the new position of current variable i]
  SideEffects []
  SeeAlso     []
***********************************************************************/
void
Cvr_CoverAdjust  (
    Cvr_Cover_t *pCvr,
    Vm_VarMap_t *pVmNew,
    int         *pnPos)
{
    int  iVal, nVal;
    Mvc_Cover_t *pMvcNew;
    
    // nVal was not assigned - alanmi
    nVal = Vm_VarMapReadValuesOutNum(pCvr->pVm);

    for (iVal=0; iVal<nVal; ++iVal) {
        if (pCvr->ppCovers[iVal] == NULL) continue;
        pMvcNew = Cvr_IsetAdjust (pCvr->ppCovers[iVal],pVmNew,pCvr->pVm,pnPos);
        Mvc_CoverFree(pCvr->ppCovers[iVal]);
        pCvr->ppCovers[iVal] = pMvcNew;
    }
    return;
}

/**Function*************************************************************
  Synopsis    [Creates the adjusted cover without deallocating the old cover.]
  Description [pnPos[i] tells the new position of current variable i]
  SideEffects []
  SeeAlso     []
***********************************************************************/
Cvr_Cover_t *
Cvr_CoverCreateAdjusted(
    Cvr_Cover_t *pCvr,
    Vm_VarMap_t *pVmNew,
    int         *pnPos)
{
    Mvc_Cover_t ** ppIsets;
    int  iVal, nVal;
    
    nVal = Vm_VarMapReadValuesOutNum(pCvr->pVm);
    ppIsets = ALLOC( Mvc_Cover_t *, nVal );
    for (iVal=0; iVal<nVal; ++iVal) 
    {
        if (pCvr->ppCovers[iVal] == NULL) 
        {
            ppIsets[iVal] = NULL;
            continue;
        }
        ppIsets[iVal] = Cvr_IsetAdjust (pCvr->ppCovers[iVal],pVmNew,pCvr->pVm,pnPos);
    }
    return Cvr_CoverCreate( pVmNew, ppIsets );
}


/**Function*************************************************************
  Synopsis    [Adjust the covers to a new variable map.]
  Description [pnPos[i] tells the new position of current variable i]
  SideEffects []
  SeeAlso     []
***********************************************************************/
Mvc_Cover_t*
Cvr_IsetAdjust (
    Mvc_Cover_t *pIset,
    Vm_VarMap_t *pMnew,
    Vm_VarMap_t *pMold, 
    int         *pnPos)
{
    int  nBits, nVars, nVals, iPos, i, k;
    int *pnBits, *pnFirst;
    Mvc_Cover_t *pMvcNew;
    
    nVars = Vm_VarMapReadVarsInNum(pMold);
    nBits = Vm_VarMapReadValuesInNum(pMnew);
    pnFirst = Vm_VarMapReadValuesFirstArray(pMnew);
    pnBits = ALLOC(int, nBits);
    for ( i=0; i<nBits; ++i ) {
        pnBits[i] = -1;
    }
    
    for (i=0; i<nVars; ++i) {
        nVals = Vm_VarMapReadValues(pMold, i);
        iPos  = Vm_VarMapReadValuesFirst(pMold, i);
        for (k=0; k<nVals; ++k) {
            /* pnBits[new position] = old position */
            pnBits[pnFirst[pnPos[i]]+k] = iPos+k;
        }
    }
    pMvcNew = Mvc_CoverRemap(pIset, pnBits, nBits);
    
    FREE( pnBits );
    return pMvcNew;
}

/**Function*************************************************************
  Synopsis    [Adjust the covers to a new variable map return a new cover.]
  Description [pnPos[i] tells the new position of current variable i]
  SideEffects []
  SeeAlso     []
***********************************************************************/
Cvr_Cover_t *
Cvr_CoverCreateAdjust  (
    Cvr_Cover_t *pCvr,
    Vm_VarMap_t *pVmNew,
    int         *pnPos)
{
    int  iVal, nVal;
    Cvr_Cover_t *pCvrNew;
    
    if (pCvr==NULL) return NULL;
    
    /* clone the information */
    pCvrNew = ALLOC( Cvr_Cover_t, 1 );
    pCvrNew->pVm = pVmNew;
    pCvrNew->pTree = NULL;
    
    nVal = Vm_VarMapReadValuesOutNum(pCvr->pVm);
    pCvrNew->ppCovers = ALLOC( Mvc_Cover_t *, nVal );
    
    for (iVal=0; iVal<nVal; ++iVal) {
        if (pCvr->ppCovers[iVal]) {
            pCvrNew->ppCovers[iVal] = Cvr_IsetAdjust (pCvr->ppCovers[iVal],pVmNew,pCvr->pVm,pnPos);
        }
        else {
            pCvrNew->ppCovers[iVal] = NULL;
        }
    }
    
    return pCvrNew;
}

/**Function*************************************************************
  Synopsis    [Create a new cover after permuting fanins of the node.]
  Description [This function create a new Cvr after the node's fanins
  have been permuted. The permutation array contains, for each new
  MV variable (i), its place in the old variable order of MV variables
  (pPermuteInv[i]).]
  SideEffects []
  SeeAlso     []
***********************************************************************/
Cvr_Cover_t *
Cvr_CoverCreatePermuted( 
    Cvr_Cover_t * pCvr, 
    int * pPermuteInv )
{
    Mvc_Cover_t * pCover;
    Mvc_Cover_t ** ppIsets;
	Vm_VarMap_t * pVm, * pVmNew;
    int nValues, nVarsIn, i, v;
    int * pValuesNew, * pValuesFirstNew;
    int * pValuesOld, * pValuesFirstOld;
    int * pBitPermute;

    // get the old MV var map
    pVm = Cvr_CoverReadVm( pCvr );
    // create the new permuted Vm
	pVmNew = Vm_VarMapCreatePermuted( pVm, pPermuteInv );
    
    // get the parameters
    // the number of inputs and the number of output values
    nVarsIn         = Vm_VarMapReadVarsInNum( pVm );
    nValues         = Vm_VarMapReadValuesOutNum( pVm );
    // the old value info
    pValuesOld      = Vm_VarMapReadValuesArray( pVm );
    pValuesFirstOld = Vm_VarMapReadValuesFirstArray( pVm );
    // the new value info
    pValuesNew      = Vm_VarMapReadValuesArray( pVmNew );
    pValuesFirstNew = Vm_VarMapReadValuesFirstArray( pVmNew );

    // get one of the old covers
    pCover = NULL;
    for ( i = 0; i < nValues; i++ )
        if ( pCvr->ppCovers[i] )
        {
            pCover = pCvr->ppCovers[i];
            break;
        }        

    // create the permutation of bits
    pBitPermute = ALLOC( int, pCover->nBits );
    // go through all variables of the new map
    for ( i = 0; i < nVarsIn; i++ )
        for ( v = 0; v < pValuesNew[i]; v++ )
        {
            // make sure that the number of values of old and new MV var is the same
            assert( pValuesNew[i] == pValuesOld[pPermuteInv[i]] );
            pBitPermute[ pValuesFirstNew[i] + v ] =
                pValuesFirstOld[pPermuteInv[i]] + v;
        }

    // create the new Cvr after permutation
    ppIsets = ALLOC( Mvc_Cover_t *, nValues );
    for ( i = 0; i < nValues; i++ )
        if ( pCvr->ppCovers[i] )
            ppIsets[i] = Mvc_CoverRemap( pCvr->ppCovers[i], pBitPermute, pCover->nBits );
        else
            ppIsets[i] = NULL;
    FREE( pBitPermute );
    // create and return the cover
    return Cvr_CoverCreate( pVmNew, ppIsets );
}


/**Function*************************************************************

  Synopsis    [Counts the number of MV literals in the cover.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int
Cvr_CoverCountLiterals( Vm_VarMap_t * pVm, Mvc_Cover_t * pCover )
{
    Mvc_Cube_t * pCube;
    int nVarsIn, nValuesIn;
    int * pValues, iValues;
    bool fDontCare;
    int Counter;
    int i, v;

    // get the var map parameters
    nVarsIn   = Vm_VarMapReadVarsInNum( pVm );
    nValuesIn = Vm_VarMapReadValuesInNum( pVm );
    pValues   = Vm_VarMapReadValuesArray( pVm );

    // go through the cubes and count literals
    Counter = 0;
    Mvc_CoverForEachCube( pCover, pCube ) 
    {
        iValues = 0;
        for ( i = 0; i < nVarsIn; i++ )
        {
            // check it is true that the value set is a don't-care 
            fDontCare = 1;
            for ( v = 0; v < pValues[i]; v++ )
                if ( !Mvc_CubeBitValue( pCube, iValues + v ) ) 
                {
                    fDontCare = 0;
                    break;
                }
            if ( !fDontCare )
                Counter++;
            // add to the total number of values
            iValues += pValues[i];
        }
        assert( iValues == nValuesIn );
    }
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Counts the number of values in all MV literals.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int
Cvr_CoverCountLiteralValues( Vm_VarMap_t * pVm, Mvc_Cover_t * pCover )
{
    Mvc_Cube_t * pCube;
    int   nVarsIn, nValuesIn;
    int * pValues, iValues;
    int   nLitValues, Counter, i, v;

    // get the var map parameters
    nVarsIn   = Vm_VarMapReadVarsInNum( pVm );
    nValuesIn = Vm_VarMapReadValuesInNum( pVm );
    pValues   = Vm_VarMapReadValuesArray( pVm );

    // go through the cubes and count literals
    nLitValues = 0;
    Mvc_CoverForEachCube( pCover, pCube ) 
    {
        iValues = 0;
        for ( i = 0; i < nVarsIn; i++ )
        {
            // count the number of literal values in this cube
            Counter = 0;
            for ( v = 0; v < pValues[i]; v++ )
                if ( Mvc_CubeBitValue( pCube, iValues + v ) ) 
                {
                    Counter++;
                }
            // if not full-set then add the #values in this literal
            if ( Counter != pValues[i] )
                nLitValues += Counter;
            // add to the total number of values
            iValues += pValues[i];
        }
        assert( iValues == nValuesIn );
    }
    return nLitValues;
}

/**Function*************************************************************
  Synopsis    [Boolean OR of two binary covers]
  Description []
  SideEffects []
  SeeAlso     []
***********************************************************************/
Cvr_Cover_t *
Cvr_CoverOr  ( Cvr_Cover_t *pCvr1, Cvr_Cover_t *pCvr2 )
{
    bool fMvc1, fMvc2;
    Mvc_Cover_t **pIsets, *pMvc1, *pMvc2;
    Cvr_Cover_t *pCvrNew;
    
    if ( pCvr1==NULL || pCvr2==NULL) return NULL;
    
    if ( Vm_VarMapReadValuesOutNum(pCvr1->pVm) != 2 ||
         Vm_VarMapReadValuesOutNum(pCvr2->pVm) != 2 ) {
        return NULL;
    }
    
    /* derive onsets of the two covers */
    pIsets = ALLOC( Mvc_Cover_t *, 2 );
    fMvc1 = fMvc2 = 0;
    if ( pCvr1->ppCovers[1] ) {
        pMvc1 = pCvr1->ppCovers[1];
    } else {
        pMvc1 = Cvr_CoverComplement( pCvr1->pVm, pCvr1->ppCovers[0] );
        fMvc1 = 1;
    }
    if ( pCvr2->ppCovers[1] ) {
        pMvc2 = pCvr2->ppCovers[1];
    } else {
        pMvc2 = Cvr_CoverComplement( pCvr2->pVm, pCvr2->ppCovers[0] );
        fMvc2 = 1;
    }
    
    /* perform the disjunction */
    pIsets[1] = Mvc_CoverBooleanOr( pMvc1, pMvc2 );
    pIsets[0] = NULL;
    if ( fMvc1 ) Mvc_CoverFree( pMvc1 );
    if ( fMvc2 ) Mvc_CoverFree( pMvc2 );
    
    /* create the new cover */
    pCvrNew = Cvr_CoverCreate( pCvr1->pVm, pIsets );
    
    return pCvrNew;
}



/**Function*************************************************************

  Synopsis    [Derive the collapsed cover.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cvr_Cover_t * Cvr_CoverCreateCollapsed( Cvr_Cover_t * pCvrN, Cvr_Cover_t * pCvrF, 
        Vm_VarMap_t * pVmNew, int * pTransMapNInv, int * pTransMapF )
{
    Mvc_Data_t * pData;
    Cvr_Cover_t * pCvrNshift, * pCvrFshift, * pCvrNew;
    Mvc_Cover_t ** ppIsets, ** ppCofsN;
    Mvc_Cover_t * pIsetRes, * pMvcTemp;
    Vm_VarMap_t * pVmNewF;
    int * pTransMapN;
    int DefValueFinit, DefValueF, VarFofN, nVarsIn;
    int nValuesF, DefValueN, nValuesN;
    int v, i;

    // get parameters
    nValuesN  = Vm_VarMapReadValuesOutNum(pCvrN->pVm);
    nValuesF  = Vm_VarMapReadValuesOutNum(pCvrF->pVm);
    DefValueN = Cvr_CoverReadDefault(pCvrN);
    DefValueF = Cvr_CoverReadDefault(pCvrF);
    VarFofN   = pTransMapF[ Vm_VarMapReadVarsInNum(pCvrF->pVm) ];
    nVarsIn   = Vm_VarMapReadVarsInNum(pVmNew);

    // inverse pTransMapNInv
    pTransMapN = ALLOC( int, nVarsIn );
    for ( i = 0; i < nVarsIn; i++ )
        if ( pTransMapNInv[i] >= 0 )
            pTransMapN[pTransMapNInv[i]] = i;

    // create the shifted var map for F
    if ( nValuesN != nValuesF )
        pVmNewF = Vm_VarMapCreateOneDiff( pVmNew, nVarsIn, nValuesF );
    else
        pVmNewF = pVmNew;

    // expand the cover of the node to match the new map
    pCvrNshift = Cvr_CoverCreateAdjusted( pCvrN, pVmNew,  pTransMapN ); 
    // remap the cover of the fanin (pFanin) to match the new map
    pCvrFshift = Cvr_CoverCreateAdjusted( pCvrF, pVmNewF, pTransMapF ); 
    FREE( pTransMapN );


    // get any cover of pCvrFshift
    pMvcTemp = (DefValueF != 0)? pCvrFshift->ppCovers[0]: pCvrFshift->ppCovers[1];
    // get the MV data for the new man
    pData = Mvc_CoverDataAlloc( pVmNew, pMvcTemp );

    // if the cover of node N has literals, which include the def value of F, 
    // we need the def value cover of F explicitly
    DefValueFinit = DefValueF;
    if ( Cvr_CoverCountLitsWithValue(pData, pCvrNshift, VarFofN, DefValueF) )
    {
        if ( DefValueF >= 0 )
        {
            Cvr_CoverComputeDefault( pCvrFshift );
            DefValueF = -1;
        }
    }

    // if the default value is used in one of the covers, and one of them is ND
    // we need to get rid of the default value and have all i-sets present
    if ( (DefValueF >= 0 || DefValueN >= 0) && Cvr_CoverIsND(pData, pCvrFshift) )
    {
        if ( DefValueN >= 0 )
        {
            Cvr_CoverComputeDefault( pCvrNshift );
            DefValueN = -1;
        }
        if ( DefValueF >= 0 )
        {
            Cvr_CoverComputeDefault( pCvrFshift );
            DefValueF = -1;
        }
    }

    // start the resulting i-sets
    ppIsets = ALLOC( Mvc_Cover_t *, nValuesN );
    // for each value of N, compose the covers
    for ( v = 0; v < nValuesN; v++ )
    {
        if ( pCvrNshift->ppCovers[v] == NULL )
        {
            ppIsets[v] = NULL;
            continue;
        }
        // cofactor the corresponding i-set of N w.r.t the fanin var
        // and compute the cubes that do not depend on this var (ppCofsN[nValuesF])
        ppCofsN = Mvc_CoverCofactors( pData, pCvrNshift->ppCovers[v], VarFofN );

        // convolve the cofactors
        pIsetRes = Mvc_CoverClone( pCvrNshift->ppCovers[v] );
        for ( i = 0; i < nValuesF; i++ )
        {
            if ( Mvc_CoverReadCubeNum(ppCofsN[i]) > 0 )
            {
                // there are cubes, there should be the corresponding i-set of F-shifted
                assert( pCvrFshift->ppCovers[i] );
                // multiply
                Mvc_CoverIntersectCubes( pData, ppCofsN[i], pCvrFshift->ppCovers[i] );
                // add the cubes
                Mvc_CoverAppendCubes( pIsetRes, ppCofsN[i] );
            }
            Mvc_CoverFree( ppCofsN[i] );
        }
        // add the cubes that do not depend on this var
        Mvc_CoverAppendCubes( pIsetRes, ppCofsN[nValuesF] );
        Mvc_CoverFree( ppCofsN[nValuesF] );
        FREE( ppCofsN );
        // compress the cover
        Mvc_CoverContain( pIsetRes );
        Mvc_CoverDist1Merge( pData, pIsetRes );
        // set the cover
        ppIsets[v] = pIsetRes;
    }

    // create the new Cover
    pCvrNew = Cvr_CoverCreate( pVmNew, ppIsets );

    // free storage
    Mvc_CoverDataFree( pData, pMvcTemp );
    Cvr_CoverFree( pCvrNshift );
    Cvr_CoverFree( pCvrFshift );

    // remove the default value of F
//    if ( DefValueFinit >= 0 && DefValueF < 0 )
//        Cvr_CoverSetDefault( pCvrF, DefValueFinit );

    return pCvrNew;
}

/**Function*************************************************************

  Synopsis    [Returns the cover with the appended variables.]

  Description [The original cover is expanded by adding some variables.
  These variables are the additional variables in pVmNew, compared to
  pCvr->pVm. The resulting cover is the same as the original one, except
  that it contains the additional variables present as full literals
  in every cube.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cvr_Cover_t * Cvr_CoverCreateExpanded( Cvr_Cover_t * pCvr, Vm_VarMap_t * pVmNew )
{
    Mvc_Cover_t ** ppIsets;
    int  iVal, nVal;
    
    nVal = Vm_VarMapReadValuesOutNum(pCvr->pVm);
    ppIsets = ALLOC( Mvc_Cover_t *, nVal );
    for ( iVal = 0; iVal < nVal; ++iVal ) 
    {
        if ( pCvr->ppCovers[iVal] == NULL ) 
        {
            ppIsets[iVal] = NULL;
            continue;
        }
        ppIsets[iVal] = Mvc_CoverCreateExpanded( pCvr->ppCovers[iVal], pVmNew );
    }
    return Cvr_CoverCreate( pVmNew, ppIsets );
}



/**Function*************************************************************

  Synopsis    [Checks if the given Cvr is non-deterministic.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Cvr_CoverIsND( Mvc_Data_t * pData, Cvr_Cover_t * pCvr )
{
    int i, k, nValues;
    nValues = Vm_VarMapReadValuesOutNum(pCvr->pVm);
    for ( i = 0; i < nValues; i++ )
        for ( k = i + 1; k < nValues; k++ )
            if ( pCvr->ppCovers[i] && pCvr->ppCovers[k] )
                if ( Mvc_CoverIsIntersecting( pData, pCvr->ppCovers[i], pCvr->ppCovers[k] ) )
                    return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Count the cubes with non-trivial literals with the given value.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cvr_CoverCountLitsWithValue( Mvc_Data_t * pData, Cvr_Cover_t * pCvr, int iVar, int iValue )
{
    int v, Counter, nValuesOut;
    Counter = 0;
    nValuesOut = Vm_VarMapReadValues( pCvr->pVm, Vm_VarMapReadVarsInNum(pCvr->pVm) );
    for ( v = 0; v < nValuesOut; v++ )
        if ( pCvr->ppCovers[v] )
            Counter += Mvr_CoverCountLitsWithValue( pData, pCvr->ppCovers[v], iVar, iValue );
    return Counter;
}


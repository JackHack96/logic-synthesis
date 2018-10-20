/**CFile****************************************************************

  FileName    [cvrRelation.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [two-level relation minimization]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - March 15, 2003.]

  Revision    [$Id: cvrRelation.c,v 1.5 2003/05/27 23:15:01 alanmi Exp $]

***********************************************************************/

#include "mv.h"
#include "cvrInt.h"
#include "espresso.h"
#include "minimize.h"

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/
static void          CvrCoverOrderIsets( Cvr_Cover_t *pCf, int *pnOrder );
static int           CvrCoverComputeIsetCost( Vm_VarMap_t *pVm, Mvc_Cover_t *pMvc );
static Mvc_Cover_t * CvrComputeEssential( Mvc_Data_t *pMvcData, Mvc_Cover_t **ppMvc, int nVal, int iSeq );


/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************
  Synopsis    []
  Description [does not reset default explicitly]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Cvr_CoverEspressoRelation(
    Cvr_Cover_t * pCf,
    Mvc_Cover_t * pCd,
    int           method,
    bool          fSparse,
    bool          fConserve,
    bool          fPhase )
{
    bool          fDefault;
    int           nVal, i, iSeq, *pnOrder;
    Mvc_Data_t   *pMvcData;
    Mvc_Cover_t **ppMvc, *pIsetEss, *pIsetFlx, *pIsetDc, *pTemp;
    
    /* deal with tautology i-set */
    if ( Cvr_CoverEspressoSpecial( pCf ) )
        return;
    
    nVal = Vm_VarMapReadValuesOutNum(pCf->pVm);
    
    /* prepare the cube structure */
    Cvr_CoverEspressoSetup( pCf->pVm );
    
    /* if asked to reset default */
    if ( fPhase ) {
        Cvr_CoverComputeDefault(pCf);
    }
    
    /* nondeterministic relation */
    ppMvc = Cvr_CoverReadIsets( pCf );
    pMvcData = NULL;
    
    /* assume there is no default i-set */
    fDefault = FALSE;
    
    /* first perform d1merge */
    for ( i=0; i < nVal; ++i ) {
        
        if ( ppMvc[i] == NULL ) {
            fDefault = TRUE;
            continue;
        }
        if ( pMvcData == NULL ) {
            pMvcData = Mvc_CoverDataAlloc( pCf->pVm, ppMvc[i] );
        }
        
        Mvc_CoverDist1Merge( pMvcData, ppMvc[i] );
    }
    
    /* find an heuristic ordering */
    pnOrder = Vm_VarMapGetStorageSupport1( pCf->pVm );
    CvrCoverOrderIsets( pCf, pnOrder );
    
    /* minimize each i-set in this order */
    for ( i=0; i < nVal; ++i ) {
        
        iSeq = pnOrder[i];
        if ( ppMvc[iSeq] == NULL ) {
            continue;
        }
        
        /* avoid sharp operator for large i-sets */
        if ( fConserve && Cvr_IsetIsTooLarge( pCf->pVm, ppMvc[iSeq]) ) {
            
            pTemp = Cvr_IsetEspresso( pCf->pVm, ppMvc[iSeq], pCd, method,
                                      fSparse, fConserve );
            Mvc_CoverFree( ppMvc[iSeq] );
            ppMvc[iSeq] = pTemp;
            continue;
        }
        
        /* compute essential minterms */
        pIsetEss = CvrComputeEssential( pMvcData, ppMvc, nVal, iSeq );
        pIsetFlx = Mvc_CoverSharp( pMvcData, ppMvc[iSeq], pIsetEss );
        
        if ( pCd ) {
            pIsetDc = Mvc_CoverBooleanOr( pIsetFlx, pCd );
            Mvc_CoverFree( pIsetFlx );
        }
        else {
            pIsetDc = pIsetFlx;
        }
        
        pTemp = Cvr_IsetEspresso( pCf->pVm, pIsetEss, pIsetDc, method,
                                  fSparse, fConserve );
        
        Mvc_CoverFree( pIsetEss );
        Mvc_CoverFree( pIsetDc );
        Mvc_CoverFree( ppMvc[iSeq] );
        ppMvc[iSeq] = pTemp;
    }
    
    /* reset the default if asked to do so, or all i-sets are present */
    if ( fPhase || !fDefault ) {
        Cvr_CoverResetDefault( pCf );
    }
    
    /* clean up memory */
    if ( pMvcData ) {
        for ( i=0; i < nVal; ++i )
            if ( ppMvc[i] ) break;
        Mvc_CoverDataFree( pMvcData, ppMvc[i] );
    }
    sf_cleanup();
    Cvr_CoverEspressoSetdown(pCf->pVm);
    
    return;
}



/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
CvrCoverOrderIsets( Cvr_Cover_t *pCf, int *pnOrder ) 
{
    int   i, j, nVals, *pnCost, nLeast, iLeast;
    Mvc_Cover_t  **ppMvc;
    
    
    nVals  = Vm_VarMapReadValuesOutNum( pCf->pVm );
    pnCost = Vm_VarMapGetStorageSupport2( pCf->pVm );
    ppMvc  = Cvr_CoverReadIsets( pCf );
    
    /* initialize the cost */
    for ( i = 0; i < nVals; ++i ) {
        if ( ppMvc[i] )
            pnCost[i] = CvrCoverComputeIsetCost( pCf->pVm, ppMvc[i] );
        else
            pnCost[i] = (int)1<<20;  //guarranteed to be the last
    }
    
    /* bubble sort (not efficient but who cares) */
    for ( i = 0; i < nVals; ++i ) {
        
        nLeast = (int)1<<21;
        iLeast = -1;
        for ( j = 0; j < nVals; ++j ) {
            if ( pnCost[j] >= 0 && nLeast > pnCost[j] ) {
                nLeast = pnCost[j];
                iLeast = j;
            }
        }
        if ( iLeast >=0 ) {
            pnCost[ iLeast ] = -1;
            pnOrder[i] = iLeast;
        }
    }
    
    return;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
CvrCoverComputeIsetCost( Vm_VarMap_t *pVm, Mvc_Cover_t *pMvc )
{
    int nCost;
    
    nCost = Mvc_CoverReadCubeNum( pMvc );
    nCost <<= 3;
    nCost += Cvr_CoverCountLiterals( pVm, pMvc );
    
    return nCost;
}


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Mvc_Cover_t *
CvrComputeEssential( Mvc_Data_t *pMvcData, Mvc_Cover_t **ppMvc, int nVal, int iSeq )
{
    int i;
    Mvc_Cover_t *pEssen, *pTemp;
    
    assert( iSeq >= 0 && iSeq < nVal );
    assert( ppMvc[iSeq] );
    
    pEssen = Mvc_CoverDup( ppMvc[iSeq] );
    
    for ( i = 0; i < nVal; ++i ) {
        if ( i == iSeq )
            continue;
        if ( ppMvc[i] ) {
            pTemp = Mvc_CoverSharp( pMvcData, pEssen, ppMvc[i] );
            Mvc_CoverFree( pEssen );
            pEssen = pTemp;
        }
        if ( Mvc_CoverReadCubeNum( pEssen ) == 0 )
            break;
    }
    
    return pEssen;
}



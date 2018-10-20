/**CFile****************************************************************

  FileName    [fraigSymms.c]

  PackageName [FRAIG: Functionally reduced AND-INV graphs.]

  Synopsis    []

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 4, 2005]

  Revision    [$Id: fraigSymSim.c,v 1.3 2005/07/08 01:01:33 alanmi Exp $]

***********************************************************************/

#include "fraigInt.h"
#include "extra.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define FRAIG_SYM_MAX_INPUTS    512

static void Fraig_SymmsSimComputeOneInt( Fraig_Man_t * p, Fraig_Node_t * pNode, unsigned ** pSymmInfo, Extra_BitMat_t * pMat );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes symmetries for a single-output function.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Extra_BitMat_t * Fraig_SymmsSimComputeOne( Fraig_Man_t * p, Fraig_Node_t * pNode, int nRoundLimit )
{
    Extra_BitMat_t * pMat;
    int i, nWords, nRounds, nAttempts;
    unsigned ** pSimmInfo;
    int nPairsPrev = 0, nPairsCur, nCounter;
    int nSaturation = 10;
 
    assert( p->vInputs->nSize < FRAIG_SYM_MAX_INPUTS );
    assert( !Fraig_IsComplement( pNode ) );
//Fraig_MappingShow( p, "aig" );

    // allocate memory for the simulation info
    pSimmInfo    = ALLOC( unsigned *, p->vNodes->nSize );
    pSimmInfo[0] = ALLOC( unsigned, p->vNodes->nSize * FRAIG_SIM_ROUNDS );
    for ( i = 1; i < p->vNodes->nSize; i++ )
        pSimmInfo[i] = pSimmInfo[i-1] + FRAIG_SIM_ROUNDS;

    // get the number of words for one round
    nWords = p->vInputs->nSize / 32 + ((p->vInputs->nSize % 32) > 0);
    // get the number of simulation rounds (using the given number of patterns)
    nRounds = FRAIG_SIM_ROUNDS / nWords;
    // get the number of attempts to do one round
    nAttempts = nRoundLimit / nRounds + ((nRoundLimit % nRounds) > 0);

    // extract the simulation info
    pMat = Extra_BitMatrixStart( p->vInputs->nSize );
    for ( i = 0; i < nAttempts; i++ )
    {
        Fraig_SymmsSimComputeOneInt( p, pNode, pSimmInfo, pMat );
        // determine whether it is time to stop
        nPairsCur = Extra_BitMatrixCountOnesUpper( pMat );
        if ( nPairsPrev < nPairsCur )
        {
            nPairsPrev = nPairsCur;
            nCounter = 0;
        }
        else 
        {
            assert( nPairsPrev == nPairsCur );
            if ( nCounter++ == nSaturation )
                break;
        }
    }
//    printf( "\n" );

    free( pSimmInfo[0] );
    free( pSimmInfo );
    return pMat;
}

/**Function*************************************************************

  Synopsis    [Computes symmetries for a single output function.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_SymmsSimComputeOneInt( Fraig_Man_t * p, Fraig_Node_t * pNode, 
    unsigned ** pSymmInfo, Extra_BitMat_t * pMat )
{
    Fraig_Node_t * pTemp;
    unsigned uPattern[FRAIG_SIM_ROUNDS][FRAIG_SYM_MAX_INPUTS/32];
    unsigned uColumn[FRAIG_SYM_MAX_INPUTS/32], uRow[FRAIG_SYM_MAX_INPUTS/32];
    unsigned * pSims, * pSims0, * pSims1;
    int i, w, r, fCompl0, fCompl1, nWords, nRounds;

    // get the number of words for one round
    nWords = p->vInputs->nSize / 32 + ((p->vInputs->nSize % 32) > 0);
    nRounds = FRAIG_SIM_ROUNDS / nWords;

    // create random patterns
    for ( r = 0; r < nRounds; r++ )
        for ( w = 0; w < nWords; w++ )
            uPattern[r][w] = FRAIG_RANDOM_UNSIGNED;

    // assign square matrices of patterns
    for ( r = 0; r < nRounds; r++ )
    {
        // for each PI var copy the pattern
        for ( i = 0; i < p->vInputs->nSize; i++ )
        {
            pTemp = p->vInputs->pArray[i];
            // set the bit patterns
            if ( Fraig_BitStringHasBit( uPattern[r], i ) )
            {
                for ( w = 0; w < nWords; w++ )
                    pSymmInfo[pTemp->Num][r*nWords+w] = FRAIG_FULL;
            }
            else
            {
                for ( w = 0; w < nWords; w++ )
                    pSymmInfo[pTemp->Num][r*nWords+w] = (unsigned)0;
            }

            // complement the i-th bit
            Fraig_BitStringXorBit( pSymmInfo[pTemp->Num] + r*nWords, i );
        }
    }

    // simulate
    for ( i = 0; i < p->vNodes->nSize; i++ )
    {
        pTemp = p->vNodes->pArray[i];
        if ( !Fraig_NodeIsAnd(pTemp) )
            continue;

        fCompl0 = Fraig_IsComplement( pTemp->p1 );
        fCompl1 = Fraig_IsComplement( pTemp->p2 );
        pSims  = pSymmInfo[pTemp->Num];
        pSims0 = pSymmInfo[Fraig_Regular(pTemp->p1)->Num];
        pSims1 = pSymmInfo[Fraig_Regular(pTemp->p2)->Num];
        if ( fCompl0 && fCompl1 )
            for ( w = 0; w < FRAIG_SIM_ROUNDS; w++ )
                pSims[w] = ~pSims0[w] & ~pSims1[w];
        else if ( !fCompl0 && fCompl1 )
            for ( w = 0; w < FRAIG_SIM_ROUNDS; w++ )
                pSims[w] = pSims0[w] & ~pSims1[w];
        else if ( fCompl0 && !fCompl1 )
            for ( w = 0; w < FRAIG_SIM_ROUNDS; w++ )
                pSims[w] = ~pSims0[w] & pSims1[w];
        else // if ( fCompl0 && fCompl1 )
            for ( w = 0; w < FRAIG_SIM_ROUNDS; w++ )
                pSims[w] = pSims0[w] & pSims1[w];
    }

    // go through the simulation rounds
    pSims = pSymmInfo[pNode->Num];
    for ( r = 0; r < nRounds; r++ )
    {
        // generate vectors A1 and A2
        for ( w = 0; w < nWords; w++ )
        {
            uColumn[w] = uPattern[r][w] &  pSims[r*nWords + w];
            uRow[w]    = uPattern[r][w] & ~pSims[r*nWords + w];
        }
        // add two dimensions
        for ( i = 0; i < p->vInputs->nSize; i++ )
            if ( Fraig_BitStringHasBit( uColumn, i ) )
                Extra_BitMatrixOr( pMat, i, uRow );
        // add two dimensions
        for ( i = 0; i < p->vInputs->nSize; i++ )
            if ( Fraig_BitStringHasBit( uRow, i ) )
                Extra_BitMatrixOr( pMat, i, uColumn );

        // generate vectors B1 and B2
        for ( w = 0; w < nWords; w++ )
        {
            uColumn[w] = ~uPattern[r][w] &  pSims[r*nWords + w];
            uRow[w]    = ~uPattern[r][w] & ~pSims[r*nWords + w];
        }
        // add two dimensions
        for ( i = 0; i < p->vInputs->nSize; i++ )
            if ( Fraig_BitStringHasBit( uColumn, i ) )
                Extra_BitMatrixOr( pMat, i, uRow );
        // add two dimensions
        for ( i = 0; i < p->vInputs->nSize; i++ )
            if ( Fraig_BitStringHasBit( uRow, i ) )
                Extra_BitMatrixOr( pMat, i, uColumn );
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



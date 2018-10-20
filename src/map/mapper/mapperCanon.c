/**CFile****************************************************************

  FileName    [mapperCanon.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic technology mapping engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: mapperCanon.c,v 1.3 2005/07/08 01:01:24 alanmi Exp $]

***********************************************************************/

#include "mapperInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static unsigned Map_CanonComputePhase( unsigned uTruths[][2], int nVars, unsigned uTruth, unsigned uPhase );
static void     Map_CanonComputePhase6( unsigned uTruths[][2], int nVars, unsigned uTruth[], unsigned uPhase, unsigned uTruthRes[] );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes the N-canonical form of the Boolean function.]

  Description [The N-canonical form is defined as the truth table with 
  the minimum integer value. This function exhaustively enumerates 
  through the complete set of 2^N phase assignments.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_CanonComputeSlow( unsigned uTruths[][2], int nVarsMax, int nVarsReal, unsigned uTruth[], unsigned char * puPhases, unsigned uTruthRes[] )
{
    unsigned  uTruthPerm[2];
    int nMints, nPhases, m;

    nPhases = 0;
    nMints = (1 << nVarsReal);
    if ( nVarsMax < 6 )
    {
        uTruthRes[0] = MAP_MASK(32);
        for ( m = 0; m < nMints; m++ )
        {
            uTruthPerm[0] = Map_CanonComputePhase( uTruths, nVarsMax, uTruth[0], m );
            if ( uTruthRes[0] > uTruthPerm[0] )
            {
                uTruthRes[0] = uTruthPerm[0];
                nPhases      = 0;
                puPhases[nPhases++] = (unsigned char)m;
            }
            else if ( uTruthRes[0] == uTruthPerm[0] )
            {
                if ( nPhases < 4 ) // the max number of phases in Map_Super_t
                    puPhases[nPhases++] = (unsigned char)m;
            }
        }
        uTruthRes[1] = uTruthRes[0];
    }
    else
    {
        uTruthRes[0] = MAP_MASK(32);
        uTruthRes[1] = MAP_MASK(32);
        for ( m = 0; m < nMints; m++ )
        {
            Map_CanonComputePhase6( uTruths, nVarsMax, uTruth, m, uTruthPerm );
            if ( uTruthRes[1] > uTruthPerm[1] || uTruthRes[1] == uTruthPerm[1] && uTruthRes[0] > uTruthPerm[0] )
            {
                uTruthRes[0] = uTruthPerm[0];
                uTruthRes[1] = uTruthPerm[1];
                nPhases      = 0;
                puPhases[nPhases++] = (unsigned char)m;
            }
            else if ( uTruthRes[1] == uTruthPerm[1] && uTruthRes[0] == uTruthPerm[0] )
            {
                if ( nPhases < 4 ) // the max number of phases in Map_Super_t
                    puPhases[nPhases++] = (unsigned char)m;
            }
        }
    }
    assert( nPhases > 0 );
//    printf( "%d ", nPhases );
    return nPhases;
}

/**Function*************************************************************

  Synopsis    [Performs phase transformation for one function of less than 6 variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Map_CanonComputePhase( unsigned uTruths[][2], int nVars, unsigned uTruth, unsigned uPhase )
{
    int v, Shift;
    for ( v = 0, Shift = 1; v < nVars; v++, Shift <<= 1 )
        if ( uPhase & Shift )
            uTruth = (((uTruth & ~uTruths[v][0]) << Shift) | ((uTruth & uTruths[v][0]) >> Shift));
    return uTruth;
}

/**Function*************************************************************

  Synopsis    [Performs phase transformation for one function of 6 variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_CanonComputePhase6( unsigned uTruths[][2], int nVars, unsigned uTruth[], unsigned uPhase, unsigned uTruthRes[] )
{
    unsigned uTemp;
    int v, Shift;

    // initialize the result
    uTruthRes[0] = uTruth[0];
    uTruthRes[1] = uTruth[1];
    if ( uPhase == 0 )
        return;
    // compute the phase 
    for ( v = 0, Shift = 1; v < nVars; v++, Shift <<= 1 )
        if ( uPhase & Shift )
        {
            if ( Shift < 32 )
            {
                uTruthRes[0] = (((uTruthRes[0] & ~uTruths[v][0]) << Shift) | ((uTruthRes[0] & uTruths[v][0]) >> Shift));
                uTruthRes[1] = (((uTruthRes[1] & ~uTruths[v][1]) << Shift) | ((uTruthRes[1] & uTruths[v][1]) >> Shift));
            }
            else
            {
                uTemp        = uTruthRes[0];
                uTruthRes[0] = uTruthRes[1];
                uTruthRes[1] = uTemp;
            }
        }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



/**CFile****************************************************************

  FileName    [autiVolume.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    []

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: autiVolume.c,v 1.1 2004/02/19 03:06:52 alanmi Exp $]

***********************************************************************/

#include "autiInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static double Auti_AutoComputeVolume_rec( Aut_State_t * pState, int Length );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes the volume of the prefix closed part up to the depth.]

  Description [The volume is defined as the number of accepted input/output
  sequences. If Depth is 0, computes the number of minterms in the I/O space.
  Otherwise, performs the depths first traversal and adds the volume. For
  the MV automata, this computation is approximate because of encoding.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
double Auti_AutoComputeVolume( Aut_Auto_t * pAut, int Length )
{
    Aut_State_t * pState;
    Aut_Trans_t * pTrans;
    int * pBitsFirst;
    int nBits, i;
    double dRes;

    // get the size of boolean space
    pBitsFirst = Vmx_VarMapReadBitsFirst( pAut->pVmx );
    nBits = pBitsFirst[pAut->nVars];
    if ( nBits > 31 )
    {
        printf( "This command works only for automata with input space up to 2^31 minterms.\n" );
        return 0.0;
    }

    // set the size of boolean space for each condition
    Aut_AutoForEachState_int( pAut, pState )
    Aut_StateForEachTransitionFrom_int( pState, pTrans )
    {
        dRes = Cudd_CountMinterm( pState->pAut->pMan->dd, pTrans->bCond, nBits );
        pTrans->pData = (char *)((int)dRes);
    }

    dRes = 0.0;
    for ( i = 0; i < pAut->nInit; i++ )
        dRes += Auti_AutoComputeVolume_rec( pAut->pInit[0], Length );
    return dRes;
}

/**Function*************************************************************

  Synopsis    [Performs the recursive step.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
double Auti_AutoComputeVolume_rec( Aut_State_t * pState, int Length )
{
    Aut_Trans_t * pTrans;
    double dRes;

    // if the state is non-accepting it does not contribute
    if ( !pState->fAcc )
        return 0.0;
    // if we ran out of depth, stop
    if ( Length == 0 )
        return 1.0;
    // count the number of mintems due to each transition from this state
    dRes = 0.0;
    Aut_StateForEachTransitionFrom_int( pState, pTrans )
        dRes += ((int)pTrans->pData) *
            Auti_AutoComputeVolume_rec( pTrans->pTo, Length - 1 );
    return dRes;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



/**CFile****************************************************************

  FileName    [resmMatch.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic resynthesis engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: resmMatch.c,v 1.1 2005/01/23 06:59:52 alanmi Exp $]

***********************************************************************/

#include "resmInt.h"
#include "mapperInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Resm_MatchCutOne( Resm_Node_t * pNode, Resm_Node_t * pCut );
static void Resm_MatchCutSuper( Resm_Node_t * pNode, Resm_Node_t * pCut, Map_Super_t * pSuperList, unsigned char uPhases[], int nPhases, int fCompl );
static float Resm_TimeCutComputeArrival( Resm_Cut_t * pCut, float fWorstLimit );
static int Resm_MatchSuperContainsInv( Map_Super_t * pSuper );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Match the set of cuts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Resm_MatchCuts( Resm_Node_t * pNode, Resm_Node_t * pCutList )
{
    Resm_Node_t * pCut;
    float AreaTemp;
    // undo the current cut
    pNode->aArea = Resm_NodeAreaDeref( pNode );
    if ( pNode->nRefs[0] > 0 )
        pNode->aArea += pNode->p->AreaInv;
    // compute parameters for each cut
    for ( pCut = pCutList; pCut; pCut = pCut->pNext )
        Resm_MatchCutOne( pNode, pCut );
    // return the current cut
    AreaTemp = Resm_NodeAreaRef( pNode );
    if ( pNode->nRefs[0] > 0 )
        AreaTemp += pNode->p->AreaInv;
    assert( pNode->aArea == AreaTemp );
}

/**Function*************************************************************

  Synopsis    [Match one cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Resm_MatchCutOne( Resm_Node_t * pNode, Resm_Node_t * pCut )
{
    Resm_Man_t * p = pNode->p;
    Resm_Node_t MatchBest, * pMatchBest = &MatchBest;
    Map_Super_t * pSuperList;
    int i, k, gen, nNum, nNumInit, nMints, nGens, nCanons0, nCanons1, nSupers0, nSupers1;
    unsigned uCanons0[256][2], uCanons1[256][2], uTruth[2], uCanon[2];
    int pMints[64];
    unsigned char uPhases[8];
    int nPhases;
    int fUsePhase = 0;

    // update the arrival times of the cut inputs
    for ( i = 0; i < (int)pCut->nLeaves; i++ )
        Resm_NodeComputeArrival_rec( Resm_Regular(pCut->ppLeaves[i]) );

    // skip large cuts
    assert( pCut->nLeaves <= p->nVarsMax );

    // start the best match
    memset( pMatchBest, 0, sizeof(Resm_Node_t) );
    pMatchBest->fCompl          = pCut->fCompl          = 0;
    pMatchBest->aArea           = pCut->aArea           = RESM_FLOAT_LARGE;
    pMatchBest->tPrevious.Fall  = pCut->tPrevious.Fall  = RESM_FLOAT_LARGE;
    pMatchBest->tPrevious.Rise  = pCut->tPrevious.Rise  = RESM_FLOAT_LARGE;
    pMatchBest->tPrevious.Worst = pCut->tPrevious.Worst = RESM_FLOAT_LARGE;
    pMatchBest->tArrival.Fall   = pCut->tArrival.Fall   = RESM_FLOAT_LARGE;
    pMatchBest->tArrival.Rise   = pCut->tArrival.Rise   = RESM_FLOAT_LARGE;
    pMatchBest->tArrival.Worst  = pCut->tArrival.Worst  = RESM_FLOAT_LARGE;
    pMatchBest->pSuperBest      = pCut->pSuperBest      = NULL;
    pMatchBest->uPhase          = pCut->uPhase          = 0;
    pMatchBest->nLeaves         = pCut->nLeaves;

    // get the total number of minterms
    nMints = (1 << pCut->nLeaves);
    // find the don't-care minterms
    nNum = 0;
    for ( i = 0; i < nMints; i++ )
        if ( !Map_InfoReadVar(pCut->uTruthTemp, i) && !Map_InfoReadVar(pCut->uTruthZero, i) )
            pMints[nNum++] =  i;

    // reduce the number of don't-care minterms
    nNumInit = nNum;
    if ( nNum > 8 )
        nNum = 8;

    // enumerate through possible truth tables
    nCanons0 = 0;
    nCanons1 = 0;
    nSupers0 = 0;
    nSupers1 = 0;
    nGens    = (1 << nNum);
    for ( gen = 0; gen < nGens; gen++ )
    {
        // create the modified truth table
        uTruth[0] = pCut->uTruthTemp[0];
        uTruth[1] = pCut->uTruthTemp[1];
        for ( k = 0; k < nNum; k++ )
            if ( gen & (1 << k) )
                Map_InfoSetVar( uTruth, pMints[k] );

        // expand the truth table to fit 6 variables
        Resm_MatchExpandTruth( uTruth, pCut->nLeaves );

        // compute the canonical form and canonical phases
        nPhases = Map_CanonComputeSlow( p->uTruths, p->nVarsMax, pCut->nLeaves, uTruth, uPhases, uCanon );
        // check if this truth table was already encountered
        for ( i = 0; i < nCanons1; i++ )
            if ( uCanons1[i][0] == uCanon[0] && uCanons1[i][1] == uCanon[1] )
                break;
        if ( i != nCanons1 )
            continue;
        // did not find - add it to storage
        uCanons1[nCanons1][0] = uCanon[0];
        uCanons1[nCanons1][1] = uCanon[1];
        nCanons1++;

        // go through the matching supergates
        if ( pSuperList = Map_SuperTableLookupC( p->pSuperLib, uCanon ) )
        {
            // find the best match
            Resm_MatchCutSuper( pNode, pCut, pSuperList, uPhases, nPhases, 0 );
            if ( pCut->pSuperBest && Resm_MatchCompare( pMatchBest, pCut ) )
            {
                pMatchBest->fCompl     = pCut->fCompl;
                pMatchBest->aArea      = pCut->aArea;
                pMatchBest->tPrevious  = pCut->tPrevious; // required for this phase
                pMatchBest->tArrival   = pCut->tArrival;
                pMatchBest->pSuperBest = pCut->pSuperBest;
                pMatchBest->uPhase     = pCut->uPhase;
            }
            nSupers1++;
        }


        if ( fUsePhase )
        {
            // complement the truth table
            uTruth[0] = ~uTruth[0];
            uTruth[1] = ~uTruth[1];

            // compute the canonical form and canonical phases
            nPhases = Map_CanonComputeSlow( p->uTruths, p->nVarsMax, pCut->nLeaves, uTruth, uPhases, uCanon );
            // check if this truth table was already encountered
            for ( i = 0; i < nCanons0; i++ )
                if ( uCanons0[i][0] == uCanon[0] && uCanons0[i][1] == uCanon[1] )
                    break;
            if ( i != nCanons0 )
                continue;
            // did not find - add it to storage
            uCanons0[nCanons0][0] = uCanon[0];
            uCanons0[nCanons0][1] = uCanon[1];
            nCanons0++;

            // go through the matching supergates
            if ( pSuperList = Map_SuperTableLookupC( p->pSuperLib, uCanon ) )
            {
                // find the best match
                Resm_MatchCutSuper( pNode, pCut, pSuperList, uPhases, nPhases, 1 );
                if ( pCut->pSuperBest && Resm_MatchCompare( pMatchBest, pCut ) )
                {
                    pMatchBest->fCompl     = pCut->fCompl;
                    pMatchBest->aArea      = pCut->aArea;
                    pMatchBest->tPrevious  = pCut->tPrevious; // required for this phase
                    pMatchBest->tArrival   = pCut->tArrival;
                    pMatchBest->pSuperBest = pCut->pSuperBest;
                    pMatchBest->uPhase     = pCut->uPhase;
                }
                nSupers0++;
            }
        }
    }

    // assign the best match
    pCut->fCompl     = pMatchBest->fCompl;
    pCut->aArea      = pMatchBest->aArea;
    pCut->tPrevious  = pMatchBest->tPrevious;
    pCut->tArrival   = pMatchBest->tArrival;
    pCut->pSuperBest = pMatchBest->pSuperBest;
    pCut->uPhase     = pMatchBest->uPhase;

if ( p->fVerbose > 1 )
    printf( "dc = %3d  f = %3d  (c0 = %5d  s0 = %5d)  (c1 = %5d  s1 = %5d)\n", 
        nNumInit, nGens, nCanons0, nSupers0, nCanons1, nSupers1 );

    if ( pMatchBest->pSuperBest )
    {
if ( p->fVerbose > 1 )
{
    printf( "ORIGINAL  " ); Resm_NodePrint( pNode );
    printf( "RESULTING " ); Resm_NodePrint( pCut );
}
    }
}


/**Function*************************************************************

  Synopsis    [Find the best match among the set of supergates.]

  Description [The cut's match comes with the following two things assigned:
  the set of supergates (pMatch->pSuperList) and the matching phase (pMatch->uPhase).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Resm_MatchCutSuper( Resm_Node_t * pNode, Resm_Node_t * pCut, Map_Super_t * pSuperList, unsigned char uPhases[], int nPhases, int fCompl )
{
    Map_Super_t * pSuper;
    Resm_Node_t MatchBest, * pMatchBest = &MatchBest;
    unsigned char * uPhasesSuper;
    int i, k;

    // start the best match
    pMatchBest->fCompl     = pCut->fCompl;
    pMatchBest->aArea      = pCut->aArea;
    pMatchBest->tPrevious  = pCut->tPrevious;
    pMatchBest->tArrival   = pCut->tArrival;
    pMatchBest->pSuperBest = pCut->pSuperBest;
    pMatchBest->uPhase     = pCut->uPhase;
    pMatchBest->nLeaves    = pCut->nLeaves;

    pCut->fCompl          = fCompl;
    pCut->aArea           = RESM_FLOAT_LARGE;
    pCut->tPrevious       = pNode->tRequired[!fCompl];
    pCut->tArrival.Fall   = RESM_FLOAT_LARGE;
    pCut->tArrival.Rise   = RESM_FLOAT_LARGE;
    pCut->tArrival.Worst  = RESM_FLOAT_LARGE;
    pCut->pSuperBest      = NULL;
    pCut->uPhase          = 0;

    // go through the supergates
    for ( pSuper = pSuperList; pSuper; pSuper = Map_SuperReadNext(pSuper) )
    {
        if ( Resm_MatchSuperContainsInv( pSuper ) )
            continue;
        // assign the current match
        pCut->fCompl     = fCompl;
        pCut->pSuperBest = pSuper;
        uPhasesSuper = Map_SuperReadPhases(pCut->pSuperBest);
        for ( k = 0; k < (int)nPhases; k++ )
        {
            // find the overall phase of this match
            pCut->uPhase = uPhases[k] ^ uPhasesSuper[0];
            assert( pCut->uPhase < (unsigned)(1 << pCut->nLeaves) );

            // get the arrival time
            Resm_TimeCutComputeArrival( pCut, pNode->tRequired[!fCompl].Worst );
            // skip the cut if the arrival times exceed the required times
            if ( pCut->tArrival.Worst > pNode->tRequired[!fCompl].Worst + pNode->p->fEpsilon )
                continue;

            // set the phase polarity
            for ( i = 0; i < (int)pCut->nLeaves; i++ )
                pCut->ppLeaves[i] = Resm_NotCond( pCut->ppLeaves[i], ((pCut->uPhase & (1<<i)) > 0) );
            // get the area
            pCut->aArea = Resm_NodeGetAreaDerefed( pCut ) + 
                pSuper->Area + ((pNode->nRefs[fCompl] > 0) * pNode->p->AreaInv);
            // unset the phase polarity
            for ( i = 0; i < (int)pCut->nLeaves; i++ )
                pCut->ppLeaves[i] = Resm_Regular( pCut->ppLeaves[i] );

            // find the supergate with the minimum worst case arrival time
            if ( Resm_MatchCompare( pMatchBest, pCut ) )
            {
                pMatchBest->fCompl     = pCut->fCompl;
                pMatchBest->aArea      = pCut->aArea;
                pMatchBest->tPrevious  = pCut->tPrevious;
                pMatchBest->tArrival   = pCut->tArrival;
                pMatchBest->pSuperBest = pCut->pSuperBest;
                pMatchBest->uPhase     = pCut->uPhase;
            }
        }
    }
    // assign the best match
    pCut->fCompl     = pMatchBest->fCompl;
    pCut->aArea      = pMatchBest->aArea;
    pCut->tPrevious  = pMatchBest->tPrevious;
    pCut->tArrival   = pMatchBest->tArrival;
    pCut->pSuperBest = pMatchBest->pSuperBest;
    pCut->uPhase     = pMatchBest->uPhase;
}

/**Function*************************************************************

  Synopsis    [Compares two nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Resm_MatchCompare( Resm_Node_t * pM1, Resm_Node_t * pM2 )
{
    if ( !pM2->p->fDoingArea )
    {
        // compare the arrival times
//        if ( pM1->tArrival.Worst < pM2->tArrival.Worst - pM2->p->fEpsilon )
//            return 0;
//        if ( pM1->tArrival.Worst > pM2->tArrival.Worst + pM2->p->fEpsilon )
//            return 1;
        // compare the arrival times
        if ( pM1->tArrival.Worst - pM1->tPrevious.Worst < pM2->tArrival.Worst - pM2->tPrevious.Worst - pM2->p->fEpsilon )
            return 0;
        if ( pM1->tArrival.Worst - pM1->tPrevious.Worst > pM2->tArrival.Worst - pM2->tPrevious.Worst + pM2->p->fEpsilon )
            return 1;
        // compare the areas or area flows
        if ( pM1->aArea < pM2->aArea - pM2->p->fEpsilon )
            return 0;
        if ( pM1->aArea > pM2->aArea + pM2->p->fEpsilon )
            return 1;
        // compare the fanout limits
//        if ( Resm_SuperReadFanoutLimit(pM1->pSuperBest) > Resm_SuperReadFanoutLimit(pM2->pSuperBest) )
//            return 0;
//        if ( Resm_SuperReadFanoutLimit(pM1->pSuperBest) < Resm_SuperReadFanoutLimit(pM2->pSuperBest) )
//            return 1;
        // compare the number of leaves
//        if ( pM1->nLeaves < pM2->nLeaves )
//            return 0;
//        if ( pM1->nLeaves > pM2->nLeaves )
//            return 1;
        // otherwise prefer the old cut
//        return 0;
        return pM2->p->fAcceptMode == 1;
    }
    else
    {
        // compare the areas or area flows
        if ( pM1->aArea < pM2->aArea - pM2->p->fEpsilon )
            return 0;
        if ( pM1->aArea > pM2->aArea + pM2->p->fEpsilon )
            return 1;
        // compare the arrival times
//        if ( pM1->tArrival.Worst < pM2->tArrival.Worst - pM2->p->fEpsilon )
//            return 0;
//        if ( pM1->tArrival.Worst > pM2->tArrival.Worst + pM2->p->fEpsilon )
//            return 1;
        // compare the arrival times
        if ( pM1->tArrival.Worst - pM1->tPrevious.Worst < pM2->tArrival.Worst - pM2->tPrevious.Worst - pM2->p->fEpsilon )
            return 0;
        if ( pM1->tArrival.Worst - pM1->tPrevious.Worst > pM2->tArrival.Worst - pM2->tPrevious.Worst + pM2->p->fEpsilon )
            return 1;
        // compare the fanout limits
//        if ( Resm_SuperReadFanoutLimit(pM1->pSuperBest) > Resm_SuperReadFanoutLimit(pM2->pSuperBest) )
//            return 0;
//        if ( Resm_SuperReadFanoutLimit(pM1->pSuperBest) < Resm_SuperReadFanoutLimit(pM2->pSuperBest) )
//            return 1;
        // compare the number of leaves
//        if ( pM1->nLeaves < pM2->nLeaves )
//            return 0;
//        if ( pM1->nLeaves > pM2->nLeaves )
//            return 1;
        // otherwise prefer the old cut
//        return 0;
        return pM2->p->fAcceptMode == 1;
    }
}


/**Function*************************************************************

  Synopsis    [Computes the arrival times of the cut.]

  Description [Computes the arrival times of the cut if it is implemented using 
  the given supergate with the given phase. Uses the constraint-type specification
  of rise/fall arrival times.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Resm_TimeCutComputeArrival( Resm_Cut_t * pCut, float fWorstLimit )
{
    Resm_Time_t tArrIn, * ptArrIn = &tArrIn;
    Resm_Time_t * ptArrRes = &pCut->tArrival;
    Map_Super_t * pSuper = pCut->pSuperBest;
    unsigned uPhaseTot = pCut->uPhase;
    Resm_Node_t * pChild;
    bool fPinPhase;
    float tDelay;
    int i;

    ptArrRes->Rise  = ptArrRes->Fall = 0;
    ptArrRes->Worst = RESM_FLOAT_LARGE;
    for ( i = pCut->nLeaves - 1; i >= 0; i-- )
    {
        // get the given pin
        pChild = pCut->ppLeaves[i];        
        // get the phase of the given pin
        fPinPhase = ((uPhaseTot & (1 << i)) == 0);
        // get the arrival times of the given polarity
        if ( fPinPhase )
        {
            ptArrIn->Rise = pChild->tArrival.Rise;
            ptArrIn->Fall = pChild->tArrival.Fall;
        }
        else
        {
            ptArrIn->Rise = pChild->tArrival.Fall + pCut->p->tDelayInv.Rise;
            ptArrIn->Fall = pChild->tArrival.Rise + pCut->p->tDelayInv.Fall;
        }

        // get the rise of the output due to rise of the inputs
        if ( pSuper->tDelaysR[i].Rise > 0 )
        {
            tDelay = ptArrIn->Rise + pSuper->tDelaysR[i].Rise;
            if ( tDelay > fWorstLimit )
                return RESM_FLOAT_LARGE;
            if ( ptArrRes->Rise < tDelay )
                ptArrRes->Rise = tDelay;
        }

        // get the rise of the output due to fall of the inputs
        if ( pSuper->tDelaysR[i].Fall > 0 )
        {
            tDelay = ptArrIn->Fall + pSuper->tDelaysR[i].Fall;
            if ( tDelay > fWorstLimit )
                return RESM_FLOAT_LARGE;
            if ( ptArrRes->Rise < tDelay )
                ptArrRes->Rise = tDelay;
        }

        // get the fall of the output due to rise of the inputs
        if ( pSuper->tDelaysF[i].Rise > 0 )
        {
            tDelay = ptArrIn->Rise + pSuper->tDelaysF[i].Rise;
            if ( tDelay > fWorstLimit )
                return RESM_FLOAT_LARGE;
            if ( ptArrRes->Fall < tDelay )
                ptArrRes->Fall = tDelay;
        }

        // get the fall of the output due to fall of the inputs
        if ( pSuper->tDelaysF[i].Fall > 0 )
        {
            tDelay = ptArrIn->Fall + pSuper->tDelaysF[i].Fall;
            if ( tDelay > fWorstLimit )
                return RESM_FLOAT_LARGE;
            if ( ptArrRes->Fall < tDelay )
                ptArrRes->Fall = tDelay;
        }
    }
    // return the worst-case of rise/fall arrival times
    ptArrRes->Worst = RESM_MAX(ptArrRes->Rise, ptArrRes->Fall);
    return ptArrRes->Worst;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the supergate has invertor root.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Resm_MatchSuperContainsInv2( Map_Super_t * pSuper )
{
    return Map_SuperReadFaninNum(pSuper) == 1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the supergate contains inverter.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Resm_MatchSuperContainsInv( Map_Super_t * pSuper )
{
    Map_Super_t ** ppFaninSupers;
    int nFaninSupers, i;
    if ( Map_SuperReadRoot(pSuper) == NULL )
        return 0;
    nFaninSupers  = Map_SuperReadFaninNum( pSuper );
    if ( nFaninSupers == 1 )
        return 1;
    ppFaninSupers = Map_SuperReadFanins( pSuper );
    for ( i = 0; i < nFaninSupers; i++ )
        if ( Resm_MatchSuperContainsInv( ppFaninSupers[i] ) )
            return 1;
    return 0;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



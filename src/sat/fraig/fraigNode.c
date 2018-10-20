/**CFile****************************************************************

  FileName    [fraigNode.c]

  PackageName [FRAIG: Functionally reduced AND-INV graphs.]

  Synopsis    [Implementation of the FRAIG node.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.1. Started - October 1, 2004]

  Revision    [$Id: fraigNode.c,v 1.3 2005/07/08 01:01:32 alanmi Exp $]

***********************************************************************/

#include "fraigInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates the constant 1 node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_Node_t * Fraig_NodeCreateConst( Fraig_Man_t * p )
{
    Fraig_Node_t * pNode;

    // create the node
    pNode = (Fraig_Node_t *)Fraig_MemFixedEntryFetch( p->mmNodes );
    memset( pNode, 0, sizeof(Fraig_Node_t) );

    // assign the number and add to the array of nodes
    pNode->Num   = p->vNodes->nSize;
    Fraig_NodeVecPush( p->vNodes,  pNode );
    pNode->NumPi = -1;  // this is not a PI, so its number is -1
    pNode->Level =  0;  // just like a PI, it has 0 level
    pNode->nRefs =  1;  // it is a persistent node, which comes referenced
    pNode->fInv  =  1;  // the simulation info is complemented

    // create the simulation info
    pNode->pSimsR = (Fraig_Sims_t *)Fraig_MemFixedEntryFetch( p->mmSims );
    pNode->pSimsD = (Fraig_Sims_t *)Fraig_MemFixedEntryFetch( p->mmSims );
    memset( pNode->pSimsR, 0, sizeof(Fraig_Sims_t) );
    memset( pNode->pSimsD, 0, sizeof(Fraig_Sims_t) );

    // count the number of ones in the simulation vector
    pNode->nOnes = FRAIG_SIM_ROUNDS * sizeof(unsigned) * 8;

    // insert it into the hash table
    Fraig_HashTableLookupF0( p, pNode );
    return pNode;
}

/**Function*************************************************************

  Synopsis    [Creates a primary input node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_Node_t * Fraig_NodeCreatePi( Fraig_Man_t * p )
{
    Fraig_Node_t * pNode, * pNodeRes;
    Fraig_Sims_t * pSims;
    int i, clk;

    // create the node
    pNode = (Fraig_Node_t *)Fraig_MemFixedEntryFetch( p->mmNodes );
    memset( pNode, 0, sizeof(Fraig_Node_t) );
    pNode->pSimsR = (Fraig_Sims_t *)Fraig_MemFixedEntryFetch( p->mmSims );
    pNode->pSimsD = (Fraig_Sims_t *)Fraig_MemFixedEntryFetch( p->mmSims );
    memset( pNode->pSimsD, 0, sizeof(Fraig_Sims_t) );

    // assign the number and add to the array of nodes
    pNode->Num   = p->vNodes->nSize;
    Fraig_NodeVecPush( p->vNodes,  pNode );

    // assign the PI number and add to the array of primary inputs
    pNode->NumPi = p->vInputs->nSize;   
    Fraig_NodeVecPush( p->vInputs, pNode );

    pNode->Level =  0;  // PI has 0 level
    pNode->nRefs =  1;  // it is a persistent node, which comes referenced
    pNode->fInv  =  0;  // the simulation info of the PI is not complemented

    // derive the simulation info for the new node
clk = clock();
    pSims = pNode->pSimsR;
    // set the random simulation info for the primary inputs
    pSims->uHash = 0;
    for ( i = 0; i < FRAIG_SIM_ROUNDS; i++ )
    {
        // generate the simulation info
        pSims->uTests[i] = FRAIG_RANDOM_UNSIGNED;
/*
        pSims->uTests[i] = p->pVarSims[pNode->NumPi % 12]->uTests[i];
        if ( i == 1 )
            pSims->uTests[i] = FRAIG_RANDOM_UNSIGNED;
*/
/*
        if ( i < FRAIG_SIM_ROUNDS / 2 )
            pSims->uTests[i] = p->pVarSims[pNode->NumPi % 12]->uTests[i];
        else
            pSims->uTests[i] = FRAIG_RANDOM_UNSIGNED;
*/
        // compute the hash key
        pSims->uHash ^= pSims->uTests[i] * s_FraigPrimes[i];
    }
    // count the number of ones in the simulation vector
    pNode->nOnes = Fraig_BitStringCountOnes( pSims->uTests, FRAIG_SIM_ROUNDS );

    // set the second simulation info
    pSims = pNode->pSimsD;
    // set the random simulation info for the primary inputs
    pSims->uHash = 0;
    for ( i = 0; i < p->iWordStart; i++ )
    {
        // generate the simulation info
        pSims->uTests[i] = FRAIG_RANDOM_UNSIGNED;
        // compute the hash key
        pSims->uHash ^= pSims->uTests[i] * s_FraigPrimes[i];
    }
p->timeSims += clock() - clk;

    // insert it into the hash table
    pNodeRes = Fraig_HashTableLookupF( p, pNode );
    assert( pNodeRes == NULL );
    // add to the runtime of simulation
    return pNode;
}

/**Function*************************************************************

  Synopsis    [Creates a new node.]

  Description [This procedure should be called to create the constant
  node and the PI nodes first.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_Node_t * Fraig_NodeCreate( Fraig_Man_t * p, Fraig_Node_t * p1, Fraig_Node_t * p2 )
{
    Fraig_Node_t * pNode;
    int clk;

    // create the node
    pNode = (Fraig_Node_t *)Fraig_MemFixedEntryFetch( p->mmNodes );
    memset( pNode, 0, sizeof(Fraig_Node_t) );

    // assign the children
    pNode->p1  = p1;  Fraig_Ref(p1);  Fraig_Regular(p1)->nRefs++;
    pNode->p2  = p2;  Fraig_Ref(p2);  Fraig_Regular(p2)->nRefs++;

    // assign the number and add to the array of nodes
    pNode->Num = p->vNodes->nSize;
    Fraig_NodeVecPush( p->vNodes,  pNode );

    // assign the PI number
    pNode->NumPi = -1;

    // compute the level of this node
    pNode->Level = 1 + FRAIG_MAX(Fraig_Regular(p1)->Level, Fraig_Regular(p2)->Level);
    pNode->fInv  = Fraig_NodeIsSimComplement(p1) & Fraig_NodeIsSimComplement(p2);

    // derive the simulation info 
clk = clock();
    // allocate memory for the simulation info
    pNode->pSimsR = (Fraig_Sims_t *)Fraig_MemFixedEntryFetch( p->mmSims );
    pNode->pSimsD = (Fraig_Sims_t *)Fraig_MemFixedEntryFetch( p->mmSims );
    // derive random simulation info
    pNode->pSimsR->uHash = 0;
    Fraig_NodeSimulate( pNode, 0, FRAIG_SIM_ROUNDS, 1 );
    // derive dynamic simulation info
    pNode->pSimsD->uHash = 0;
    Fraig_NodeSimulate( pNode, 0, p->iWordStart, 0 );
    // count the number of ones in the real simulation info
    pNode->nOnes = Fraig_BitStringCountOnes( pNode->pSimsR->uTests, FRAIG_SIM_ROUNDS );
    if ( pNode->fInv )
        pNode->nOnes = FRAIG_SIM_ROUNDS * 32 - pNode->nOnes;
    // add to the runtime of simulation
p->timeSims += clock() - clk;

#ifdef FRAIG_ENABLE_FANOUTS
    // create the fanout info
    Fraig_NodeAddFaninFanout( Fraig_Regular(p1), pNode );
    Fraig_NodeAddFaninFanout( Fraig_Regular(p2), pNode );
#endif
    return pNode;
}


/**Function*************************************************************

  Synopsis    [Simulates the node.]

  Description [Simulates the random or dynamic simulation info through 
  the node. Uses phases of the children to determine their real simulation
  info. Uses phase of the node to determine the way its simulation info 
  is stored. The resulting info is guaranteed to be 0 for the first pattern.]
  
  SideEffects [This procedure modified the hash value of the simulation info.]

  SeeAlso     []

***********************************************************************/
void Fraig_NodeSimulate( Fraig_Node_t * pNode, int iWordStart, int iWordStop, int fUseRand )
{
    Fraig_Sims_t * pSims, * pSims1, * pSims2;
    int fCompl, fCompl1, fCompl2, i;

    assert( !Fraig_IsComplement(pNode) );

    // get hold of the simulation information
    pSims  = fUseRand? pNode->pSimsR : pNode->pSimsD;
    pSims1 = fUseRand? Fraig_Regular(pNode->p1)->pSimsR : Fraig_Regular(pNode->p1)->pSimsD;
    pSims2 = fUseRand? Fraig_Regular(pNode->p2)->pSimsR : Fraig_Regular(pNode->p2)->pSimsD;

    // get complemented attributes of the children using their random info
    fCompl  = pNode->fInv;
    fCompl1 = Fraig_NodeIsSimComplement(pNode->p1);
    fCompl2 = Fraig_NodeIsSimComplement(pNode->p2);

    // simulate
    if ( fCompl1 && fCompl2 )
    {
        if ( fCompl )
            for ( i = iWordStart; i < iWordStop; i++ )
            {
                pSims->uTests[i] = (pSims1->uTests[i] | pSims2->uTests[i]);
                pSims->uHash ^= pSims->uTests[i] * s_FraigPrimes[i];
            }
        else
            for ( i = iWordStart; i < iWordStop; i++ )
            {
                pSims->uTests[i] = ~(pSims1->uTests[i] | pSims2->uTests[i]);
                pSims->uHash ^= pSims->uTests[i] * s_FraigPrimes[i];
            }
    }
    else if ( fCompl1 && !fCompl2 )
    {
        if ( fCompl )
            for ( i = iWordStart; i < iWordStop; i++ )
            {
                pSims->uTests[i] = (pSims1->uTests[i] | ~pSims2->uTests[i]);
                pSims->uHash ^= pSims->uTests[i] * s_FraigPrimes[i];
            }
        else
            for ( i = iWordStart; i < iWordStop; i++ )
            {
                pSims->uTests[i] = (~pSims1->uTests[i] & pSims2->uTests[i]);
                pSims->uHash ^= pSims->uTests[i] * s_FraigPrimes[i];
            }
    }
    else if ( !fCompl1 && fCompl2 )
    {
        if ( fCompl )
            for ( i = iWordStart; i < iWordStop; i++ )
            {
                pSims->uTests[i] = (~pSims1->uTests[i] | pSims2->uTests[i]);
                pSims->uHash ^= pSims->uTests[i] * s_FraigPrimes[i];
            }
        else
            for ( i = iWordStart; i < iWordStop; i++ )
            {
                pSims->uTests[i] = (pSims1->uTests[i] & ~pSims2->uTests[i]);
                pSims->uHash ^= pSims->uTests[i] * s_FraigPrimes[i];
            }
    }
    else // if ( !fCompl1 && !fCompl2 )
    {
        if ( fCompl )
            for ( i = iWordStart; i < iWordStop; i++ )
            {
                pSims->uTests[i] = ~(pSims1->uTests[i] & pSims2->uTests[i]);
                pSims->uHash ^= pSims->uTests[i] * s_FraigPrimes[i];
            }
        else
            for ( i = iWordStart; i < iWordStop; i++ )
            {
                pSims->uTests[i] = (pSims1->uTests[i] & pSims2->uTests[i]);
                pSims->uHash ^= pSims->uTests[i] * s_FraigPrimes[i];
            }
    }
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


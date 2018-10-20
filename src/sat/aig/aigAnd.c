/**CFile****************************************************************

  FileName    [aigAnd.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [AND-INV graph package with built in sat_sweep.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - July 10, 2004.]

  Revision    [$Id: aigAnd.c,v 1.2 2004/07/29 04:54:47 alanmi Exp $]

***********************************************************************/

#include "aigInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// primes used to compute the hash function
#define MAX_AIG_PRIMES      304
static int s_AigPrimes[1100] =
{
//    2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37,  
    // commented it out to shift towards larger primes
    41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 
    97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 
    157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 
    227, 229, 233, 239, 241, 251, 257, 263, 269, 271, 277, 281, 
    283, 293, 307, 311, 313, 317, 331, 337, 347, 349, 353, 359, 
    367, 373, 379, 383, 389, 397, 401, 409, 419, 421, 431, 433, 
    439, 443, 449, 457, 461, 463, 467, 479, 487, 491, 499, 503, 
    509, 521, 523, 541, 547, 557, 563, 569, 571, 577, 587, 593, 
    599, 601, 607, 613, 617, 619, 631, 641, 643, 647, 653, 659, 
    661, 673, 677, 683, 691, 701, 709, 719, 727, 733, 739, 743, 
    751, 757, 761, 769, 773, 787, 797, 809, 811, 821, 823, 827, 
    829, 839, 853, 857, 859, 863, 877, 881, 883, 887, 907, 911, 
    919, 929, 937, 941, 947, 953, 967, 971, 977, 983, 991, 997, 
    1009, 1013, 1019, 1021, 1031, 1033, 1039, 1049, 1051, 1061, 1063, 1069, 
    1087, 1091, 1093, 1097, 1103, 1109, 1117, 1123, 1129, 1151, 1153, 1163, 
    1171, 1181, 1187, 1193, 1201, 1213, 1217, 1223, 1229, 1231, 1237, 1249, 
    1259, 1277, 1279, 1283, 1289, 1291, 1297, 1301, 1303, 1307, 1319, 1321, 
    1327, 1361, 1367, 1373, 1381, 1399, 1409, 1423, 1427, 1429, 1433, 1439, 
    1447, 1451, 1453, 1459, 1471, 1481, 1483, 1487, 1489, 1493, 1499, 1511, 
    1523, 1531, 1543, 1549, 1553, 1559, 1567, 1571, 1579, 1583, 1597, 1601, 
    1607, 1609, 1613, 1619, 1621, 1627, 1637, 1657, 1663, 1667, 1669, 1693, 
    1697, 1699, 1709, 1721, 1723, 1733, 1741, 1747, 1753, 1759, 1777, 1783, 
    1787, 1789, 1801, 1811, 1823, 1831, 1847, 1861, 1867, 1871, 1873, 1877, 
    1879, 1889, 1901, 1907, 1913, 1931, 1933, 1949, 1951, 1973, 1979, 1987, 
    1993, 1997, 1999, 2003 
};

static Aig_Node_t *    Aig_NodeAndCanon( Aig_Man_t * p, Aig_Node_t * p1, Aig_Node_t * p2 );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates a new node.]

  Description [This procedure should be called to create the constant
  node and the PI nodes first.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Node_t * Aig_NodeCreate( Aig_Man_t * p, Aig_Node_t * p1, Aig_Node_t * p2 )
{
    Aig_Node_t * pNode;
    Aig_Sims_t * pSims, * pSims1, * pSims2;
    unsigned uNum1, uNum2;
    unsigned * pSupp1, * pSupp2;
    int fCompl1, fCompl2, i, clk;

    // create the node
    pNode = (Aig_Node_t *)Extra_MmFixedEntryFetch( p->mmNodes );
    memset( pNode, 0, sizeof(Aig_Node_t) );
    pNode->p1  = p1; 
    pNode->p2  = p2;
    pNode->pSims = (Aig_Sims_t *)Extra_MmFixedEntryFetch( p->mmSims );
    pNode->pSupp = (unsigned *)Extra_MmFixedEntryFetch( p->mmSupps );
    // set the number of this node
    pNode->Num = p->pVec->nSize;
    Aig_NodeVecPush( p->pVec, pNode );
    // reference the inputs (will be used to compute the number of fanouts)
    if ( p1 ) Aig_NodeRef(p1);
    if ( p2 ) Aig_NodeRef(p2);

    // derive the simulation info for the new node
clk = clock();
    pSims = pNode->pSims;
    if ( p1 )
    {
        // get the pointers to the simulation info of the children
        pSims1  = Aig_Regular(p1)->pSims;
        pSims2  = Aig_Regular(p2)->pSims;
        // determine whether the simulation info is complemented
        // by looking at the complemented attribute of the node
        // and the complemented attribute of the simulation info
        fCompl1 = (Aig_IsComplement(p1) ^ pSims1->fInv);
        fCompl2 = (Aig_IsComplement(p2) ^ pSims2->fInv);
        for ( i = 0; i < NSIMS; i++ )
        {
            uNum1 = fCompl1? ~pSims1->uTests[i] : pSims1->uTests[i];
            uNum2 = fCompl2? ~pSims2->uTests[i] : pSims2->uTests[i];
            pSims->uTests[i] = uNum1 & uNum2;
        }
        // compute the support info
        pSupp1 = Aig_Regular(p1)->pSupp;
        pSupp2 = Aig_Regular(p2)->pSupp;
        for ( i = 0; i < p->nSuppWords; i++ )
            pNode->pSupp[i] = pSupp1[i] | pSupp2[i]; 
        // add the clauses to the solver
        Sat_SolverAddVar( p->pSat );
        Aig_NodeAddClauses( p, pNode, 0 );
    }
    else
    {
        // set the random simulation info for the primary inputs
        for ( i = 0; i < NSIMS; i++ )
            pSims->uTests[i] = ((rand() << 16) | rand()) * rand();
        // set the support info
        for ( i = 0; i < p->nSuppWords; i++ )
            pNode->pSupp[i] = 0;
        Aig_NodeSetVar(pNode,pNode->Num-1);
        // add new variable to the solver
        Sat_SolverAddVar( p->pSat );
    }

    // set the complemented attribute of the simulation info
    // this is necessary to make sure that the node and its complement
    // hash into the same function, although their simulation info is complemented

    // we require that the first bit of the first vector is 0
    // if it is 1, the whole simulation info is complemented
    pSims->fInv = (pSims->uTests[0] & 1);
    pSims->uHash = 0;
    if ( pSims->fInv )
    {
        for ( i = 0; i < NSIMS; i++ )
        {
            pSims->uTests[i] = ~pSims->uTests[i];
            pSims->uHash += pSims->uTests[i] * s_AigPrimes[i];
        }
    }
    else
    {
        for ( i = 0; i < NSIMS; i++ )
            pSims->uHash += pSims->uTests[i] * s_AigPrimes[i];
    }
    // add to the runtime of simulation
p->timeSims += clock() - clk;
    return pNode;
}

/**Function*************************************************************

  Synopsis    [The internal AND operation on two nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Node_t * Aig_NodeAndCanon( Aig_Man_t * pMan, Aig_Node_t * p1, Aig_Node_t * p2 )
{
    Aig_Node_t * pNodeNew, * pNodeClass, * pNodeTemp;

    // check for trivial cases
    if ( p1 == p2 )
        return p1;
    if ( p1 == Aig_Not(p2) )
        return Aig_Not(pMan->pConst1);
    if ( Aig_NodeIsConst(p1) )
    {
        if ( p1 == pMan->pConst1 )
            return p2;
        return Aig_Not(pMan->pConst1);
    }
    if ( Aig_NodeIsConst(p2) )
    {
        if ( p2 == pMan->pConst1 )
            return p1;
        return Aig_Not(pMan->pConst1);
    }

    // perform level-one structural hashing
    if ( Aig_HashTableLookupS( pMan, pMan->pTableS, p1, p2, &pNodeNew ) ) // the same node is found
        return pNodeNew;
    // the same node is not found, but the new one is created

    // if one level hashing is requested (without functionality hashing), return
    if ( pMan->fOneLevel )
        return pNodeNew;

    // check if the new node is unique using the simulation info
    pNodeClass = Aig_HashTableLookupF( pMan->pTableF, pNodeNew );
    if ( pNodeClass == NULL ) // the node is unique
        return pNodeNew;
    // there are other nodes, which look the same according to simulation
    // these nodes are linked into the linked list pNodeClass->pDiff

    // use SAT to resolve the ambiguity
    for ( pNodeTemp = pNodeClass; pNodeTemp; pNodeTemp = pNodeTemp->pDiff )
    {
        bool fCompl = (pNodeTemp->pSims->fInv ^ pNodeNew->pSims->fInv);
        if ( Aig_NodeIsEquivalent( pMan, pNodeTemp, pNodeNew, fCompl, 1, 0 ) )
        {
            // set the node to be equivalent with this node
            pNodeNew->pEqu = pNodeTemp->pEqu;
            pNodeTemp->pEqu = pNodeNew;
            // return the equivalent node
            return Aig_NotCond( pNodeTemp, fCompl );
        }
    }
    // we tried all nodes in this simulation class and did not find equivalent ones

    // now we add another member to this class
    pNodeNew->pDiff = pNodeClass->pDiff;
    pNodeClass->pDiff = pNodeNew;
    // return the new node
    return pNodeNew;
}


/**Function*************************************************************

  Synopsis    [Perfoms the AND operation with functional hashing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Node_t * Aig_NodeAnd( Aig_Man_t * p, Aig_Node_t * p1, Aig_Node_t * p2 )
{
    Aig_Node_t * pNode;
    pNode = Aig_NodeAndCanon( p, p1, p2 );       Aig_NodeRef( pNode );
    return pNode;
}

/**Function*************************************************************

  Synopsis    [Perfoms the OR operation with functional hashing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Node_t * Aig_NodeOr( Aig_Man_t * p, Aig_Node_t * p1, Aig_Node_t * p2 )
{
    Aig_Node_t * pNode;
    pNode = Aig_Not( Aig_NodeAndCanon( p, Aig_Not(p1), Aig_Not(p2) ) );   Aig_NodeRef( pNode );
    return pNode;
}

/**Function*************************************************************

  Synopsis    [Perfoms the EXOR operation with functional hashing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Node_t * Aig_NodeExor( Aig_Man_t * p, Aig_Node_t * p1, Aig_Node_t * p2 )
{
    return Aig_NodeMux( p, p1, Aig_Not(p2), p2 );
}

/**Function*************************************************************

  Synopsis    [Perfoms the MUX operation with functional hashing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Node_t * Aig_NodeMux( Aig_Man_t * p, Aig_Node_t * pC, Aig_Node_t * pT, Aig_Node_t * pE )
{
    Aig_Node_t * pAnd1, * pAnd2, * pRes;
    pAnd1 = Aig_NodeAndCanon( p, pC,          pT );   Aig_NodeRef( pAnd1 );
    pAnd2 = Aig_NodeAndCanon( p, Aig_Not(pC), pE );   Aig_NodeRef( pAnd2 );
    pRes  = Aig_NodeOr( p, pAnd1, pAnd2 );                 
    return pRes;
}




////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



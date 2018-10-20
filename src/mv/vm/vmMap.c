/**CFile****************************************************************

  FileName    [vmMap.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Procedures to manipulate variable maps.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: vmMap.c,v 1.9 2003/09/01 04:56:58 alanmi Exp $]

***********************************************************************/

#include "vmInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define MAX_PRIMES      304

static int s_Primes[MAX_PRIMES+500] =
{
    2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 
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



////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Lookup if the given variable map exists in the manager.]

  Description [The resulting map is not referenced.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vm_VarMap_t * Vm_VarMapLookup( Vm_Manager_t * p, int nVarsIn, int nVarsOut, int * pValues )
{
    Vm_VarMap_t * pMap;
    unsigned Key;
    int nVars;

    nVars = nVarsIn + nVarsOut;
    Key = Vm_VarMapHash( nVars, pValues ) % p->nTableSize;
    for ( pMap = p->pTable[Key]; pMap; pMap = pMap->pNext )
    {
        if ( Vm_VarMapCompare( pMap, nVarsIn, nVarsOut, pValues ) )
        {
            p->nMapsUsed++;
            return pMap;
        }
    }
    // create the new map
    pMap = Vm_VarMapCreate( p, nVarsIn, nVarsOut, pValues );
    // insert the new map in the table
    pMap->pNext    = p->pTable[Key];
    p->pTable[Key] = pMap;
    // increment the counter of maps and relations
    p->nMaps++;
    p->nMapsUsed++;
    return pMap;
}

/**Function*************************************************************

  Synopsis    [Create variable map for the give set of MV variables.]

  Description [The first argument is the variable map manager, in which
  the variable map will be created. The second argument is the number of 
  all variblest. The array pValues contains nVars entries. In the array,
  there are the number of values for each input variable followed by the
  numbers of values for each output variable.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vm_VarMap_t * Vm_VarMapCreate( Vm_Manager_t * pMan, int nVarsIn, int nVarsOut, int * pValues )
{
    Vm_VarMap_t * pMap;
    int nVars, nValuesTotal;
    int i;

    // allocate the map structure
    nVars = nVarsIn + nVarsOut;
    pMap = ALLOC( Vm_VarMap_t, 1 );
    memset( pMap, 0, sizeof(Vm_VarMap_t) );
    pMap->pValues      = ALLOC( int, nVars  );
    pMap->pValuesFirst = ALLOC( int, nVars + 1 );

    // assign internal var map data
    pMap->pMan            = pMan;
    pMap->nRefs           = 0;
    pMap->nVarsIn         = nVarsIn;
    pMap->nVarsOut        = nVarsOut;
    pMap->pValuesFirst[0] = 0;
    nValuesTotal          = 0;
    for ( i = 0; i < nVars; i++ )
    {
        // mark-up the variables
        pMap->pValues[i]           = pValues[i];
        if ( i < nVarsIn )
            pMap->nValuesIn  += pValues[i];
        else
            pMap->nValuesOut += pValues[i];
        nValuesTotal              += pValues[i];
        pMap->pValuesFirst[i+1]    = nValuesTotal;
    }

    // check if the internal storage in the manager needs extending
    if ( pMan->nValuesMax < nValuesTotal )
    {
        // set the new size
        pMan->nValuesMax = nValuesTotal;
        // reallocate the var-num-sized arrays in the manager
        pMan->pSupport1 = REALLOC( int, pMan->pSupport1, 2 * pMan->nValuesMax );
        pMan->pSupport2 = REALLOC( int, pMan->pSupport2, 2 * pMan->nValuesMax );
        pMan->pPermute  = REALLOC( int, pMan->pPermute,  2 * pMan->nValuesMax );
        pMan->pArray1   = REALLOC( int, pMan->pArray1,   2 * pMan->nValuesMax );
        pMan->pArray2   = REALLOC( int, pMan->pArray2,   2 * pMan->nValuesMax );
    }
    return pMap;
}

/**Function*************************************************************

  Synopsis    [Delete variable map with all the internal data.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Vm_VarMapFree( Vm_VarMap_t * pMap )
{
    // make sure that the map is not used
//    assert( pMap->nRefs == 0 );
    free( pMap->pValuesFirst );
    free( pMap->pValues );
    free( pMap );
}

/**Function*************************************************************

  Synopsis    [Computes the hash value for the variable map.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Vm_VarMapHash( int nVars, int * pValues )
{
    unsigned Key;
    int i;
    Key = s_Primes[99] * nVars;
    for ( i = 0; i < nVars; i++ )
    {
        assert( pValues[i] > 1 );
        Key ^= s_Primes[i+100] * pValues[i];
    }
    return Key;
}

/**Function*************************************************************

  Synopsis    [Compares to variable maps.]

  Description [Returns 1, of the maps are identical.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Vm_VarMapCompare( Vm_VarMap_t * pMap, int nVarsIn, int nVarsOut, int * pValues )
{
    int i;
    if ( nVarsIn != pMap->nVarsIn )
        return 0;
    if ( nVarsOut != pMap->nVarsOut )
        return 0;
    for ( i = 0; i < nVarsIn + nVarsOut; i++ )
        if ( pMap->pValues[i] != pValues[i] )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Increments the ref counter of the variable map.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vm_VarMap_t * Vm_VarMapRef( Vm_VarMap_t * pMap )
{
    pMap->nRefs++;
    return pMap;
}


/**Function*************************************************************

  Synopsis    [Decrements the ref counter of the variable map.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vm_VarMap_t * Vm_VarMapDeref( Vm_VarMap_t * pMap )
{
    pMap->nRefs--;
//    assert( pMap->nRefs >= 0 );
    return pMap;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



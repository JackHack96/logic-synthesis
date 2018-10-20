/**CFile****************************************************************

  FileName    [vmxMap.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Procedures to manipulate variable maps.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: vmxMap.c,v 1.12 2003/11/18 18:55:07 alanmi Exp $]

***********************************************************************/

#include "vmxInt.h"

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

  Synopsis    [Create a new Vmx or return the available one if it exists.]

  Description [The arguments of this lookup procedure are the Vmx manager (pMan),
  the MV variable map (pVm), the number of bits in the new variable map (bBits),
  and the ordering of bits (pBitsOrder). The last argument (pBitsOrder) can be
  NULL, if the trivial variable order is assumed. The resulting map is returned 
  from the hash table if it exits there (ref counter is incremented), or created 
  if it is the first time that such map is needed. The resulting map is not
  referenced.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vmx_VarMap_t * Vmx_VarMapLookup( Vmx_Manager_t * pMan, Vm_VarMap_t * pVm, int nBits, int * pBitsOrder )
{
	Vmx_VarMap_t * pVmx, * pVmxNew;
    unsigned Key;
	int i, nVars;

	// if bit order is not given, a simple Vmx is assumed
	// create the trival variable map
	if ( pBitsOrder == NULL )
	{ 
		// count the number of binary bits
		nBits = 0;
        nVars = pVm->nVarsIn + pVm->nVarsOut;
		for ( i = 0; i < nVars; i++ )
			nBits += Extra_Base2Log(pVm->pValues[i]);
        assert( nBits >= nVars );
		// create the new bit order array
		pBitsOrder = Vm_VarMapGetStorageArray1( pVm );
		for ( i = 0; i < nBits; i++ )
			pBitsOrder[i] = i;
	}

    Key = Vmx_VarMapHash( pVm, nBits, pBitsOrder ) % pMan->nTableSize;
    for ( pVmx = pMan->pTable[Key]; pVmx; pVmx = pVmx->pNext )
    {
        if ( Vmx_VarMapCompare( pVmx, pVm, nBits, pBitsOrder ) )
        { // the map is found!
            pMan->nMapsUsed++;
            // return the existent map
            return pVmx;
        }
    }

	// create the new variable map
    pVmxNew = Vmx_VarMapCreate( pMan, pVm, nBits, pBitsOrder );
    // insert the new map in the table
    pVmxNew->pNext = pMan->pTable[Key];
    pMan->pTable[Key] = pVmxNew;
    // increment the counter of maps
    pMan->nMaps++;
    pMan->nMapsUsed++;
    // return the new map
    return pVmxNew;
}


/**Function*************************************************************

  Synopsis    [Create Vmx for the given Vm with the given BDD variable mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vmx_VarMap_t * Vmx_VarMapCreate( Vmx_Manager_t * pMan, Vm_VarMap_t * pVm, int nBits, int * pBitsOrder )
{
    Vmx_VarMap_t * pVmx;
    int i, nVars;

    // allocate the map structure
    pVmx = ALLOC( Vmx_VarMap_t, 1 );
    memset( pVmx, 0, sizeof(Vmx_VarMap_t) );
    pVmx->pBits      = ALLOC( int, pVm->nVarsIn + pVm->nVarsOut );
    pVmx->pBitsFirst = ALLOC( int, pVm->nVarsIn + pVm->nVarsOut + 1 );
    pVmx->pBitsOrder = ALLOC( int, nBits );
    // set the variable permutation
    for ( i = 0; i < nBits; i++ )
        pVmx->pBitsOrder[i] = pBitsOrder[i];

    // assign internal data
    pVmx->pMan          = pMan;
    pVmx->pVm           = pVm;
    pVmx->nRefs         = 0;
    pVmx->pBitsFirst[0] = 0;
    nVars = pVm->nVarsIn + pVm->nVarsOut;
    for ( i = 0; i < nVars; i++ )
    {
        // mark-up the variables
        pVmx->pBits[i]        = Extra_Base2Log( pVm->pValues[i] );
        pVmx->nBits          += pVmx->pBits[i];
        pVmx->pBitsFirst[i+1] = pVmx->nBits;
    }
    // consistency check: the number of bits is correct
    assert( pVmx->nBits == nBits );

    // check if the storage needs extending
    if ( pMan->nBitsMax < pVmx->nBits )
    {
        // set the new size
        pMan->nBitsMax = pVmx->nBits;
        // reallocate the var-num-sized arrays in the manager
        free( pMan->pArray );
        pMan->pArray   = ALLOC( int, pMan->nBitsMax );
    }
    if ( pMan->nValuesMax < pVm->nValuesIn + pVm->nValuesOut )
    {
        // set the new size
        pMan->nValuesMax = pVm->nValuesIn + pVm->nValuesOut;
        // reallocate the var-num-sized arrays in the manager
        free( pMan->pbCodes );
        pMan->pbCodes  = ALLOC( DdNode *, pMan->nValuesMax + 1 ); 
		memset( pMan->pbCodes, 0, sizeof(int) * (pMan->nValuesMax + 1) );
    }
    return pVmx;
}

/**Function*************************************************************

  Synopsis    [Delete variable map with all the associated data.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Vmx_VarMapFree( Vmx_VarMap_t * pVmx )
{
    // make sure that the map is not used
//    assert( pVmx->nRefs == 0 );
    free( pVmx->pBitsOrder );
    free( pVmx->pBitsFirst );
    free( pVmx->pBits );
    free( pVmx );
}

/**Function*************************************************************

  Synopsis    [Computes the hash value for the variable map.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Vmx_VarMapHash( Vm_VarMap_t * pVm, int nBits, int * pBitsOrder )
{
    unsigned Key;
    int i;
    Key = (unsigned)pVm;
    for ( i = 0; i < nBits; i++ )
    {
        assert( pBitsOrder[i] >= 0 );
        Key ^= s_Primes[i+100] * pBitsOrder[i];
    }
    return Key;
}

/**Function*************************************************************

  Synopsis    [Compares to variable maps.]

  Description [Returns 1, of the maps are identical.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Vmx_VarMapCompare( Vmx_VarMap_t * pVmx, Vm_VarMap_t * pVm, int nBits, int * pBitsOrder )
{
    int i;
    if ( pVmx->pVm != pVm )
        return 0;
    if ( pVmx->nBits != nBits )
        return 0;
    for ( i = 0; i < nBits; i++ )
        if ( pVmx->pBitsOrder[i] != pBitsOrder[i] )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Increments the ref counter of the variable map.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vmx_VarMap_t * Vmx_VarMapRef( Vmx_VarMap_t * pVmx )
{
    pVmx->nRefs++;
    return pVmx;
}


/**Function*************************************************************

  Synopsis    [Decrements the ref counter of the variable map.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vmx_VarMap_t * Vmx_VarMapDeref( Vmx_VarMap_t * pVmx )
{
    pVmx->nRefs--;
//    assert( pVmx->nRefs >= 0 );
    return pVmx;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



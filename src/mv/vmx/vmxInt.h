/**CFile****************************************************************

  FileName    [vmxInt.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Declarations of the extended variable map (VMX) package.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: vmxInt.h,v 1.9 2003/09/01 04:57:00 alanmi Exp $]

***********************************************************************/

#ifndef __VMX_INT_H__
#define __VMX_INT_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "mv.h"
#include "vmInt.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

struct VmxManagerStruct
{
    // the reference counter of the manager (when used with multiple networks)
    int                 nRefs;        // the reference counter
    // cache of available variable maps
    Vmx_VarMap_t **     pTable;       // the table of variable maps
    int                 nTableSize;   // the table size
    // manager utilization statistics
    int                 nMaps;        // the number of actual variable maps
    int                 nMapsUsed;    // the number of times these variable maps are used (pMaps <= pMapsUsed)
    // temporary storage for BDD cubes
    int                 nValuesMax;   // the largest number of all values
    DdNode **           pbCodes;      // the cubes for each value of each variable
    // temporary storage for the logarithmically encoded cube
    int                 nBitsMax;     // the largest number of all bits
    int *               pArray;       // temporary array for future use
};

struct VmxVarMapStruct
{
    // general information about the variable map    
    Vmx_Manager_t *     pMan;         // the manager this map belongs to 
    Vm_VarMap_t *       pVm;          // the parent VM map
    int                 nRefs;        // reference counter of this map
    // information about variables and values
    int *               pBits;        // the number of binary bits to encode one MV variable
    int *               pBitsFirst;   // the first bit encoding this MV variable
    int                 nBits;        // the total number of bits
    int *               pBitsOrder;   // the ordering of bits
    // connections in cache
    Vmx_VarMap_t *      pNext;        // the next variable map in the cache
};


////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/*=== vmxMap.c =======================================================*/
extern Vmx_VarMap_t * Vmx_VarMapCreate( Vmx_Manager_t * pMan, Vm_VarMap_t * pVmxMv, int nBits, int * pBitsOrder );
extern void           Vmx_VarMapFree( Vmx_VarMap_t * pVmx );
extern unsigned       Vmx_VarMapHash( Vm_VarMap_t * pVm, int nBits, int * pBitsOrder );
extern int            Vmx_VarMapCompare( Vmx_VarMap_t * pVmx, Vm_VarMap_t * pVm, int nBits, int * pBitsOrder );

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

#endif

/**CFile****************************************************************

  FileName    [vmInt.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Declarations of the variable map (VM) package.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: vmInt.h,v 1.9 2003/09/01 04:56:57 alanmi Exp $]

***********************************************************************/

#ifndef __VM_INT_H__
#define __VM_INT_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "mv.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

struct VmManagerStruct
{
    // the reference counter of the manager (when used with multiple networks)
    int                 nRefs;        // the reference counter
    // cache of available variable maps
    Vm_VarMap_t **      pTable;       // the table of variable maps
    int                 nTableSize;   // the table size
    // manager utilization statistics
    int                 nMaps;        // the number of actual variable maps
    int                 nMapsUsed;    // the number of times these variable maps are used (pMaps <= pMapsUsed)
    // temporary storage
    int                 nValuesMax;   // the largest sum total of values over all maps
    int *               pSupport1;    // the temporary array for variable support
    int *               pSupport2;    // the temporary array for variable support
	int *               pPermute;     // the temporary array for variable permutation
    int *               pArray1;      // temporary array for future use
    int *               pArray2;      // temporary array for future use
};

struct VmVarMapStruct
{
    // the manager this map belongs to 
    Vm_Manager_t *      pMan;
    // reference counter for this varmap
    int                 nRefs;
    // information about variables and values
    int                 nVarsIn;      // the number of input variable
    int                 nVarsOut;     // the number of output variable
    int                 nValuesIn;    // the total number of input values
    int                 nValuesOut;   // the total number of output values
    int *               pValues;      // the number of values all (input and output) variables
    int *               pValuesFirst; // the first value of each variable
    // connections in cache
    Vm_VarMap_t *       pNext;        // the next variable map in the cache
};

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/*=== vmMap.c =======================================================*/
extern Vm_VarMap_t * Vm_VarMapCreate( Vm_Manager_t * p, int nVarsIn, int nVarsOut, int * pValues );
extern void          Vm_VarMapFree( Vm_VarMap_t * p );
extern unsigned      Vm_VarMapHash( int nVars, int * pValues );
extern int           Vm_VarMapCompare( Vm_VarMap_t * pVm, int nVarsIn, int nVarsOut, int * pValues );

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

#endif

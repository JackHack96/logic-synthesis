/**CFile****************************************************************

  FileName    [mioInt.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [File reading/writing for technology mapping.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: mioInt.h,v 1.5 2005/01/23 06:59:45 alanmi Exp $]

***********************************************************************/

#ifndef __MIO_INT_H__
#define __MIO_INT_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "mv.h"
#include "mio.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

#define    MIO_STRING_GATE       "GATE"
#define    MIO_STRING_PIN        "PIN"
#define    MIO_STRING_NONINV     "NONINV"
#define    MIO_STRING_INV        "INV"
#define    MIO_STRING_UNKNOWN    "UNKNOWN"

#define    MIO_STRING_CONST0     "CONST0"
#define    MIO_STRING_CONST1     "CONST1"
 
// the bit masks
#define    MIO_MASK(n)         ((~((unsigned)0)) >> (32-(n)))
#define    MIO_FULL             (~((unsigned)0))

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

struct  Mio_LibraryStruct_t_
{
    char *             pName;       // the name of the library
    int                nGates;      // the number of the gates
    Mio_Gate_t *       pGates;      // the linked list of all gates in no particular order
    Mio_Gate_t *       pGateInv;    // the INV gate
    Mio_Gate_t *       pGateNand2;  // the NAND2 gate
    st_table *         tName2Gate;  // the mapping of gate names into their pointer
    DdManager *        dd;          // the nanager storing functions of gates
    Fnc_Manager_t *    pMan;        // the functionality manager
}; 

struct  Mio_GateStruct_t_
{
    // information derived from the genlib file
    char *             pName;       // the name of the gate
    double             dArea;       // the area of the gate
    char *             pForm;       // the formula describing functionality of the gate
    Mio_Pin_t *        pPins;       // the linked list of all pins (one pin if info is the same)
    char *             pOutName;    // the name of the output pin 
    // the library to which this gate belongs
    Mio_Library_t *    pLib; 
    // the next gate in the list
    Mio_Gate_t *       pNext;    

    // the derived information
    int                nInputs;     // the number of inputs
    double             dDelayMax;   // the maximum delay
    Mvc_Cover_t *      pMvc;        // the functionality
    Mvc_Cover_t *      pMvcC;       // the functionality
    DdNode *           bFunc;       // the functionality
};

struct  Mio_PinStruct_t_
{
    char *             pName;
    Mio_PinPhase_t     Phase;
    double             dLoadInput;
    double             dLoadMax;
    double             dDelayBlockRise;
    double             dDelayFanoutRise;
    double             dDelayBlockFall;
    double             dDelayFanoutFall;
    double             dDelayBlockMax;
    Mio_Pin_t *        pNext;     
};


////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

extern Mio_Library_t * s_pLib;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/*=== mio.c =============================================================*/
/*=== mioRead.c =============================================================*/
/*=== mioUtils.c =============================================================*/

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

#endif

/**CFile****************************************************************

  FileName    [mioApi.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [File reading/writing for technology mapping.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: mioApi.c,v 1.5 2005/01/23 06:59:45 alanmi Exp $]

***********************************************************************/

#include "mioInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char *            Mio_LibraryReadName          ( Mio_Library_t * pLib )  { return pLib->pName;    }
int               Mio_LibraryReadGateNum       ( Mio_Library_t * pLib )  { return pLib->nGates;   }
Mio_Gate_t *      Mio_LibraryReadGates         ( Mio_Library_t * pLib )  { return pLib->pGates;   }
DdManager *       Mio_LibraryReadDd            ( Mio_Library_t * pLib )  { return pLib->dd;       }
Mio_Gate_t *      Mio_LibraryReadInv           ( Mio_Library_t * pLib )  { return pLib->pGateInv; }
float             Mio_LibraryReadDelayInvRise  ( Mio_Library_t * pLib )  { return (float)pLib->pGateInv->pPins->dDelayBlockRise;   }
float             Mio_LibraryReadDelayInvFall  ( Mio_Library_t * pLib )  { return (float)pLib->pGateInv->pPins->dDelayBlockFall;   }
float             Mio_LibraryReadDelayInvMax   ( Mio_Library_t * pLib )  { return (float)pLib->pGateInv->pPins->dDelayBlockMax;    }
float             Mio_LibraryReadDelayNand2Rise( Mio_Library_t * pLib )  { return (float)pLib->pGateNand2->pPins->dDelayBlockRise; }
float             Mio_LibraryReadDelayNand2Fall( Mio_Library_t * pLib )  { return (float)pLib->pGateNand2->pPins->dDelayBlockFall; }
float             Mio_LibraryReadDelayNand2Max ( Mio_Library_t * pLib )  { return (float)pLib->pGateNand2->pPins->dDelayBlockMax;  }
float             Mio_LibraryReadAreaInv       ( Mio_Library_t * pLib )  { return (float)pLib->pGateInv->dArea;             }
float             Mio_LibraryReadAreaNand2     ( Mio_Library_t * pLib )  { return (float)pLib->pGateNand2->dArea;           }

/**Function*************************************************************

  Synopsis    [Read Mvc of the gate by name.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mio_Gate_t * Mio_LibraryReadGateByName( Mio_Library_t * pLib, char * pName )      
{ 
    Mio_Gate_t * pGate;
    if ( st_lookup( pLib->tName2Gate, pName, (char **)&pGate ) )
        return pGate;
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Read Mvc of the gate by name.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Cover_t * Mio_LibraryReadMvcByName( Mio_Library_t * pLib, char * pName )      
{ 
    Mio_Gate_t * pGate;
    if ( st_lookup( pLib->tName2Gate, pName, (char **)&pGate ) )
        return pGate->pMvc;
    return NULL;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char *            Mio_GateReadName    ( Mio_Gate_t * pGate )           { return pGate->pName;     }
char *            Mio_GateReadOutName ( Mio_Gate_t * pGate )           { return pGate->pOutName;  }
double            Mio_GateReadArea    ( Mio_Gate_t * pGate )           { return pGate->dArea;     }
char *            Mio_GateReadForm    ( Mio_Gate_t * pGate )           { return pGate->pForm;     }
Mio_Pin_t *       Mio_GateReadPins    ( Mio_Gate_t * pGate )           { return pGate->pPins;     }
Mio_Library_t *   Mio_GateReadLib     ( Mio_Gate_t * pGate )           { return pGate->pLib;      }
Mio_Gate_t *      Mio_GateReadNext    ( Mio_Gate_t * pGate )           { return pGate->pNext;     }
int               Mio_GateReadInputs  ( Mio_Gate_t * pGate )           { return pGate->nInputs;   }
double            Mio_GateReadDelayMax( Mio_Gate_t * pGate )           { return pGate->dDelayMax; }
Mvc_Cover_t *     Mio_GateReadMvc     ( Mio_Gate_t * pGate )           { return pGate->pMvc;      }
Mvc_Cover_t *     Mio_GateReadMvcC    ( Mio_Gate_t * pGate )           { return pGate->pMvcC;     }
DdNode *          Mio_GateReadFunc    ( Mio_Gate_t * pGate )           { return pGate->bFunc;     }

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char *            Mio_PinReadName           ( Mio_Pin_t * pPin )      { return pPin->pName;           }
Mio_PinPhase_t    Mio_PinReadPhase          ( Mio_Pin_t * pPin )      { return pPin->Phase;           }
double            Mio_PinReadInputLoad      ( Mio_Pin_t * pPin )      { return pPin->dLoadInput;      }
double            Mio_PinReadMaxLoad        ( Mio_Pin_t * pPin )      { return pPin->dLoadMax;        }
double            Mio_PinReadDelayBlockRise ( Mio_Pin_t * pPin )      { return pPin->dDelayBlockRise; }
double            Mio_PinReadDelayFanoutRise( Mio_Pin_t * pPin )      { return pPin->dDelayFanoutRise;}
double            Mio_PinReadDelayBlockFall ( Mio_Pin_t * pPin )      { return pPin->dDelayBlockFall; }
double            Mio_PinReadDelayFanoutFall( Mio_Pin_t * pPin )      { return pPin->dDelayFanoutFall;}
double            Mio_PinReadDelayBlockMax  ( Mio_Pin_t * pPin )      { return pPin->dDelayBlockMax;          }
Mio_Pin_t *       Mio_PinReadNext           ( Mio_Pin_t * pPin )      { return pPin->pNext;           }

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



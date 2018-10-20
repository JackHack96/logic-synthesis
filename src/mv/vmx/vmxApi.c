/**CFile****************************************************************

  FileName    [vmxApi.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [APIs of the variable map package.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: vmxApi.c,v 1.9 2003/09/01 04:56:59 alanmi Exp $]

***********************************************************************/

#include "vmxInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Basic APIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vmx_Manager_t * Vmx_VarMapReadMan( Vmx_VarMap_t * p )          { return p->pMan;       }
Vm_VarMap_t *   Vmx_VarMapReadVm( Vmx_VarMap_t * p )           { return p->pVm;        }
int *           Vmx_VarMapReadBits( Vmx_VarMap_t * p )         { return p->pBits;      }
int *           Vmx_VarMapReadBitsFirst( Vmx_VarMap_t * p )    { return p->pBitsFirst; }
int             Vmx_VarMapReadBitsNum( Vmx_VarMap_t * p )      { return p->nBits;      }
int *           Vmx_VarMapReadBitsOrder( Vmx_VarMap_t * p )    { return p->pBitsOrder; }


/**Function*************************************************************

  Synopsis    [Returns the number input bits.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Vmx_VarMapReadBitsInNum( Vmx_VarMap_t * p )
{
    Vm_VarMap_t * pVm;
    int * pBitsFirst;
    int nVarsIn;

    pVm        = Vmx_VarMapReadVm(p);
    pBitsFirst = Vmx_VarMapReadBitsFirst(p);
    nVarsIn    = Vm_VarMapReadVarsInNum(pVm);

    return nVarsIn;
}

/**Function*************************************************************

  Synopsis    [Returns the number input bits.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Vmx_VarMapReadBitsOutNum( Vmx_VarMap_t * p )
{
    Vm_VarMap_t * pVm;
    int * pBitsFirst;
    int nVarsIn;

    pVm        = Vmx_VarMapReadVm(p);
    pBitsFirst = Vmx_VarMapReadBitsFirst(p);
    nVarsIn    = Vm_VarMapReadVarsInNum(pVm);

    return Vmx_VarMapReadBitsNum(p) - pBitsFirst[nVarsIn];
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



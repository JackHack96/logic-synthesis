/**CFile****************************************************************

  FileName    [vmApi.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [APIs of the variable map package.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: vmApi.c,v 1.11 2003/09/01 04:56:57 alanmi Exp $]

***********************************************************************/

#include "vmInt.h"

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
Vm_Manager_t * Vm_VarMapReadMan( Vm_VarMap_t * p )               { return p->pMan; }
int            Vm_VarMapReadVarsInNum( Vm_VarMap_t * p )              { return p->nVarsIn; }
int            Vm_VarMapReadVarsOutNum( Vm_VarMap_t * p )             { return p->nVarsOut; }
int            Vm_VarMapReadVarsNum( Vm_VarMap_t * p )                { return p->nVarsIn + p->nVarsOut; }
int            Vm_VarMapReadValuesInNum( Vm_VarMap_t * p )            { return p->nValuesIn; }
int            Vm_VarMapReadValuesOutNum( Vm_VarMap_t * p )           { return p->nValuesOut; }
int            Vm_VarMapReadValuesNum( Vm_VarMap_t * p )              { return p->nValuesIn + p->nValuesOut; }
int            Vm_VarMapReadValues( Vm_VarMap_t * p, int iVar )       { return p->pValues[iVar]; }
int            Vm_VarMapReadValuesFirst( Vm_VarMap_t * p, int iVar )  { return p->pValuesFirst[iVar]; }
int            Vm_VarMapReadValuesOutput( Vm_VarMap_t * p )           { return p->pValues[p->nVarsIn]; }
int *          Vm_VarMapReadValuesArray( Vm_VarMap_t * p )            { return p->pValues; }
int *          Vm_VarMapReadValuesFirstArray( Vm_VarMap_t * p )       { return p->pValuesFirst; }

/**Function*************************************************************

  Synopsis    [Returns the temporary storage.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int * Vm_VarMapGetStorageSupport1( Vm_VarMap_t * p )  {  return p->pMan->pSupport1; }
int * Vm_VarMapGetStorageSupport2( Vm_VarMap_t * p )  {  return p->pMan->pSupport2; }
int * Vm_VarMapGetStoragePermute( Vm_VarMap_t * p )   {  return p->pMan->pPermute;  }
int * Vm_VarMapGetStorageArray1( Vm_VarMap_t * p )    {  return p->pMan->pArray1;   }
int * Vm_VarMapGetStorageArray2( Vm_VarMap_t * p )    {  return p->pMan->pArray2;   }

/**Function*************************************************************

  Synopsis    [Returns true if the variable map is binary.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Vm_VarMapIsBinary( Vm_VarMap_t * p )
{
    if ( p->nValuesIn + p->nValuesOut == 2 * (p->nVarsIn + p->nVarsOut) )
        return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Returns true if the variable map is binary.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Vm_VarMapIsBinaryInput( Vm_VarMap_t * p )
{
    if ( p->nValuesIn == 2 * p->nVarsIn )
        return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Returns true if the variable map is binary.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Vm_VarMapGetBitsNum( Vm_VarMap_t * pVm )
{
    int nBits, i;
    nBits = 0;
    for ( i = 0; i < pVm->nVarsIn + pVm->nVarsOut; i++ )
        nBits += Extra_Base2Log( pVm->pValues[i] );
    return nBits;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



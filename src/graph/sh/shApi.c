/**CFile****************************************************************

  FileName    [shApi.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Exported APIs of the structural hashing package.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: shApi.c,v 1.6 2004/05/12 04:30:10 alanmi Exp $]

***********************************************************************/

#include "shInt.h"

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
Sh_Node_t **   Sh_ManagerReadVars( Sh_Manager_t * p )          { return p->pVars;    }
Sh_Node_t *    Sh_ManagerReadVar( Sh_Manager_t * p, int i )    { return p->pVars[i]; }
int            Sh_ManagerReadVarsNum( Sh_Manager_t * p )       { return p->nVars;    }
Sh_Node_t *    Sh_ManagerReadConst1( Sh_Manager_t * p )        { return p->pConst1;  }
int            Sh_ManagerReadNodes( Sh_Manager_t * p )         { return Sh_TableReadNodes(p); }

Sh_Manager_t * Sh_NetworkReadManager( Sh_Network_t * p )       { return p->pMan;     }
int            Sh_NetworkReadInputNum( Sh_Network_t * p )      { return p->nInputs;  }
int            Sh_NetworkReadOutputNum( Sh_Network_t * p )     { return p->nOutputs; }
void           Sh_NetworkSetOutputNum( Sh_Network_t * p, int nOuts )  { p->nOutputs = nOuts; }
Sh_Node_t **   Sh_NetworkReadOutputs( Sh_Network_t * p )       { return p->ppOutputs; }
Sh_Node_t **   Sh_NetworkReadInputs( Sh_Network_t * p )        { return p->pMan->pVars; }
Sh_Node_t *    Sh_NetworkReadOutput( Sh_Network_t * p, int i ) { return p->ppOutputs[i]; }
Sh_Node_t **   Sh_NetworkReadNodes( Sh_Network_t * p )         { return p->ppNodes;     }
int            Sh_NetworkReadNodeNum( Sh_Network_t * p )       { return p->nNodes;     }

Vm_VarMap_t *  Sh_NetworkReadVmL( Sh_Network_t * p )           { return p->pVmL;     }
Vm_VarMap_t *  Sh_NetworkReadVmR( Sh_Network_t * p )           { return p->pVmR;     }
Vm_VarMap_t *  Sh_NetworkReadVmS( Sh_Network_t * p )           { return p->pVmS;     }
Vm_VarMap_t *  Sh_NetworkReadVmLC( Sh_Network_t * p )          { return p->pVmLC;    }
Vm_VarMap_t *  Sh_NetworkReadVmRC( Sh_Network_t * p )          { return p->pVmRC;    }

void           Sh_NetworkSetVmL( Sh_Network_t * p, Vm_VarMap_t * pVm )   { p->pVmL  = pVm; }
void           Sh_NetworkSetVmR( Sh_Network_t * p, Vm_VarMap_t * pVm )   { p->pVmR  = pVm; }
void           Sh_NetworkSetVmS( Sh_Network_t * p, Vm_VarMap_t * pVm )   { p->pVmS  = pVm; }
void           Sh_NetworkSetVmLC( Sh_Network_t * p, Vm_VarMap_t * pVm )  { p->pVmLC = pVm; }
void           Sh_NetworkSetVmRC( Sh_Network_t * p, Vm_VarMap_t * pVm )  { p->pVmRC = pVm; }

Sh_Node_t *    Sh_NodeReadOne( Sh_Node_t * p )                 { return Sh_Regular(p)->pOne;     }
Sh_Node_t *    Sh_NodeReadTwo( Sh_Node_t * p )                 { return Sh_Regular(p)->pTwo;     }
Sh_Node_t *    Sh_NodeReadOrder( Sh_Node_t * p )               { return Sh_Regular(p)->pOrder;   }
int            Sh_NodeReadIndex( Sh_Node_t * p )               { return (Sh_Regular(p)->index < SH_CONST_INDEX)? Sh_Regular(p)->index : Sh_Regular(p)->index - SH_CONST_INDEX; }

unsigned       Sh_NodeReadData( Sh_Node_t * p )                { return p->pData;    }
void           Sh_NodeSetData( Sh_Node_t * p, unsigned uData ) { p->pData = uData;   }

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Sh_ManagerGabrageCollectionEnable( Sh_Manager_t * p )   
{ 
    bool Temp;
    Temp = p->fEnableGC; 
    p->fEnableGC = 1; 
    return Temp; 
}   

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Sh_ManagerGabrageCollectionDisable( Sh_Manager_t * p )  
{ 
    bool Temp = p->fEnableGC; 
    p->fEnableGC = 0; 
    return Temp; 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Sh_ManagerTwoLevelEnable( Sh_Manager_t * p )
{ 
    bool Temp = p->fTwoLevelHash; 
    p->fTwoLevelHash = 1; 
    // create the canonicity table
    if ( p->pCanTable == NULL )
        p->pCanTable = Sh_SignPrecompute();
    return Temp; 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Sh_ManagerTwoLevelDisable( Sh_Manager_t * p ) 
{ 
    bool Temp = p->fTwoLevelHash; 
    p->fTwoLevelHash = 0; 
    return Temp; 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Sh_NodeReadDataCompl( Sh_Node_t * p )
{ 
    return Sh_IsComplement(p)? ~Sh_Regular(p)->pData : p->pData;    
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_NodeSetDataCompl( Sh_Node_t * p, unsigned uData )
{
    if ( Sh_IsComplement(p) )
        Sh_Regular(p)->pData = ~uData;
    else
        p->pData = uData;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Node_t ** Sh_NetworkAllocInputsCore( Sh_Network_t * p, int nInputsCore )
{
    assert( nInputsCore > 0 );
    if ( p->nInputsCore > 0 )
        FREE( p->ppInputsCore );
    p->nInputsCore  = nInputsCore;
    p->ppInputsCore = ALLOC( Sh_Node_t *, nInputsCore );
    return p->ppInputsCore;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Node_t ** Sh_NetworkAllocOutputsCore( Sh_Network_t * p, int nOutputsCore )
{
    assert( nOutputsCore > 0 );
    if ( p->nOutputsCore > 0 )
        FREE( p->ppOutputsCore );
    p->nOutputsCore  = nOutputsCore;
    p->ppOutputsCore = ALLOC( Sh_Node_t *, nOutputsCore );
    return p->ppOutputsCore;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Node_t ** Sh_NetworkAllocSpecials( Sh_Network_t * p, int nSpecials )
{
    assert( nSpecials > 0 );
    if ( p->nSpecials > 0 )
        FREE( p->ppSpecials );
    p->nSpecials  = nSpecials;
    p->ppSpecials = ALLOC( Sh_Node_t *, nSpecials );
    return p->ppSpecials;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Node_t * Sh_NodeAnd( Sh_Manager_t * p, Sh_Node_t * gNode1, Sh_Node_t * gNode2 )
{
    return Sh_CanonNodeAnd( p, gNode1, gNode2 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Node_t * Sh_NodeOr( Sh_Manager_t * p, Sh_Node_t * gNode1, Sh_Node_t * gNode2 )
{
    return Sh_Not( Sh_CanonNodeAnd( p, Sh_Not(gNode1), Sh_Not(gNode2) ) );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Node_t * Sh_NodeExor( Sh_Manager_t * p, Sh_Node_t * gNode1, Sh_Node_t * gNode2 )
{
    return Sh_NodeMux( p, gNode1, Sh_Not(gNode2), gNode2 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Node_t * Sh_NodeMux( Sh_Manager_t * p, Sh_Node_t * gNode, Sh_Node_t * gNodeT, Sh_Node_t * gNodeE )
{
    Sh_Node_t * gAnd1, * gAnd2, * gRes;
    gAnd1 = Sh_CanonNodeAnd( p, gNode,         gNodeT );  shRef( gAnd1 );
    gAnd2 = Sh_CanonNodeAnd( p, Sh_Not(gNode), gNodeE );  shRef( gAnd2 );
    gRes  = Sh_NodeOr( p, gAnd1, gAnd2 );                 shRef( gRes );
    Sh_RecursiveDeref( p, gAnd1 );
    Sh_RecursiveDeref( p, gAnd2 );
    shDeref( gRes );
    return gRes;
}




/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Sh_NodeIsAnd( Sh_Node_t * gNode )
{
    return shNodeIsAnd(gNode);
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Sh_NodeIsVar( Sh_Node_t * gNode )
{
    return shNodeIsVar(gNode);
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Sh_NodeIsConst( Sh_Node_t * gNode )
{
    return shNodeIsConst(gNode);
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_Ref( Sh_Node_t * gNode )           
{
    shRef(gNode);
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_Deref( Sh_Node_t * gNode )           
{
    shDeref(gNode);
}


/**Function*************************************************************

  Synopsis    [Recursively dereferences the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_RecursiveDeref( Sh_Manager_t * pMan, Sh_Node_t * gNode )
{
    Sh_Node_t * gNodeR;
    gNodeR = Sh_Regular(gNode);
    if ( gNodeR->nRefs <= 0 )
    {
        if ( pMan->nRefErrors++ == 0 )
        {
            printf( "\n" );
            printf( "A reference counting error is detected in the AI-graph package.\n" );
            printf( "This error indicates that a new node was not referenced by\n" );
            printf( "calling Sh_Ref(), or that a node was dereferenced twice.\n" );
            printf( "The package will fail after the next garbage collection.\n" );
            printf( "\n" );
//            assert( 0 );
        }
//        Sh_NodePrintOne( gNode );
//        printf( "\n" );
        return;
    }
    if ( --gNodeR->nRefs == 0 && gNodeR->pOne )
    {
        Sh_RecursiveDeref( pMan, gNodeR->pOne );
        Sh_RecursiveDeref( pMan, gNodeR->pTwo );
    }
}

///////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



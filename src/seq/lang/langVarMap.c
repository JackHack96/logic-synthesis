/**CFile****************************************************************

  FileName    [langVarMap.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Procedures to create various variable maps.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: langVarMap.c,v 1.5 2004/02/19 03:06:55 alanmi Exp $]

***********************************************************************/

#include "langInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates permuted extended variable map.]

  Description [The original extended global variable map contains the
  PI, CS, NS, and PO vars ordered as follows: mix(PI,CS), mix(PO,NS).
  The resulting variable map is ordered as follows: PI, PO, CS, NS.
  The resulting map is useful to derive the PI/PO conditions of
  the transitions by the call to Fnc_FunctionDeriveSopFromMddSpecial().
  The node (pNodeGlo) gives the ordering of PI and CS variables. 
  The way the COs are linked in the internal network linked list
  gives the ordering of PO and NS variables. ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vmx_VarMap_t * Lang_VarMapStg( Ntk_Network_t * pNet )
{
    Vmx_VarMap_t * pVmxExt;
    Vmx_VarMap_t * pVmxNew;
    Ntk_Node_t * pNode;
//    Ntk_Pin_t * pPin;
    int nVarsTot, nVarsPIO, nVarsPI, nVarsPO, nLatches;//, nFanins;
    int iVarPIO, iVarLIO, i, k;
    int * pPermute;

    // get the extended variable map
    pVmxExt = Ntk_NetworkReadVmxGlo( pNet );

    // the number of variables
    nVarsTot = Ntk_NetworkReadCiNum(pNet) + Ntk_NetworkReadCoNum(pNet);
    nLatches = Ntk_NetworkReadLatchNum(pNet);
    nVarsPI  = Ntk_NetworkReadCiNum(pNet) - nLatches;
    nVarsPO  = Ntk_NetworkReadCoNum(pNet) - nLatches;
    nVarsPIO = nVarsPI + nVarsPO;
//    nFanins  = Ntk_NodeReadFaninNum( pNodeGlo );
/*
    // create the variable map, which contains the PI and PO variables
    // listed first, followed by other variables
    iVarPIO = 0;
    iVarLIO = nVarsPIO;
    pPermute = ALLOC( int, nVarsTot );
    Ntk_NodeForEachFaninWithIndex( pNodeGlo, pPin, pFanin, i )
    {
        if ( pFanin->Subtype == MV_BELONGS_TO_LATCH )
            pPermute[iVarLIO++] = i;
        else
            pPermute[iVarPIO++] = i;
    }
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        if ( pNode->Subtype == MV_BELONGS_TO_LATCH )
            pPermute[iVarLIO++] = i;
        else
            pPermute[iVarPIO++] = i;
        i++;
    }
    assert( iVarPIO == nVarsPIO );
    assert( iVarLIO == nVarsTot );
*/
    // create Vmx with the following ordering: PI, PO, CS, NS
    iVarPIO = 0;
    iVarLIO = nVarsPIO;
    pPermute = ALLOC( int, nVarsTot );
    k = 0;
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        // find the given variable in the order
//        Ntk_NodeForEachFaninWithIndex( pNodeGlo, pPin, pFanin, k )
//            if ( pFanin == pNode )
//                break;
        assert( k < nVarsPI + nLatches );
        // set the given variable in the new order
        if ( pNode->Subtype == MV_BELONGS_TO_LATCH )
            pPermute[iVarLIO++] = k;
        else
            pPermute[iVarPIO++] = k;
        k++;
    }
    assert( iVarPIO == nVarsPI );
    assert( iVarLIO == nVarsPIO + nLatches );
    i = nVarsPI + nLatches;
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        if ( pNode->Subtype == MV_BELONGS_TO_LATCH )
            pPermute[iVarLIO++] = i;
        else
            pPermute[iVarPIO++] = i;
        i++;
    }
    assert( iVarPIO == nVarsPIO );
    assert( iVarLIO == nVarsTot );

    pVmxNew = Vmx_VarMapCreatePermuted( pVmxExt, pPermute );
    FREE( pPermute );

    return pVmxNew;
}

/**Function*************************************************************

  Synopsis    [Create the variable map with the duplicated latches.]

  Description [The original variable map is ordered: PI, PO, CS, NS.
  Two new sets of variables are added: CS2 and NS2, equal in size to
  CS and NS. The bits for them are appended to the current bit order.
  The MV variables themselves are ordered as follows: PI, PO, CS, CS2, 
  NS, NS2. This variable map is used for reencoding and state 
  minimization.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vmx_VarMap_t * Lang_VarMapDupLatch( Lang_Auto_t * pAut )
{
    Vm_VarMap_t * pVm, * pVmNew;
    Vmx_VarMap_t * pVmxNew;
    int * pValuesNew, * pValues;
    int * pMapping;
    int i, iVar;

    // create the variable map of the new variables
    pVm = Vmx_VarMapReadVm( pAut->pVmx );
    pValues = Vm_VarMapReadValuesArray( pVm );
    pValuesNew = ALLOC( int, pAut->nInputs + 4 * pAut->nLatches );
    memcpy( pValuesNew, pValues, sizeof(int) * (pAut->nInputs + 2 * pAut->nLatches) );

    // add the new CS and NS vars
    iVar = pAut->nInputs + 2 * pAut->nLatches;
    for ( i = 0; i < pAut->nLatches; i++ )
        pValuesNew[iVar++] = pValues[pAut->nInputs + i];
    for ( i = 0; i < pAut->nLatches; i++ )
        pValuesNew[iVar++] = pValues[pAut->nInputs + i];

    // create the new VM
    pVmNew  = Vm_VarMapLookup( Fnc_ManagerReadManVm(pAut->pMan), pAut->nInputs, 4 * pAut->nLatches, pValuesNew );
    pVmxNew = Vmx_VarMapCreateAppended( pAut->pVmx, pVmNew );
    FREE( pValuesNew );

    // permute the new Vmx as follows:
    // (IO, CS1, NS1, CS2, NS2) -> (IO, CS1, CS2, NS1, NS2)
    pMapping = ALLOC( int, Vm_VarMapReadVarsNum(pVmNew) );
    iVar = 0;
    // add IO vars
    for ( i = 0; i < pAut->nInputs; i++ )
        pMapping[iVar++] = i;
    // add CS1 vars
    for ( i = 0; i < pAut->nLatches; i++ )
        pMapping[iVar++] = pAut->nInputs + 0 * pAut->nLatches + i;
    // add CS2 vars
    for ( i = 0; i < pAut->nLatches; i++ )
        pMapping[iVar++] = pAut->nInputs + 2 * pAut->nLatches + i;
    // add NS1 vars
    for ( i = 0; i < pAut->nLatches; i++ )
        pMapping[iVar++] = pAut->nInputs + 1 * pAut->nLatches + i;
    // add NS2 vars
    for ( i = 0; i < pAut->nLatches; i++ )
        pMapping[iVar++] = pAut->nInputs + 3 * pAut->nLatches + i;

    assert( iVar == Vm_VarMapReadVarsNum(pVmNew) );
    pVmxNew = Vmx_VarMapCreatePermuted( pVmxNew, pMapping );
    FREE( pMapping );
    return pVmxNew;
}

/**Function*************************************************************

  Synopsis    [Create the variable map for the product automaton.]

  Description [The original variable maps are ordered: IO, CS, NS.
  The new map merges the idential IO vars, appends the unique IO vars 
  of the second automaton after the vars of the first automaton.
  Then, it maps the CS/NS vars of the second automaton at the end of 
  the var order. Finally, the MV variables are permuted as follows:
  (IO1, CS1, NS1, IO2n, CS2, NS2) -> (IO1, IO2n, CS1, CS2, NS1, NS2).
  Return also the permutation variable map, which remaps the BDDs
  from the second automaton into the product automaton. The first
  automaton does not need such a map.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vmx_VarMap_t * Lang_VarMapProduct( Lang_Auto_t * pAut, Lang_Auto_t * pAut1, Lang_Auto_t * pAut2, int ** ppPermute )
{
    Vm_VarMap_t * pVm1, * pVm2, * pVmNew;
    Vmx_VarMap_t * pVmxNew;
    int * pValuesNew, * pValues1, * pValues2, nVarsNew;
    int * pBits1, * pBitsFirst1, * pBitsOrder1;
    int * pBits2, * pBitsFirst2, * pBitsOrder2;
    int * pPermute, * pMapping;
    int i, v, b, iVar, iBit;

    // start the expanded var map
    pVm1 = Vmx_VarMapReadVm( pAut1->pVmx );
    pVm2 = Vmx_VarMapReadVm( pAut2->pVmx );
    pValues1 = Vm_VarMapReadValuesArray( pVm1 );
    pValues2 = Vm_VarMapReadValuesArray( pVm2 );
    pValuesNew = ALLOC( int, pAut1->nInputs + pAut2->nInputs + 4 );
    memcpy( pValuesNew, pValues1, sizeof(int) * (pAut1->nInputs + 2) );

    pBits1      = Vmx_VarMapReadBits( pAut1->pVmx );
    pBitsFirst1 = Vmx_VarMapReadBitsFirst( pAut1->pVmx );
    pBitsOrder1 = Vmx_VarMapReadBitsOrder( pAut1->pVmx );

    pBits2      = Vmx_VarMapReadBits( pAut2->pVmx );
    pBitsFirst2 = Vmx_VarMapReadBitsFirst( pAut2->pVmx );
    pBitsOrder2 = Vmx_VarMapReadBitsOrder( pAut2->pVmx );

    // start the permutation map
    pPermute = ALLOC( int, pAut1->dd->size + pAut2->dd->size );
    for ( v = 0; v < pAut1->dd->size + pAut2->dd->size; v++ )
        pPermute[v] = v;

    // start with the variables from the first automaton
    for ( v = 0; v < pAut1->nInputs + 2 * pAut1->nLatches; v++ )
        pValuesNew[v] = pValues1[v];
//Vmx_VarMapPrint( pAut1->pVmx );
//Vmx_VarMapPrint( pAut2->pVmx );
//Vmx_VarMapPrint( pAut->pVmx );

    // go through the variables of pAut2 and decide what to do with them
    // if they are present in pAut1, they should be mapped into them
    // if not, they should be appended to the variable map
    nVarsNew = 0;
    iVar = pAut1->nInputs + 2 * pAut1->nLatches;
//    iBit = Vmx_VarMapReadBitsNum(pAut1->pVmx); //pAut->dd->size;
    iBit = Vmx_VarMapGetLargestBit( pAut1->pVmx );
    for ( v = 0; v < pAut2->nInputs; v++ )
    {
        for ( i = 0; i < pAut1->nInputs; i++ )
            if ( strcmp( pAut1->ppNamesInput[i], pAut2->ppNamesInput[v] ) == 0 )
                break;
        if ( i == pAut1->nInputs ) // new var
        {
            nVarsNew++;
            pValuesNew[iVar++] = pValues2[v];
            // map v-th var of pAut2 at the end of the variable range of pAut1
            for ( b = 0; b < pBits2[v]; b++ )
                pPermute[ pBitsOrder2[pBitsFirst2[v] + b] ] = iBit++;
        }
        else
        {
            // remap v-th var of pAut2 into i-th var of pAut1
            for ( b = 0; b < pBits2[v]; b++ )
                pPermute[ pBitsOrder2[pBitsFirst2[v] + b] ] = pBitsOrder1[pBitsFirst1[i] + b];
        }
    }
    assert( nVarsNew == 0 );
    // add the CS and NS vars of pAut2
    for ( v = 0; v < pAut2->nLatches; v++ )
    {
        pValuesNew[iVar++] = pValues2[pAut2->nInputs + v];
        for ( b = 0; b < pBits2[pAut2->nInputs + v]; b++ )
        {
            pPermute[ pBitsOrder2[pBitsFirst2[pAut2->nInputs + v] + b] ] = iBit++;
//            Cudd_bddNewVar( pAut->dd );
        }
    }
    for ( v = 0; v < pAut2->nLatches; v++ )
    {
        pValuesNew[iVar++] = pValues2[pAut2->nInputs + pAut2->nLatches + v];
        for ( b = 0; b < pBits2[pAut2->nInputs + pAut2->nLatches + v]; b++ )
        {
            pPermute[ pBitsOrder2[pBitsFirst2[pAut2->nInputs + pAut2->nLatches + v] + b] ] = iBit++;
//            Cudd_bddNewVar( pAut->dd );
        }
    }


    // create the new VM
    pVmNew  = Vm_VarMapLookup( Fnc_ManagerReadManVm(pAut1->pMan), 
        iVar - pAut1->nLatches - pAut2->nLatches, pAut1->nLatches + pAut2->nLatches, pValuesNew );
    pVmxNew = Vmx_VarMapCreateAppended( pAut1->pVmx, pVmNew );
    FREE( pValuesNew );
//    assert( Vmx_VarMapReadBitsNum(pVmxNew) == iBit );

    // permute the new Vmx as follows:
    // (IO1, CS1, NS1, IO2, CS2, NS2) -> (IO1, IO2, CS1, CS2, NS1, NS2)
    pMapping = ALLOC( int, Vm_VarMapReadVarsNum(pVmNew) );
    iVar = 0;
    for ( v = 0; v < pAut1->nInputs; v++ )  // IO1
        pMapping[iVar++] = v;
    for ( v = 0; v < nVarsNew; v++ )        // IO2
        pMapping[iVar++] = pAut1->nInputs + 2*pAut1->nLatches + v;
    for ( v = 0; v < pAut1->nLatches; v++ ) // CS1
        pMapping[iVar++] = pAut1->nInputs + v;
    for ( v = 0; v < pAut2->nLatches; v++ ) // CS2
        pMapping[iVar++] = pAut1->nInputs + 2*pAut1->nLatches + nVarsNew + v;
    for ( v = 0; v < pAut1->nLatches; v++ ) // NS1
        pMapping[iVar++] = pAut1->nInputs + pAut1->nLatches + v;
    for ( v = 0; v < pAut2->nLatches; v++ ) // NS2
        pMapping[iVar++] = pAut1->nInputs + 2*pAut1->nLatches + nVarsNew + pAut2->nLatches + v;

    assert( iVar == Vm_VarMapReadVarsNum(pVmNew) );
    pVmxNew = Vmx_VarMapCreatePermuted( pVmxNew, pMapping );
    FREE( pMapping );

    if ( ppPermute )
        *ppPermute = pPermute;
    else
        FREE( ppPermute );
    return pVmxNew;
}


/**Function*************************************************************

  Synopsis    [Updates the var map after changing the number of state bits.]

  Description [The last two variables in the var map are the CS and NS vars.
  When the number of state decreases, we reuse the variables currently
  allocated for state bits. When the number increases, we add extra binary
  variables at the bottom of the manager.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vmx_VarMap_t * Lang_VarMapReencodeExt( Lang_Auto_t * pAut, int nStatesNew )
{
    Vm_VarMap_t * pVm, * pVmNew;
    int * pValues, * pValuesNew;
    int nVars;

    // create the new var map
    pVm     = Vmx_VarMapReadVm( pAut->pVmx );
    pValues = Vm_VarMapReadValuesArray( pVm );
    nVars   = Vm_VarMapReadVarsNum( pVm );
    pValuesNew = ALLOC( int, nVars + 1 );
    memcpy( pValuesNew, pValues, sizeof(int) * nVars );
    pValuesNew[nVars] = nStatesNew;
    pVmNew = Vm_VarMapLookup( Fnc_ManagerReadManVm(pAut->pMan), nVars, 1, pValuesNew );
    FREE( pValuesNew );
    return Vmx_VarMapCreateAppended( pAut->pVmx, pVmNew );
}

/**Function*************************************************************

  Synopsis    [Updates the var map after changing the number of state bits.]

  Description [The last two variables in the var map are the CS and NS vars.
  When the number of state decreases, we reuse the variables currently
  allocated for state bits. When the number increases, we add extra binary
  variables at the bottom of the manager.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vmx_VarMap_t * Lang_VarMapReencodeNew( Lang_Auto_t * pAut, int nStatesNew )
{
    Vm_VarMap_t * pVm, * pVmNew;
    Vmx_VarMap_t * pVmxNew;
    int * pBits, * pBitsFirst, * pBitsOrder;
    int * pValuesNew, * pBitsOrderNew;
    int nBitsTotal, nBitsNew, nBitsIo;
    int v, b, iBit, iVar;

    // create the new var map
    pVm = Vmx_VarMapReadVm( pAut->pVmx );
    pValuesNew = ALLOC( int, Vm_VarMapReadVarsNum(pVm) );
    for ( v = 0; v < pAut->nInputs; v++ )
        pValuesNew[v] = Vm_VarMapReadValues( pVm, v );
    pValuesNew[pAut->nInputs + 0] = nStatesNew;
    pValuesNew[pAut->nInputs + 1] = nStatesNew;
    pVmNew = Vm_VarMapLookup( Fnc_ManagerReadManVm(pAut->pMan), pAut->nInputs, 2, pValuesNew );
    FREE( pValuesNew );

    // derive the extended var map
    nBitsNew   = Extra_Base2Log( nStatesNew );

    // get hold of the bit information
    pBits      = Vmx_VarMapReadBits( pAut->pVmx );
    pBitsFirst = Vmx_VarMapReadBitsFirst( pAut->pVmx );
    pBitsOrder = Vmx_VarMapReadBitsOrder( pAut->pVmx );

    // start the new storage for bits
    pBitsOrderNew = ALLOC( int, pAut->dd->size );
    iBit = 0;
    // copy the input bits first
    for ( v = 0; v < pAut->nInputs; v++ )
        for ( b = 0; b < pBits[v]; b++ )
            pBitsOrderNew[iBit++] = pBitsOrder[pBitsFirst[v] + b];
    // copy the CS bits starting from the old latch CS bits
    nBitsIo = iBit;
    for ( v = 0; v < pAut->nLatches; v++ )
    {
        iVar = pAut->nInputs + v;
        for ( b = 0; b < pBits[iVar]; b++ )
        {
            pBitsOrderNew[iBit++] = pBitsOrder[pBitsFirst[iVar] + b];
            if ( iBit == nBitsIo + nBitsNew )
                break;
        }
        if ( b != pBits[iVar] )
            break;
    }
    // add extra bits if necessary
    while ( iBit < nBitsIo + nBitsNew )
        pBitsOrderNew[iBit++] = Cudd_bddNewVar(pAut->dd)->index;

    // copy the NS bits starting from the old latch NS bits
    for ( v = 0; v < pAut->nLatches; v++ )
    {
        iVar = pAut->nInputs + pAut->nLatches + v;
        for ( b = 0; b < pBits[iVar]; b++ )
            pBitsOrderNew[iBit++] = pBitsOrder[pBitsFirst[iVar] + b];
    }
    // add extra bits if necessary
    while ( iBit < nBitsIo + 2 * nBitsNew )
        pBitsOrderNew[iBit++] = Cudd_bddNewVar(pAut->dd)->index;

    nBitsTotal = Vmx_VarMapReadBitsNum(pAut->pVmx) - 2 * (pAut->nStateBits - nBitsNew);
    pVmxNew = Vmx_VarMapLookup( Fnc_ManagerReadManVmx(pAut->pMan), pVmNew, nBitsTotal, pBitsOrderNew );
    FREE( pBitsOrderNew );
    return pVmxNew;
}


/**Function*************************************************************

  Synopsis    [Create the variable map for the reencoded automaton.]

  Description [The original variable map may have several latches.
  The extended map contains only one latch which can represent the
  given number of states (nStatesNew). This map reuses the state bits 
  of the old latches in their natural order.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vmx_VarMap_t * Lang_VarMapReencode_old( Lang_Auto_t * pAut, int nStatesNew )
{
    Vm_VarMap_t * pVm, * pVmNew;
    Vmx_VarMap_t * pVmxNew;
    int * pValuesNew, * pValues;
    int nVarsBin, * pBitsOrder;

    // create the variable map of the new variables
    pVm = Vmx_VarMapReadVm( pAut->pVmx );
    pValues = Vm_VarMapReadValuesArray( pVm );
    pValuesNew = ALLOC( int, pAut->nInputs + 2 );
    memcpy( pValuesNew, pValues, sizeof(int) * pAut->nInputs );

    // add the new CS and NS vars
    pValuesNew[pAut->nInputs + 0] = nStatesNew;
    pValuesNew[pAut->nInputs + 1] = nStatesNew;

    // create the new VM
    pVmNew  = Vm_VarMapLookup( Fnc_ManagerReadManVm(pAut->pMan), pAut->nInputs, 2, pValuesNew );
    FREE( pValuesNew );

    // get the bit parameters
    pBitsOrder = Vmx_VarMapReadBitsOrder( pAut->pVmx );
    nVarsBin = Vmx_VarMapReadBitsNum( pAut->pVmx ) + 2 * Extra_Base2Log(nStatesNew);

    // get the permutes var map using the same bit order
    pVmxNew = Vmx_VarMapLookup( Fnc_ManagerReadManVmx(pAut->pMan), pVmNew, nVarsBin, pBitsOrder );
    return pVmxNew;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



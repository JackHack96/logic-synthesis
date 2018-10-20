/**CFile****************************************************************

  FileName    [ntkiVerify.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Network verification procedures.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkiVerify.c,v 1.8 2004/10/18 02:50:22 alanmi Exp $]

***********************************************************************/

#include "ntkiInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Ntk_NetworkVerifyFunctions( Ntk_Network_t * pNet1, Ntk_Network_t * pNet2, int fVerbose );
extern int Ntk_NetworkVerifyVariables( Ntk_Network_t * pNet1, Ntk_Network_t * pNet2, int fVerbose );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////
    
/**Function*************************************************************

  Synopsis    [The top level verification procedure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkVerify( Ntk_Network_t * pNet1, Ntk_Network_t * pNet2, int fVerbose )
{
    // verify the identity of network CI/COs
    if ( !Ntk_NetworkVerifyVariables( pNet1, pNet2, fVerbose ) )
        return;

    // verify the functionality
    Ntk_NetworkVerifyFunctions( pNet1, pNet2, fVerbose );
}


/**Function*************************************************************

  Synopsis    [Verifies the CIs/COs of the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkVerifyVariables( Ntk_Network_t * pNet1, Ntk_Network_t * pNet2, int fVerbose )
{
    Ntk_Node_t * pNode1, * pNode2;

    // make sure that networks have the same number of inputs/outputs/latches
    if ( Ntk_NetworkReadCiNum(pNet1) != Ntk_NetworkReadCiNum(pNet2) )
    {
        fprintf( Ntk_NetworkReadMvsisOut(pNet1), "Networks have different number of PIs. Verification is not performed.\n" );
        return 0;
    }
    if ( Ntk_NetworkReadCoNum(pNet1) != Ntk_NetworkReadCoNum(pNet2) )
    {
        fprintf( Ntk_NetworkReadMvsisOut(pNet1), "Networks have different number of POs. Verification is not performed.\n" );
        return 0;
    }
    if ( Ntk_NetworkReadLatchNum(pNet1) != Ntk_NetworkReadLatchNum(pNet2) )
    {
        fprintf( Ntk_NetworkReadMvsisOut(pNet1), "Networks have different number of latches. Verification is not performed.\n" );
        return 0;
    }

    // make sure that the names of the inputs/outputs/latches are matching
    Ntk_NetworkForEachCi( pNet1, pNode1 )
    {
        pNode2 = Ntk_NetworkFindNodeByName( pNet2, pNode1->pName );
        if ( pNode2 == NULL )
        {
            fprintf( Ntk_NetworkReadMvsisOut(pNet1), "<%s> is a CI of the first network but not of the second. Verification is not performed.\n", pNode1->pName );
            return 0;
        }
        if ( pNode1->nValues != pNode2->nValues )
        {
            fprintf( Ntk_NetworkReadMvsisOut(pNet1), "CI <%s> has different number of values in the networks. Verification is not performed.\n", pNode1->pName );
            return 0;
        }
    }
    Ntk_NetworkForEachCo( pNet1, pNode1 )
    {
        pNode2 = Ntk_NetworkFindNodeByName( pNet2, pNode1->pName );
        if ( pNode2 == NULL )
        {
            fprintf( Ntk_NetworkReadMvsisOut(pNet1), "<%s> is a CO of the first network but not of the second. Verification is not performed.\n", pNode1->pName );
            return 0;
        }
        if ( pNode1->nValues != pNode2->nValues )
        {
            fprintf( Ntk_NetworkReadMvsisOut(pNet1), "CO <%s> has different number of values in the networks. Verification is not performed.\n", pNode1->pName );
            return 0;
        }
    }

    Ntk_NetworkForEachCi( pNet2, pNode1 )
    {
        if ( Ntk_NetworkFindNodeByName( pNet1, pNode1->pName ) == NULL )
        {
            fprintf( Ntk_NetworkReadMvsisOut(pNet1), "<%s> is a PI of the second network but not of the first. Verification is not performed.\n", pNode1->pName );
            return 0;
        }
    }
    Ntk_NetworkForEachCo( pNet2, pNode1 )
    {
        if ( Ntk_NetworkFindNodeByName( pNet1, pNode1->pName ) == NULL )
        {
            fprintf( Ntk_NetworkReadMvsisOut(pNet1), "<%s> is a PO of the second network but not of the first. Verification is not performed.\n", pNode1->pName );
            return 0;
        }
    }
    return 1;
}

    
/**Function*************************************************************

  Synopsis    [Verifies the functionality of the networks.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkVerifyFunctions( Ntk_Network_t * pNet1, Ntk_Network_t * pNet2, int fVerbose )
{
    DdManager * dd;
    char ** psLeavesByName, ** psRootsByName;
    Vmx_VarMap_t * pVmx;
    Vm_VarMap_t * pVm;
    DdNode *** ppMdds1, *** ppMddsExdc1;
    DdNode *** ppMdds2, *** ppMddsExdc2;
    int nLeaves, nRoots;
    int * pValuesOut;
    int fpNet1ND, fpNet2ND;
    int fMadeupExdc = 0;
    int nOuts, clk, i;
    DdNode * bTrace;
    int iOutput, nValue;
    bool fErrorTrace = 0;

    // get the CI variable order
    psLeavesByName = Ntk_NetworkOrderArrayByName( pNet1, 0 );
    // collect the output names and values
    psRootsByName = Ntk_NetworkGlobalGetOutputsAndValues( pNet1, &pValuesOut );
    // get the variable map
    pVmx = Ntk_NetworkGlobalGetVmx( pNet1, psLeavesByName );
    pVm  = Vmx_VarMapReadVm( pVmx );
    // get the number of outputs 
    nOuts = Ntk_NetworkReadCoNum( pNet1 );
    // collect the output values

    // start the manager, in which the global MDDs will be stored
    dd = Cudd_Init( 0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    Cudd_AutodynEnable( dd, CUDD_REORDER_SYMM_SIFT );

    // compute the MDDs of both networks
    nLeaves = Ntk_NetworkReadCiNum(pNet1);
    nRoots  = Ntk_NetworkReadCoNum(pNet1);
    ppMdds1 = Ntk_NetworkGlobalMddCompute( dd, pNet1, psLeavesByName, nLeaves, psRootsByName, nRoots, pVmx );
    if ( pNet1->pNetExdc )
    {
        ppMddsExdc1 = Ntk_NetworkGlobalMddCompute( dd, pNet1->pNetExdc, psLeavesByName, nLeaves, psRootsByName, nRoots, pVmx );
        // convert the EXDC MDDs into the standard format
        ppMddsExdc1 = Ntk_NetworkGlobalMddConvertExdc( dd, pValuesOut, nOuts, ppMddsExdc1 );
    }
    else
        ppMddsExdc1 = Ntk_NetworkGlobalMddEmpty( dd, pValuesOut, nOuts );

    ppMdds2 = Ntk_NetworkGlobalMddCompute( dd, pNet2, psLeavesByName, nLeaves, psRootsByName, nRoots, pVmx );
    if ( pNet2->pNetExdc )
    {
        ppMddsExdc2 = Ntk_NetworkGlobalMddCompute( dd, pNet2->pNetExdc, psLeavesByName, nLeaves, psRootsByName, nRoots, pVmx );
        // convert the EXDC MDDs into the standard format
        ppMddsExdc2 = Ntk_NetworkGlobalMddConvertExdc( dd, pValuesOut, nOuts, ppMddsExdc2 );
    }
    else
        ppMddsExdc2 = Ntk_NetworkGlobalMddEmpty( dd, pValuesOut, nOuts );

    // check the non-determinism
    clk = clock();
    fpNet1ND = Ntk_NetworkGlobalMddCheckND( dd, pValuesOut, nOuts, ppMdds1 );
    fpNet2ND = Ntk_NetworkGlobalMddCheckND( dd, pValuesOut, nOuts, ppMdds2 );

    // report the EXDC status
    fprintf( Ntk_NetworkReadMvsisOut(pNet1), "The current  network (CUR) is %s and has%s exdc.\n", 
       (fpNet1ND?"non-deterministic":"completely specified"), (pNet1->pNetExdc?"":" no")  );
    fprintf( Ntk_NetworkReadMvsisOut(pNet1), "The external network (EXT) is %s and has%s exdc.\n", 
       (fpNet2ND?"non-deterministic":"completely specified"), (pNet2->pNetExdc?"":" no")  );

    if ( Ntk_NetworkGlobalMddCheckEquality( dd, pValuesOut, nOuts, ppMdds1, ppMdds2 ) )
        fprintf( Ntk_NetworkReadMvsisOut(pNet1), "Equivalence:   CUR w/o exdc = EXT w/o exdc.\n" );
    else if ( pNet1->pNetExdc == NULL && pNet2->pNetExdc == NULL )
    {
        fprintf( Ntk_NetworkReadMvsisOut(pNet1), "Non-equival:   CUR w/o exdc != EXT w/o exdc.\n" );
        if ( Ntk_NetworkGlobalMddCheckContainment( dd, pValuesOut, nOuts, ppMdds1, ppMdds2 ) )
            fprintf( Ntk_NetworkReadMvsisOut(pNet1), "Containment:   CUR w/o exdc < EXT w/o exdc.\n" );
        else //if ( envi->trace )
        {
            bTrace = Ntk_NetworkGlobalMddErrorTrace( dd, pValuesOut, nOuts, ppMdds1, ppMdds2, &iOutput, &nValue );
            Cudd_Ref( bTrace );

            fprintf( Ntk_NetworkReadMvsisOut(pNet1), "Error trace:   CUR " );
            Ntk_NetworkGlobalMddPrintErrorTrace( Ntk_NetworkReadMvsisOut(pNet1), dd, pVmx, bTrace, iOutput, psRootsByName[iOutput], nValue, psLeavesByName );
            if ( !fMadeupExdc )
                fprintf( Ntk_NetworkReadMvsisOut(pNet1), " is not in EXT" );
            else
                fprintf( Ntk_NetworkReadMvsisOut(pNet1), " is not in EXT" );
            fprintf( Ntk_NetworkReadMvsisOut(pNet1), ".\n" );

            Cudd_RecursiveDeref( dd, bTrace );
            fErrorTrace = 1;
        }

        if ( Ntk_NetworkGlobalMddCheckContainment( dd, pValuesOut, nOuts, ppMdds2, ppMdds1 ) )
            fprintf( Ntk_NetworkReadMvsisOut(pNet1), "Containment:   EXT w/o exdc < CUR w/o exdc.\n" );
        else //if ( envi->trace )
        {
            bTrace = Ntk_NetworkGlobalMddErrorTrace( dd, pValuesOut, nOuts, ppMdds2, ppMdds1, &iOutput, &nValue );
            Cudd_Ref( bTrace );

            fprintf( Ntk_NetworkReadMvsisOut(pNet1), "Error trace:   EXT " );
            Ntk_NetworkGlobalMddPrintErrorTrace( Ntk_NetworkReadMvsisOut(pNet1), dd, pVmx, bTrace, iOutput, psRootsByName[iOutput], nValue, psLeavesByName );
            if ( !fMadeupExdc )
                fprintf( Ntk_NetworkReadMvsisOut(pNet1), " is not in CUR" );
            else
                fprintf( Ntk_NetworkReadMvsisOut(pNet1), " is not in CUR" );
            fprintf( Ntk_NetworkReadMvsisOut(pNet1), ".\n" );

            Cudd_RecursiveDeref( dd, bTrace );
            fErrorTrace = 1;
        }
    }
    else
    {
        int fAnswer = 0;
        DdNode *** ppMddsSum;

        /////////////////////////////////////////////////////////////////////////////////////////////////
        // in case the first network has EXDC and the second one has no EXDC, copy it from the first network
        if ( pNet1->pNetExdc && !pNet2->pNetExdc )
        {
            Ntk_NetworkGlobalMddDeref( dd, pValuesOut, nOuts, ppMddsExdc2 );    
            ppMddsExdc2 = Ntk_NetworkGlobalMddDup( dd, pValuesOut, nOuts, ppMddsExdc1 );
            fMadeupExdc = 1;
        }
        /////////////////////////////////////////////////////////////////////////////////////////////////

        if ( pNet2->pNetExdc || fMadeupExdc )
        {
            ppMddsSum = Ntk_NetworkGlobalMddOr( dd, pValuesOut, nOuts, ppMdds2, ppMddsExdc2 );
            if ( Ntk_NetworkGlobalMddCheckContainment( dd, pValuesOut, nOuts, ppMdds1, ppMddsSum ) )
            {
                if ( !fMadeupExdc )
                    fprintf( Ntk_NetworkReadMvsisOut(pNet1), "Containment:   CUR w/o exdc < EXT + exdc(EXT).\n" );
                else
                    fprintf( Ntk_NetworkReadMvsisOut(pNet1), "Containment:   CUR w/o exdc < EXT + exdc(CUR).\n" );
                fAnswer = 1;
            }
            else //if ( envi->trace )
            {
                bTrace = Ntk_NetworkGlobalMddErrorTrace( dd, pValuesOut, nOuts, ppMdds1, ppMddsSum, &iOutput, &nValue );
                Cudd_Ref( bTrace );

                fprintf( Ntk_NetworkReadMvsisOut(pNet1), "Error trace:   CUR " );
                Ntk_NetworkGlobalMddPrintErrorTrace( Ntk_NetworkReadMvsisOut(pNet1), dd, pVmx, bTrace, iOutput, psRootsByName[iOutput], nValue, psLeavesByName );
                if ( !fMadeupExdc )
                    fprintf( Ntk_NetworkReadMvsisOut(pNet1), " is not in EXT + exdc(EXT)" );
                else
                    fprintf( Ntk_NetworkReadMvsisOut(pNet1), " is not in EXT + exdc(CUR)" );
                fprintf( Ntk_NetworkReadMvsisOut(pNet1), ".\n" );

                Cudd_RecursiveDeref( dd, bTrace );
                fErrorTrace = 1;
            }

            Ntk_NetworkGlobalMddDeref( dd, pValuesOut, nOuts, ppMddsSum );  
        }

        if ( pNet1->pNetExdc )
        {
            ppMddsSum = Ntk_NetworkGlobalMddOr( dd, pValuesOut, nOuts, ppMdds1, ppMddsExdc1 );
            if ( Ntk_NetworkGlobalMddCheckContainment( dd, pValuesOut, nOuts, ppMdds2, ppMddsSum ) )
            {
                fprintf( Ntk_NetworkReadMvsisOut(pNet1), "Containment:   EXT w/o exdc < CUR + exdc(CUR).\n" );
                fAnswer = 1;
            }
            else //if ( envi->trace )
            {
                bTrace = Ntk_NetworkGlobalMddErrorTrace( dd, pValuesOut, nOuts, ppMdds2, ppMddsSum, &iOutput, &nValue );
                Cudd_Ref( bTrace );

                fprintf( Ntk_NetworkReadMvsisOut(pNet1), "Error trace:   EXT " );
                Ntk_NetworkGlobalMddPrintErrorTrace( Ntk_NetworkReadMvsisOut(pNet1), dd, pVmx, bTrace, iOutput, psRootsByName[iOutput], nValue, psLeavesByName );
                if ( !fMadeupExdc )
                    fprintf( Ntk_NetworkReadMvsisOut(pNet1), " is not in CUR + exdc(CUR)" );
                else
                    fprintf( Ntk_NetworkReadMvsisOut(pNet1), " is not in CUR + exdc(CUR)" );
                fprintf( Ntk_NetworkReadMvsisOut(pNet1), ".\n" );

                Cudd_RecursiveDeref( dd, bTrace );
                fErrorTrace = 1;
            }

            Ntk_NetworkGlobalMddDeref( dd, pValuesOut, nOuts, ppMddsSum );          
        }

        if ( !fAnswer )
            fprintf( Ntk_NetworkReadMvsisOut(pNet1), "Conclusion:    There is no containment between CUR and EXT.\n" );
    }

    if ( pNet1->pNetExdc && pNet2->pNetExdc && !fMadeupExdc )
    {
        if ( Ntk_NetworkGlobalMddCheckEquality( dd, pValuesOut, nOuts, ppMddsExdc1, ppMddsExdc2 ) )         
            fprintf( Ntk_NetworkReadMvsisOut(pNet1), "Equivalence:   exdc(CUR) = exdc(EXT).\n" );
        else if ( Ntk_NetworkGlobalMddCheckContainment( dd, pValuesOut, nOuts, ppMddsExdc1, ppMddsExdc2 ) ) 
            fprintf( Ntk_NetworkReadMvsisOut(pNet1), "Containment:   exdc(CUR) < exdc(EXT).\n" );
        else if ( Ntk_NetworkGlobalMddCheckContainment( dd, pValuesOut, nOuts, ppMddsExdc2, ppMddsExdc1 ) ) 
            fprintf( Ntk_NetworkReadMvsisOut(pNet1), "Containment:   exdc(EXT) < exdc(CUR).\n" );
        else
            fprintf( Ntk_NetworkReadMvsisOut(pNet1), "Non-equival:   exdc(CUR) != exdc(EXT).\n" );
    }
    if ( fVerbose )
    {
        PRT( "Total", clock() - clk );
    }

    // print the variable names if necessary
    if ( fErrorTrace )
    {
        fprintf( Ntk_NetworkReadMvsisOut(pNet1), "The ordering of inputs is {" );
        for ( i = 0; i < Vm_VarMapReadVarsInNum( pVm ); i++ )
            fprintf( Ntk_NetworkReadMvsisOut(pNet1), " %s", psLeavesByName[i] );
        fprintf( Ntk_NetworkReadMvsisOut(pNet1), " }" );
        fprintf( Ntk_NetworkReadMvsisOut(pNet1), "\n" );
    }

    // dereference the global functions
    Ntk_NetworkGlobalMddDeref( dd, pValuesOut, nOuts, ppMdds1 );            
    Ntk_NetworkGlobalMddDeref( dd, pValuesOut, nOuts, ppMdds2 );            
    Ntk_NetworkGlobalMddDeref( dd, pValuesOut, nOuts, ppMddsExdc1 );            
    Ntk_NetworkGlobalMddDeref( dd, pValuesOut, nOuts, ppMddsExdc2 );    
    
    // remove the temporary manager
    Extra_StopManager( dd );
    // undo the names and values
    FREE( psLeavesByName );
    FREE( psRootsByName );
    FREE( pValuesOut );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

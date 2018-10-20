/**CFile****************************************************************

  FileName    [verVerify.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [The core of the verification engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: verVerify.c,v 1.5 2004/05/12 04:30:15 alanmi Exp $]

***********************************************************************/

#include "verInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Ver_NetworkVerifyVariables( Ntk_Network_t * pNet1, Ntk_Network_t * pNet2, int fPoOnly, int fVerbose );
static void Ver_NetworkVerifyFunctions( Ntk_Network_t * pNet1, Ntk_Network_t * pNet2, int fPoOnly, int fVerbose );
static void Ver_NetworkVerifyStrashed( Sh_Manager_t * pShMan, Sh_Network_t * pShNet, int fVerbose );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////
    
/**Function*************************************************************

  Synopsis    [The top level verification procedure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ver_NetworkVerifySatComb( Ntk_Network_t * pNet1, Ntk_Network_t * pNet2, int fVerbose )
{
    // verify the identity of network CI/COs
    if ( !Ver_NetworkVerifyVariables( pNet1, pNet2, 0, fVerbose ) )
        return;
    // verify the functionality
    Ver_NetworkVerifyFunctions( pNet1, pNet2, 0, fVerbose );
}
    
/**Function*************************************************************

  Synopsis    [The top level verification procedure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ver_NetworkVerifySatSeq( Ntk_Network_t * pNet1, Ntk_Network_t * pNet2, int nFrames, int fVerbose )
{
    Ntk_Network_t * pNet1s, * pNet2s;

    // check if networks have latches
    if ( Ntk_NetworkReadLatchNum(pNet1) == 0 &&
         Ntk_NetworkReadLatchNum(pNet2) == 0 )
    {
        fprintf( Ntk_NetworkReadMvsisOut(pNet1), "Networks do not have latches. Sequential verification is not performed.\n" );
        return;
    }
    if ( Ntk_NetworkReadLatchNum(pNet1) == 0 ||
         Ntk_NetworkReadLatchNum(pNet2) == 0 )
    {
        fprintf( Ntk_NetworkReadMvsisOut(pNet1), "One of the networks does not have latches. Sequential verification is not performed.\n" );
        return;
    }

    // verify the identity of network CI/COs
    if ( !Ver_NetworkVerifyVariables( pNet1, pNet2, 1, fVerbose ) )
        return;

    // verify the functionality
    pNet1s = Ntk_NetworkDeriveTimeFrames( pNet1, nFrames );
    Ntk_NetworkMakeCombinational( pNet1s );
    pNet2s = Ntk_NetworkDeriveTimeFrames( pNet2, nFrames );
    Ntk_NetworkMakeCombinational( pNet2s );

    Ver_NetworkVerifyFunctions( pNet1s, pNet2s, 0, fVerbose );

    Ntk_NetworkDelete( pNet1s );
    Ntk_NetworkDelete( pNet2s );
}


/**Function*************************************************************

  Synopsis    [Verifies the CIs/COs of the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ver_NetworkVerifyVariables( Ntk_Network_t * pNet1, Ntk_Network_t * pNet2, int fPoOnly, int fVerbose )
{
    Ntk_Node_t * pNode1, * pNode2;

    // make sure that networks have the same number of inputs/outputs/latches
    if ( fPoOnly )
    {
        if ( Ntk_NetworkReadCiNum(pNet1) - Ntk_NetworkReadLatchNum(pNet1) != 
             Ntk_NetworkReadCiNum(pNet2) - Ntk_NetworkReadLatchNum(pNet2) )
        {
            fprintf( Ntk_NetworkReadMvsisOut(pNet1), "Networks have different number of PIs. Verification is not performed.\n" );
            return 0;
        }
        if ( Ntk_NetworkReadCoNum(pNet1) - Ntk_NetworkReadLatchNum(pNet1) != 
             Ntk_NetworkReadCoNum(pNet2) - Ntk_NetworkReadLatchNum(pNet2) )
        {
            fprintf( Ntk_NetworkReadMvsisOut(pNet1), "Networks have different number of POs. Verification is not performed.\n" );
            return 0;
        }
    }
    else
    {
        if ( Ntk_NetworkReadCiNum(pNet1) != Ntk_NetworkReadCiNum(pNet2) )
        {
            fprintf( Ntk_NetworkReadMvsisOut(pNet1), "Networks have different number of CIs. Verification is not performed.\n" );
            return 0;
        }
        if ( Ntk_NetworkReadCoNum(pNet1) != Ntk_NetworkReadCoNum(pNet2) )
        {
            fprintf( Ntk_NetworkReadMvsisOut(pNet1), "Networks have different number of COs. Verification is not performed.\n" );
            return 0;
        }
        if ( Ntk_NetworkReadLatchNum(pNet1) != Ntk_NetworkReadLatchNum(pNet2) )
        {
            fprintf( Ntk_NetworkReadMvsisOut(pNet1), "Networks have different number of latches. Verification is not performed.\n" );
            return 0;
        }
    }

    // make sure that the names of the inputs/outputs/latches are matching
    Ntk_NetworkForEachCi( pNet1, pNode1 )
    {
        if ( fPoOnly && pNode1->Subtype == MV_BELONGS_TO_LATCH )
            continue;
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
        if ( fPoOnly && pNode1->Subtype == MV_BELONGS_TO_LATCH )
            continue;
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
        if ( fPoOnly && pNode1->Subtype == MV_BELONGS_TO_LATCH )
            continue;
        if ( Ntk_NetworkFindNodeByName( pNet1, pNode1->pName ) == NULL )
        {
            fprintf( Ntk_NetworkReadMvsisOut(pNet1), "<%s> is a CI of the second network but not of the first. Verification is not performed.\n", pNode1->pName );
            return 0;
        }
    }
    Ntk_NetworkForEachCo( pNet2, pNode1 )
    {
        if ( fPoOnly && pNode1->Subtype == MV_BELONGS_TO_LATCH )
            continue;
        if ( Ntk_NetworkFindNodeByName( pNet1, pNode1->pName ) == NULL )
        {
            fprintf( Ntk_NetworkReadMvsisOut(pNet1), "<%s> is a CO of the second network but not of the first. Verification is not performed.\n", pNode1->pName );
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
void Ver_NetworkVerifyFunctions( Ntk_Network_t * pNet1, Ntk_Network_t * pNet2, int fPoOnly, int fVerbose )
{
    Sh_Manager_t * pShMan;
    Sh_Network_t * pShNet;
    Sh_Node_t * pShOut;

    if ( !Ntk_NetworkIsBinary(pNet1) || !Ntk_NetworkIsBinary(pNet2) )
    {
        printf( "Currently can only verify two binary networks.\n" );
        return;
    }

    // strash both networks into the same ShManager
    pShMan = Sh_ManagerCreate( Ntk_NetworkReadCiNum(pNet1), 100000, 1 );
    Sh_ManagerTwoLevelEnable( pShMan );
    pShNet = Ver_NetworkStrashMiter( pShMan, pNet1, pNet2, fPoOnly );
    pShOut = Sh_NetworkReadOutput( pShNet, 0 );

    if ( !shNodeIsConst(pShOut) )
        Ver_NetworkVerifyStrashed( pShMan, pShNet, fVerbose );
    else //if ( fVerbose )
        printf( "The networks are equivalent after strashing.\n" );

    Sh_NetworkFree( pShNet );
    Sh_ManagerFree( pShMan, 1 );
}

    
/**Function*************************************************************

  Synopsis    [Proves that the miter cone is never equal to 1.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ver_NetworkVerifyStrashed( Sh_Manager_t * pShMan, Sh_Network_t * pShNet, int fVerbose )
{
    Ver_Manager_t * pMan;
    int RetValue, i, clk;

    // order the nodes in the network and back-annotate eash node to its place
    Sh_NetworkCollectInternal( pShNet );
    for ( i = 0; i < pShNet->nNodes; i++ )
        pShNet->ppNodes[i]->pData2 = i;

    pMan = ALLOC( Ver_Manager_t, 1 );
    memset( pMan, 0, sizeof(Ver_Manager_t) );
    pMan->pShMan = pShMan;
    pMan->pShNet = pShNet;
    pMan->fMiter = 1;

    clk = clock();
    if ( Ver_VerificationSimulate( pMan, 10 ) == -1 )
    {
        if ( fVerbose ) { PRT( "Simulation time", clock() - clk ); }
        printf( "The networks are not equivalent after %d rounds of simulation.\n", pMan->nRounds );
    }
    else
    {
        if ( fVerbose )
        {
            printf( "The strashed miter network is composed of %d AND gates.\n", pShNet->nNodes );
            printf( "Simulation detected %d candidate equivalence classes.\n", pMan->nClasses );
        }
        if ( fVerbose ) { PRT( "Simulation time", clock() - clk ); }

        clk = clock();
        RetValue = Ver_NetworkEquivCheckSat( pMan );
        if ( fVerbose ) { PRT( "SAT solver time", clock() - clk ); }

        if ( RetValue == 1 )
            printf( "Verification successful.\n" );
        else
            printf( "Verification failed.\n" );
    }
    FREE( pMan->ppClasses );
    FREE( pMan );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



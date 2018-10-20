/**CFile****************************************************************

  FileName    [verReach.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Experimental reachability analisys.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: verReach.c,v 1.6 2004/05/12 04:30:15 alanmi Exp $]

***********************************************************************/

#include "verInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs reachability analysis.]

  Description []
               
  SideEffects []

  SeeAlso     []
 
***********************************************************************/
void Ver_NetworkReachability( Ntk_Network_t * pNet, int nItersLimit, int nSquar, int fAlgorithm )
{
    Sh_Manager_t * pMan;
    DdManager * dd;
    Sh_Node_t ** pgVars, ** pgVarsCs, ** pgVarsNs, ** pgVarsIs;
    Sh_Node_t * gRel, * gReached, * gFront, * gNext, * gTemp;
    Sh_Node_t * gInitCs, * gInitNs, * gCube;
    int nInputs, nLatches, nIters, s, clk;
    int fEnableCollapse = fAlgorithm;

    // get parameters
    nInputs  = Ntk_NetworkReadCiNum(pNet) - Ntk_NetworkReadLatchNum(pNet);
    nLatches = Ntk_NetworkReadLatchNum(pNet);
    // start the manager
    pMan     = Sh_ManagerCreate( nInputs + 3 * nLatches, 1000000, 1 );
    dd       = Cudd_Init( nInputs + 3 * nLatches, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );

//    Sh_ManagerTwoLevelEnable( pMan );
    pgVars   = Sh_ManagerReadVars( pMan );      // the variables of the manager
    pgVarsCs = pgVars + nInputs;                // the current state variables
    pgVarsNs = pgVars + nInputs + nLatches;     // the next state variables
    pgVarsIs = pgVars + nInputs + 2 * nLatches; // intermediate variables used for squaring

    // get the initial state and the relation
    gInitCs = Ver_NetworkStrashInitState( pMan, pNet );                    shRef( gInitCs );
Sh_NodeShow( pMan, gInitCs, "Init" );
    gRel    = Ver_NetworkStrashTransRelation( pMan, pNet );                shRef( gRel ); 
printf( "Monolithic relation: AI-nodes = %7d. Created AI-nodes = %7d\n", Sh_NodeCountNodes(pMan,gRel),  Sh_ManagerReadNodes(pMan) ); 
//Sh_NodeShow( pMan, gRel, "rel_aig" );
Sh_NodeShow( pMan, gRel, "rel" );

    // quantify the input variables from the relation
    gRel  = Sh_NodeExistAbstract( pMan, gTemp = gRel, pgVars, nInputs );   shRef( gRel );
    Sh_RecursiveDeref( pMan, gTemp );
printf( "Quantified relation: AI-nodes = %7d. Created AI-nodes = %7d\n", Sh_NodeCountNodes(pMan,gRel),  Sh_ManagerReadNodes(pMan) ); 

    if ( nSquar > 0 )
    {
        // augment the relation with the self-loop in the initial state (Donald's idea)
        gInitNs = Sh_NodeVarReplace( pMan, gInitCs, pgVarsCs, pgVarsNs, nLatches );   shRef( gInitNs );
        gCube = Sh_NodeAnd( pMan, gInitCs, gInitNs );                                 shRef( gCube );
        gRel = Sh_NodeOr( pMan, gTemp = gRel, gCube );                                shRef( gRel );
        Sh_RecursiveDeref( pMan, gTemp );
        Sh_RecursiveDeref( pMan, gCube );
        Sh_RecursiveDeref( pMan, gInitNs );
    }

    // perform iterative squaring if the user requested it
    for ( s = 0; s < nSquar; s++ )
    {
        gRel = Sh_RelationSquare( pMan, gTemp = gRel, pgVarsCs, pgVarsNs, pgVarsIs, nLatches ); shRef( gRel );
        Sh_RecursiveDeref( pMan, gTemp );
printf( "Squared relation   : AI-nodes = %7d. Created AI-nodes = %7d\n", Sh_NodeCountNodes(pMan,gRel),  Sh_ManagerReadNodes(pMan) ); 
    }
Sh_NodeWriteBlif( pMan, gRel, "rel.blif" );
Sh_NodeShow( pMan, gRel, "relQ" );

    // perform reachability analisys
    gReached = gInitCs;         shRef( gReached );      
    gFront   = gInitCs;         shRef( gFront );    
    for ( nIters = 0; nIters < nItersLimit; nIters++ )
    {
clk = clock();
        // compute the states reachable in one iteration from gFront
        gNext = Sh_NodeAndAbstract( pMan, gRel, gFront, pgVarsCs, nLatches );   shRef( gNext ); 
        Sh_RecursiveDeref( pMan, gFront );
        // remap these states into the current state vars
        gNext = Sh_NodeVarReplace( pMan, gTemp = gNext, pgVarsCs, pgVarsNs, nLatches );  shRef( gNext );           
        Sh_RecursiveDeref( pMan, gTemp );
        // get the new front (that is, the states that are reached for the first time)
        gFront = Sh_NodeAnd( pMan, gNext, Sh_Not(gReached) );      shRef( gFront );


        if ( fEnableCollapse )
        {
        // get the new gFront representation
printf( "%8d ->", Sh_NodeCountNodes(pMan, gFront) );
        gFront = Sh_NodeCollapseBdd( dd, pMan, gTemp = gFront );   shRef( gFront );
        Sh_RecursiveDeref( pMan, gTemp );
printf( "%8d  ", Sh_NodeCountNodes(pMan, gFront) );
//PRT( "Time", clock() - clk );
        }

//        if ( nIters == 30 )
//        {
//            Sh_RecursiveDeref( pMan, gNext );
//            Sh_NodeWriteBlif( pMan, gReached, "reached30.blif" );
//            break;
//        }


        // check if there are new states
        if ( gFront == Sh_Not(pMan->pConst1) )
        {
            Sh_RecursiveDeref( pMan, gNext );
            printf( "Fixed point is reached after %d iterations.\n", nIters );
            break;
        }
        // add to the reached states
//        gReached = Sh_NodeOr( pMan, gTemp = gReached, gNext );   shRef( gReached ); 
        gReached = Sh_NodeOr( pMan, gTemp = gReached, gFront );   shRef( gReached ); 
        Sh_RecursiveDeref( pMan, gNext );
        Sh_RecursiveDeref( pMan, gTemp );
printf( "Iter %2d : Nodes = %7d.  ", nIters+1, Sh_NodeCountNodes(pMan, gReached) );
PRT( "Time", clock() - clk );
    }
    if ( nIters == nItersLimit )
        fprintf( stdout, "Reachability analysis stopped after %d iterations.\n", nIters );
Sh_NodeWriteBlif( pMan, gReached, "reached.blif" );
Sh_NodeShow( pMan, gReached, "reached" );

    // at this point the reached states are computed
    Sh_RecursiveDeref( pMan, gReached );
    Sh_RecursiveDeref( pMan, gFront );
    Sh_RecursiveDeref( pMan, gInitCs );
    Sh_RecursiveDeref( pMan, gRel );

    printf( "The number of nodes in the manager = %d.\n", Sh_ManagerReadNodes(pMan) ); 
    Sh_ManagerFree( pMan, 1 );
    // quit the BDD manager
    Extra_StopManager( dd );
}


/*
//Sh_NodeWriteBlif( pMan, gRel, "temp_rel.blif" );
//gRel = Sh_NodeCollapse( pMan, gRel );
//gTemp = Sh_GraphRewrite( pMan, NULL );
//Sh_NodeShow( pMan, gRel );
//Sh_GraphRewrite( pMan, gRel );
//Sh_NodeResynthesize( pMan, gRel );
//Sh_NodeShow( pMan, gRel );
//Sh_NodeWriteBlif( pMan, gRel );

if ( nIters == 11 )
{
    char Buffer[100];
    Sh_Node_t * pRes;
    Sh_NodeShow( pMan, gReached );
    pRes = Sh_NodeResynthesize( pMan, gReached );
    Sh_NodeShow( pMan, pRes );
    sprintf( Buffer, "iter%02d.blif", nIters+1 );
    Sh_NodeWriteBlif( pMan, gReached, Buffer );
}
*/

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



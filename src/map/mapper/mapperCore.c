/**CFile****************************************************************

  FileName    [mapperCore.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic technology mapping engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: mapperCore.c,v 1.17 2005/07/08 01:01:24 alanmi Exp $]

***********************************************************************/

#include "mapperInt.h"
#include "resm.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs technology mapping for the given object graph.]

  Description [The object graph is stored in the mapping manager.
  First, the AND nodes that fanout into POs are collected in the DFS order.
  Two preprocessing steps are performed: the k-feasible cuts are computed 
  for each node and the truth tables are computed for each cut. Next, the 
  delay-optimal matches are assigned for each node, followed by several 
  iterations of area recoveryd: using area flow (global optimization) 
  and using exact area at a node (local optimization).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_Mapping( Map_Man_t * p )
{
    int fUseAreaFlow           = 1;
    int fUseExactArea          = 1;
    int fUseExactAreaWithPhase = 1;
    int clk;

    //////////////////////////////////////////////////////////////////////
    // perform pre-mapping computations
    // collect the nodes reachable from POs in the DFS order (including the choices)
    p->vAnds = Map_MappingDfs( p, 1 );
    if ( p->fVerbose )
        Map_MappingReportChoices( p ); 
    Map_MappingSetChoiceLevels( p ); // should always be called before mapping!
    // compute the cuts of nodes in the DFS order
    clk = clock();
    Map_MappingCuts( p );
    p->timeCuts = clock() - clk;
    // derive the truth tables 
    clk = clock();
    Map_MappingTruths( p );
    p->timeTruth = clock() - clk;
    //////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////
    // compute the minimum-delay mapping
    clk = clock();
    p->fMappingMode = 0;
    if ( !Map_MappingMatches( p ) )
        return 0;
    p->timeMatch = clock() - clk;
    // compute the references and collect the nodes used in the mapping
    Map_MappingSetRefs( p );
    p->AreaBase = Map_MappingGetArea( p, p->vMapping );
if ( p->fVerbose )
{
printf( "Delay    : FanViols = %5d  Flow = %11.1f  Area = %11.1f  %4.1f %%   ", 
                    0, Map_MappingGetAreaFlow(p), p->AreaBase, 0.0 );
PRT( "Time", p->timeMatch );
}
    //////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////
    // perform area recovery using area flow
    clk = clock();
    if ( fUseAreaFlow )
    {
        // compute the required times and the fanouts
        Map_TimeComputeRequiredGlobal( p );
        // recover area flow
        p->fMappingMode = 1;
        Map_MappingMatches( p );
        // compute the references and collect the nodes used in the mapping
        Map_MappingSetRefs( p );
        p->AreaFinal = Map_MappingGetArea( p, p->vMapping );
if ( p->fVerbose )
{
printf( "AreaFlow : FanViols = %5d  Flow = %11.1f  Area = %11.1f  %4.1f %%   ", 
                    0, Map_MappingGetAreaFlow(p), p->AreaFinal, 
                    100.0*(p->AreaBase-p->AreaFinal)/p->AreaBase );
PRT( "Time", clock() - clk );
}
    }
    p->timeArea += clock() - clk;
    //////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////
    // perform area recovery using exact area
    clk = clock();
    if ( fUseExactArea )
    {
        // compute the required times and the fanouts
        Map_TimeComputeRequiredGlobal( p );
        // recover area flow
        p->fMappingMode = 2;
        Map_MappingMatches( p );
        // compute the references and collect the nodes used in the mapping
        Map_MappingSetRefs( p );
        p->AreaFinal = Map_MappingGetArea( p, p->vMapping );
if ( p->fVerbose )
{
printf( "Area     : FanViols = %5d  Flow = %11.1f  Area = %11.1f  %4.1f %%   ", 
                    0, 0.0, p->AreaFinal, 
                    100.0*(p->AreaBase-p->AreaFinal)/p->AreaBase );
PRT( "Time", clock() - clk );
}
    }
    p->timeArea += clock() - clk;
    //////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////
    // perform area recovery using exact area
    clk = clock();
    if ( fUseExactAreaWithPhase )
    {
        // compute the required times and the fanouts
        Map_TimeComputeRequiredGlobal( p );
        // recover area flow
        p->fMappingMode = 3;
        Map_MappingMatches( p );
        // compute the references and collect the nodes used in the mapping
        Map_MappingSetRefs( p );
        p->AreaFinal = Map_MappingGetArea( p, p->vMapping );
if ( p->fVerbose )
{
printf( "Area     : FanViols = %5d  Flow = %11.1f  Area = %11.1f  %4.1f %%   ", 
                    0, 0.0, p->AreaFinal, 
                    100.0*(p->AreaBase-p->AreaFinal)/p->AreaBase );
PRT( "Time", clock() - clk );
}
    }
    p->timeArea += clock() - clk;
    //////////////////////////////////////////////////////////////////////

    // print the arrival times of the latest outputs
    if ( p->fVerbose )
        Map_MappingPrintOutputArrivals( p );
    return 1;
}

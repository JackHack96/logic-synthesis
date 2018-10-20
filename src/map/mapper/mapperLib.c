/**CFile****************************************************************

  FileName    [mapperLib.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic technology mapping engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: mapperLib.c,v 1.7 2005/07/08 01:01:25 alanmi Exp $]

***********************************************************************/

#include "mapperInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Reads in the supergate library and prepares it for use.]

  Description [The supergates library comes in a .super file. This file
  contains descriptions of supergates along with some relevant information.
  This procedure reads the supergate file, canonicizes the supergates,
  and constructs an additional lookup table, which can be used to map
  truth tables of the cuts into the pair (phase, supergate). The phase
  indicates how the current truth table should be phase assigned to 
  match the canonical form of the supergate. The resulting phase is the
  bitwise EXOR of the phase needed to canonicize the supergate and the
  phase needed to transform the truth table into its canonical form.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Map_SuperLib_t * Map_SuperLibCreate( char * pFileName, char * pExcludeFile, bool fAlgorithm, bool fVerbose )
{
    Map_SuperLib_t * p;
    int clk;

    // start the supergate library
    p = ALLOC( Map_SuperLib_t, 1 );
    memset( p, 0, sizeof(Map_SuperLib_t) );
    p->pName     = pFileName;
    p->fVerbose  = fVerbose;
    p->mmSupers  = Extra_MmFixedStart( sizeof(Map_Super_t), 10000, 100 );
    p->mmEntries = Extra_MmFixedStart( sizeof(Map_HashEntry_t), 10000, 100 );
    p->mmForms   = Extra_MmFlexStart( 10000, 100 );
    Map_MappingSetupTruthTables( p->uTruths );

    // start the hash table
    p->tTableC = Map_SuperTableCreate( p );
    p->tTable  = Map_SuperTableCreate( p );

    // read the supergate library from file
clk = clock();
    if ( fAlgorithm )
    {
        if ( !Map_LibraryReadTree( p, pFileName, pExcludeFile ) )
        {
            Map_SuperLibFree( p );
            return NULL;
        }
    }
    else
    {
        if ( pExcludeFile != 0 )
        {
            printf ("Error: Exclude file support not present for old format. Stop.\n");
            return NULL;
        }
        if ( !Map_LibraryRead( p, pFileName ) )
        {
            Map_SuperLibFree( p );
            return NULL;
        }
    }
    assert( p->nVarsMax > 0 );

    // report the stats
if ( fVerbose ) {
    printf( "Loaded %d unique %d-input supergates from \"%s\".  ", 
        p->nSupersReal, p->nVarsMax, pFileName );
    PRT( "Time", clock() - clk );
}

    // assign the interver parameters
    p->pGateInv        = Mio_LibraryReadInv( p->pGenlib );
    p->tDelayInv.Rise  = Mio_LibraryReadDelayInvRise( p->pGenlib );
    p->tDelayInv.Fall  = Mio_LibraryReadDelayInvFall( p->pGenlib );
    p->tDelayInv.Worst = MAP_MAX( p->tDelayInv.Rise, p->tDelayInv.Fall );
    p->AreaInv         = Mio_LibraryReadAreaInv( p->pGenlib );

    // assign the interver supergate
    p->pSuperInv = (Map_Super_t *)Extra_MmFixedEntryFetch( p->mmSupers );
    memset( p->pSuperInv, 0, sizeof(Map_Super_t) );
    p->pSuperInv->Num         = -1;
    p->pSuperInv->nGates      =  1;
    p->pSuperInv->nFanins     =  1;
    p->pSuperInv->nFanLimit   = 10;
    p->pSuperInv->pFanins[0]  = p->ppSupers[0];
    p->pSuperInv->pRoot       = p->pGateInv;
    p->pSuperInv->Area        = p->AreaInv;
    p->pSuperInv->tDelayMax   = p->tDelayInv;
    p->pSuperInv->tDelaysR[0].Rise = MAP_NO_VAR;
    p->pSuperInv->tDelaysR[0].Fall = p->tDelayInv.Rise;
    p->pSuperInv->tDelaysF[0].Rise = p->tDelayInv.Fall;
    p->pSuperInv->tDelaysF[0].Fall = MAP_NO_VAR;
    return p;
}


/**Function*************************************************************

  Synopsis    [Deallocates the supergate library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_SuperLibFree( Map_SuperLib_t * p )
{
    if ( p == NULL ) return;
    if ( p->pGenlib )
    {
        extern Mio_Library_t * s_pLib;
        if ( s_pLib == p->pGenlib )
            s_pLib = NULL;
        Mio_LibraryDelete( p->pGenlib );
    }
    if ( p->tTableC )
        Map_SuperTableFree( p->tTableC );
    if ( p->tTable )
        Map_SuperTableFree( p->tTable );
    Extra_MmFixedStop( p->mmSupers, 0 );
    Extra_MmFixedStop( p->mmEntries, 0 );
    Extra_MmFlexStop( p->mmForms, 0 );
    FREE( p->ppSupers );
    FREE( p );
}

/**Function*************************************************************

  Synopsis    [Derives the library from the genlib library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_SuperLibDeriveFromGenlib( Mio_Library_t * pLib )
{
    Mv_Frame_t * pMvsis = Mv_FrameGetGlobalFrame();
    extern Map_SuperLib_t * s_pSuperLib;
    char * pNameGeneric;
    char FileNameSuper[100];
    char CommandSuper[500];
    char CommandRead[500];

    if ( pLib == NULL )
        return 0;

    // get the file name with the library
    pNameGeneric = Extra_FileNameGeneric( Mio_LibraryReadName(pLib) );
    sprintf( FileNameSuper, "%s.super", pNameGeneric );
    free( pNameGeneric );

    sprintf( CommandSuper,  "super -l 1 -i 5 -d 10000000 -a 10000000 -t 100 %s", Mio_LibraryReadName(pLib) ); 
    if ( Cmd_CommandExecute( pMvsis, CommandSuper ) )
    {
        fprintf( stdout, "Cannot execute command \"%s\".\n", CommandSuper );
        return 0;
    }

    sprintf( CommandRead,  "read_super %s", FileNameSuper ); 
    if ( Cmd_CommandExecute( pMvsis, CommandRead ) )
    {
#ifdef WIN32
        _unlink( FileNameSuper );
#else
        unlink( FileNameSuper );
#endif
        fprintf( stdout, "Cannot execute command \"%s\".\n", CommandRead );
        return 0;
    }

/* // don't remove the intermediate file
#ifdef WIN32
    _unlink( FileNameSuper );
#else
    unlink( FileNameSuper );
#endif
*/
     return 1;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



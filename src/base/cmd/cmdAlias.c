/**CFile****************************************************************

  FileName    [cmdAlias.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Procedures dealing with aliases in the command package.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: cmdAlias.c,v 1.8 2003/05/27 23:14:11 alanmi Exp $]

***********************************************************************/

#include "mv.h"
#include "mvInt.h"
#include "cmdInt.h"

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
void CmdCommandAliasAdd( Mv_Frame_t * pMvsis, char * sName, int argc, char ** argv )
{
    Mv_Alias * pAlias;
    int fStatus, i;

    pAlias = ALLOC(Mv_Alias, 1);
    pAlias->sName = util_strsav(sName);
    pAlias->argc = argc;
    pAlias->argv = ALLOC(char *, pAlias->argc);
    for(i = 0; i < argc; i++) 
        pAlias->argv[i] = util_strsav(argv[i]);
    fStatus = avl_insert( pMvsis->tAliases, pAlias->sName, (char *) pAlias );
    assert(!fStatus);  
}

/**Function********************************************************************

  Synopsis    [required]

  Description [optional]

  SideEffects [required]

  SeeAlso     [optional]

******************************************************************************/
void CmdCommandAliasPrint( Mv_Frame_t * pMvsis, Mv_Alias * pAlias )
{
    int i;
    fprintf(pMvsis->Out, "%s\t", pAlias->sName);
    for(i = 0; i < pAlias->argc; i++) 
        fprintf( pMvsis->Out, " %s", pAlias->argv[i] );
    fprintf( pMvsis->Out, "\n" );
}

/**Function********************************************************************

  Synopsis    [required]

  Description [optional]

  SideEffects [required]

  SeeAlso     [optional]

******************************************************************************/
char * CmdCommandAliasLookup( Mv_Frame_t * pMvsis, char * sCommand )
{
  Mv_Alias * pAlias;
  char * value;
  if (!avl_lookup( pMvsis->tAliases, sCommand, &value)) 
    return sCommand;
  pAlias = (Mv_Alias *) value;
  return pAlias->argv[0];
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void CmdCommandAliasFree( Mv_Alias * pAlias )
{
    CmdFreeArgv( pAlias->argc, pAlias->argv );
    FREE(pAlias->sName);    
    FREE(pAlias);
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



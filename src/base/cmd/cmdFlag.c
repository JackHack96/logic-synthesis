/**CFile****************************************************************

  FileName    [cmdFlag.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    []

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: cmdFlag.c,v 1.9 2003/05/27 23:14:11 alanmi Exp $]

***********************************************************************/

#include "mv.h"
#include "mvInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////


/**Function********************************************************************

  Synopsis    [Looks up value of flag in table of named values.]

  Description [The command parser maintains a table of named values.  These
  are manipulated using the 'set' and 'unset' commands.  The value of the
  named flag is returned, or NIL(char) is returned if the flag has not been
  set.]

  SideEffects []

******************************************************************************/
char * Cmd_FlagReadByName( Mv_Frame_t * pMvsis, char * flag )
{
  char *value;

  if (avl_lookup(pMvsis->tFlags, flag, &value)) {
    return value;
  }
  else {
    return NIL(char);
  }
}


/**Function********************************************************************

  Synopsis    [Updates a set value by calling instead of set command.]

  Description [Updates a set value by calling instead of set command.]

  SideEffects []

******************************************************************************/
void Cmd_FlagUpdateValue( Mv_Frame_t * pMvsis, char * key, char * value )
{
  char *oldValue, *newValue;

  if (!key)
    return;
  if (value)
    newValue = util_strsav(value);
  else
    newValue = util_strsav("");

  if (avl_delete(pMvsis->tFlags, &key, &oldValue))
    FREE(oldValue);

  avl_insert(pMvsis->tFlags, key, newValue);
}


/**Function********************************************************************

  Synopsis    [Deletes a set value by calling instead of unset command.]

  Description [Deletes a set value by calling instead of unset command.]

  SideEffects []

******************************************************************************/
void Cmd_FlagDeleteByName( Mv_Frame_t * pMvsis, char * key )
{
  char *value;

  if (!key)
    return;

  if (avl_delete(pMvsis->tFlags, &key, &value)) {
    FREE(key);
    FREE(value);
  }
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



/**CFile****************************************************************

  FileName    [cmdApi.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [External procedures of the command package.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: cmdApi.c,v 1.19 2004/01/06 21:02:51 alanmi Exp $]

***********************************************************************/

#include "mv.h"
#include "mvInt.h"
#include "cmdInt.h"
#include "ntkInt.h"

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
void Cmd_CommandAdd( Mv_Frame_t * pMvsis, char * sGroup, char * sName, void * pFunc, int fChanges )
{
    char * key, * value;
    Mv_Command * pCommand;
    int fStatus;

    key = sName;
    if ( avl_delete( pMvsis->tCommands, &key, &value ) ) 
    {
        // delete existing definition for this command 
        fprintf( pMvsis->Err, "Cmd warning: redefining '%s'\n", sName );
        CmdCommandFree( (Mv_Command *)value );
    }

    // create the new command
    pCommand = ALLOC( Mv_Command, 1 );
    pCommand->sName   = util_strsav( sName );
    pCommand->sGroup  = util_strsav( sGroup );
    pCommand->pFunc   = pFunc;
    pCommand->fChange = fChanges;
    fStatus = avl_insert( pMvsis->tCommands, sName, (char *)pCommand );
    assert( !fStatus );  // the command should not be in the table
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cmd_CommandExecute( Mv_Frame_t * pMvsis, char * sCommand )
{
    int fStatus = 0, argc, loop;
    char * sCommandNext, **argv;

    if ( !pMvsis->fAutoexac ) 
	Cmd_HistoryAddCommand(pMvsis, sCommand);
    sCommandNext = sCommand;
    do 
    {
       	sCommandNext = CmdSplitLine( pMvsis, sCommandNext, &argc, &argv );
		loop = 0;
		fStatus = CmdApplyAlias( pMvsis, &argc, &argv, &loop );
		if ( fStatus == 0 ) 
			fStatus = CmdCommandDispatch( pMvsis, argc, argv );
       	CmdFreeArgv( argc, argv );
    } 
    while ( fStatus == 0 && *sCommandNext != '\0' );
    return fStatus;
}
	
/**Function*************************************************************

  Synopsis    [Collects the nodes mentioned on the command line.]

  Description [The nodes are linked using internal linked list. 
  The procedure returns the number of nodes collected.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cmd_CommandGetNodes( Mv_Frame_t * pMvsis, int argc, char ** argv, int fCollectCis )
{
    Ntk_Network_t * pNet;
    Ntk_Node_t * pNode, * pFanin;
    int i;

    pNet = pMvsis->pNetCur;
    Ntk_NetworkStartSpecial( pNet );
    if ( argc == 1 || strcmp(argv[1], "*")==0)
    {
        // collect the CI nodes
        for ( i = 1; i < pNet->nIds; i++ )
        {
            pNode = pNet->pId2Node[i];
            if ( pNode == NULL )
                continue;

            if ( pNode->Type == MV_NODE_CI )
            {
                if ( fCollectCis )
                    Ntk_NetworkAddToSpecial( pNode );
            }
            else if ( pNode->Type == MV_NODE_CO )
            {
                pFanin = Ntk_NodeReadFaninNode( pNode, 0 );
                if ( Ntk_NodeHasCoName(pFanin) )
                    Ntk_NetworkAddToSpecial( pFanin );
                else
                    Ntk_NetworkAddToSpecial( pNode );
            }
            else if ( pNode->Type == MV_NODE_INT )
            {
                if ( !Ntk_NodeHasCoName(pNode) )
                    Ntk_NetworkAddToSpecial( pNode );
            }
            else
            {
                assert( 0 );
            }
        }
    }
    else
    {
        // collect the nodes by name
        for ( i = 1; i < argc; i++ )
        {
            pNode = Ntk_NetworkCollectNodeByName( pNet, argv[i], fCollectCis );
            if ( pNode )
                Ntk_NetworkAddToSpecial( pNode );
        }
    }
    // finish off the linked list with NULL
    Ntk_NetworkStopSpecial( pNet );
    return Ntk_NetworkCountSpecial( pNet );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



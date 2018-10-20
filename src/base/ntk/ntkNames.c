/**CFile****************************************************************

  FileName    [ntkNames.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Procedures working with node names.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkNames.c,v 1.19 2005/03/31 01:29:34 casem Exp $]

***********************************************************************/

#include "ntkInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Ntk_NetworkAssignNewName( Ntk_Node_t * pNode, int iNum, int nChars );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Get the printable output name of the node.]

  Description [If the node is the PI or the PO, returns its name.
  If the node has a single CO fanout the printable name is the CO name. 
  Otherwise, the current name of the node is used, or if the current 
  name is NULL, a new unique name is created.]
               
  SideEffects [] 

  SeeAlso     []

***********************************************************************/
char * Ntk_NodeGetNamePrintable( Ntk_Node_t * pNode )
{
    // if the node name mode is short, return short name
    if ( Ntk_NetworkIsModeShort(pNode->pNet) )
        return Ntk_NodeGetNameShort(pNode);
    // otherwise, the node name mode is long
    return Ntk_NodeGetNameLong(pNode);
}

/**Function*************************************************************

  Synopsis    [Get the printable short name of the node.]

  Description []
               
  SideEffects [] 

  SeeAlso     []

***********************************************************************/
char * Ntk_NodeGetNameShort( Ntk_Node_t * pNode )
{
    Ntk_Node_t * pFanoutCo;
    // if the node has the only CO fanout, get the CO
    if ( pFanoutCo = Ntk_NodeHasCoName(pNode) )
        return Ntk_NodeGetNameUniqueShort( pFanoutCo );
    // assign the name if it was not found
    return Ntk_NodeGetNameUniqueShort( pNode );
}

/**Function*************************************************************

  Synopsis    [Get the printable long name of the node.]

  Description []
               
  SideEffects [] 

  SeeAlso     []

***********************************************************************/
char * Ntk_NodeGetNameLong( Ntk_Node_t * pNode )
{
    Ntk_Node_t * pFanoutCo;
    // if the only fanout of this node is a PO, write PO name
    if ( pFanoutCo = Ntk_NodeHasCoName(pNode) )
        return Ntk_NodeGetNameUniqueLong( pFanoutCo );
    // assign the name if it was not found
    return Ntk_NodeGetNameUniqueLong( pNode );
}

/**Function*************************************************************

  Synopsis    [Returns a unique name of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Ntk_NodeGetNameUniqueLong( Ntk_Node_t * pNode )
{
    static char NameBuffer[500];
    int Counter = 0;
    if ( pNode->pName )
        return pNode->pName;
    // get the name composed of the node ID
    // sprintf( NameBuffer, "[%d]", pNode->Id );
    sprintf( NameBuffer, "n_%d", pNode->Id );
    // if it is not unique, add a character at the end until it becomes unique
    while ( st_is_member( pNode->pNet->tName2Node, NameBuffer ) )
    {
        //sprintf( NameBuffer, "[%d%c%d]", pNode->Id, 'a' + Counter % 26, Counter / 26 );
        sprintf( NameBuffer, "n_%d_%c%d", pNode->Id, 'a' + Counter % 26, Counter / 26 );
        Counter++;
    }
    return NameBuffer;
}

/**Function*************************************************************

  Synopsis    [Returns a unique name of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Ntk_NodeGetNameUniqueShort( Ntk_Node_t * pNode )
{
    static char NameBuffer[500];
    int Number = pNode->Id - 1;
    // get the name composed of the node ID
    if ( Number < 26 )
        sprintf( NameBuffer, "%c", 'a' + Number % 26 );
    else
        sprintf( NameBuffer, "%c%d", 'a' + Number % 26, Number / 26 );
    return NameBuffer;
}

/**Function*************************************************************

  Synopsis    [Assign the name to the node.]

  Description [Assumes that the given name is unique and is already 
  in the memory manager of the network. If the name is not given, 
  a unique name is created and assigned.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Ntk_NodeAssignName( Ntk_Node_t * pNode, char * pName ) 
{
    char * pNameNew;
	assert( pNode->pName == NULL );
    if ( pName ) // the name is given
    {
        // make sure it is unique
        if ( st_is_member( pNode->pNet->tName2Node, pName ) )
        {
            assert( 0 );
        }
    }
    else // the name is not given
    {
        // get the unique name
        pNameNew = Ntk_NodeGetNameUniqueLong( pNode );
        // register this name
        pName = Ntk_NetworkRegisterNewName( pNode->pNet, pNameNew );
    }
    // assign the name
    st_insert( pNode->pNet->tName2Node, pName, (char *)pNode );
    pNode->pName = pName;
    return pName;
}

/**Function*************************************************************

  Synopsis    [Removes the node's name.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Ntk_NodeRemoveName( Ntk_Node_t * pNode ) 
{
    char * pName;
    assert( pNode->pName );
    // make sure the node is registered with this name
    if ( !st_is_member( pNode->pNet->tName2Node, pNode->pName ) )
    {
        assert( 0 );
    }
    // remove the name
    st_delete( pNode->pNet->tName2Node, &pNode->pName, NULL );
    pName = pNode->pName;
    pNode->pName = NULL;
    return pName;
}

/**Function*************************************************************

  Synopsis    [Compares the nodes alphabetically by name.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NodeCompareByNameAndId( Ntk_Node_t ** ppN1, Ntk_Node_t ** ppN2 )
{
    char * pName1 = (*ppN1)->pName;
    char * pName2 = (*ppN2)->pName;
//    assert( (*ppN1)->Id > 0 ); // node ID should be assigned by adding the node to the network
//    assert( (*ppN2)->Id > 0 ); // node ID should be assigned by adding the node to the network

    // if this is the same node, return right away
    if ( (*ppN1) == (*ppN2) )
        return 0;

    // make sure the comparison of nodes is valid
    assert( (*ppN1)->Id != (*ppN2)->Id );
    // two different nodes cannot have the same ID
    // if both of these IDs are 0, it means that both nodes are not in the network
    // note that it is okay if one node is not in the network 
    // in this case, its node ID is 0 but it still can be compared to other nodes

    if ( pName1 && pName2 )
        return strcmp( pName1, pName2 );
    if ( pName1 )
        return -1;
    if ( pName2 )
        return 1;
    if ( (*ppN1)->Id < (*ppN2)->Id )
        return -1;
    if ( (*ppN1)->Id > (*ppN2)->Id )
        return 1;
    assert( 0 );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Compares the nodes alphabetically by name.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NodeCompareByNameAndIdOrder( Ntk_Node_t ** ppN1, Ntk_Node_t ** ppN2 )
{
    char * pName1 = (*ppN1)->pName;
    char * pName2 = (*ppN2)->pName;
//    assert( (*ppN1)->Id > 0 ); // node ID should be assigned by adding the node to the network
//    assert( (*ppN2)->Id > 0 ); // node ID should be assigned by adding the node to the network

    // if this is the same node, return right away
    if ( (*ppN1) == (*ppN2) )
        return 0;

    if ( (*ppN1)->Type < (*ppN2)->Type )
        return -1;
    if ( (*ppN1)->Type > (*ppN2)->Type )
        return 1;

    // make sure the comparison of nodes is valid
    assert( (*ppN1)->Id != (*ppN2)->Id );
    // two different nodes cannot have the same ID
    // if both of these IDs are 0, it means that both nodes are not in the network
    // note that it is okay if one node is not in the network 
    // in this case, its node ID is 0 but it still can be compared to other nodes

    if ( pName1 && pName2 )
        return strcmp( pName1, pName2 );
    if ( pName1 )
        return -1;
    if ( pName2 )
        return 1;
    if ( (*ppN1)->Id < (*ppN2)->Id )
        return -1;
    if ( (*ppN1)->Id > (*ppN2)->Id )
        return 1;
    assert( 0 );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Compares the nodes according to the accepted order.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NodeCompareByValueAndId( Ntk_Node_t ** ppN1, Ntk_Node_t ** ppN2 )
{
    // compare using number of values
    if ( (*ppN1)->nValues < (*ppN2)->nValues )
        return -1;
    if ( (*ppN1)->nValues > (*ppN2)->nValues )
        return 1;
    // compare using unique node ID
    if ( (*ppN1)->Id < (*ppN2)->Id )
        return -1;
    if ( (*ppN1)->Id > (*ppN2)->Id )
        return 1;
    // should never happen because node IDs are unique
//    assert( 0 ); // may happen if fanins are duplicated in the BLIF file
    return 0;
}

/**Function*************************************************************

  Synopsis    [Compares the nodes according to the accepted order.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NodeCompareByLevel( Ntk_Node_t ** ppN1, Ntk_Node_t ** ppN2 )
{
    // compare using number of values
    if ( (*ppN1)->Level < (*ppN2)->Level )
        return 1;
    if ( (*ppN1)->Level > (*ppN2)->Level )
        return -1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the name mode is short.]

  Description []
               
  SideEffects [] 

  SeeAlso     []

***********************************************************************/
bool Ntk_NetworkIsModeShort( Ntk_Network_t * pNet )
{
    Mv_Frame_t * pMvsis;
    if ( pNet == NULL )
        return 1;
    pMvsis = Ntk_NetworkReadMvsis( pNet );
    return Mv_FrameReadMode( pMvsis );
}

/**Function*************************************************************

  Synopsis    [Returns the pointer to the node if the node name is known.]

  Description [Checks the internal hash table of the network for existance
  of the node with the given name. If the node exists, returns the pointer
  to the node structure. If the node does not exist, returns NULL.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NetworkFindOrAddNodeByName( Ntk_Network_t * pNet, char * pName, Ntk_NodeType_t Type )
{
    Ntk_Node_t * pNode;
    if ( st_lookup( pNet->tName2Node, pName, (char**)&pNode ) )
        return pNode;
    // create a new node
    pNode = Ntk_NodeCreate( pNet, pName, Type, -1 );
    // add the node to the network
    Ntk_NetworkAddNode( pNet, pNode, 0 );
    return pNode;
}

/**Function*************************************************************

  Synopsis    [Returns the pointer to the node if the node name is known.]

  Description [Checks the internal hash table of the network for existance
  of the node with the given name. If the node exists, returns the pointer
  to the node structure. If the node does not exist, returns NULL.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NetworkFindOrAddNodeByName2( Ntk_Network_t * pNet, char * pName, Ntk_NodeType_t Type )
{
    Ntk_Node_t * pNode;
    if ( st_lookup( pNet->tName2Node, pName, (char**)&pNode ) )
        return pNode;
    // create a new node
    pNode = Ntk_NodeCreate( pNet, pName, Type, -1 );
    // add the node to the network
    Ntk_NetworkAddNode2( pNet, pNode, 0 );
    return pNode;
}

/**Function*************************************************************

  Synopsis    [Returns the pointer to the node if the node name is known.]

  Description [Checks the internal hash table of the network for existence
  of the node with the given name. If the node exists, returns the pointer
  to the node structure. If the node does not exist, returns NULL.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NetworkFindNodeByName( Ntk_Network_t * pNet, char * pName )
{
    Ntk_Node_t * pNode;
    if ( st_lookup( pNet->tName2Node, pName, (char**)&pNode ) )
        return pNode;
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Returns the pointer to the node if the node name is known.]

  Description [The name of the node may be its short or long name, which
  is not actually assigned to the node, but derived from its node ID.
  This procedure find the node in this case, too.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NetworkFindNodeByNameWithMode( Ntk_Network_t * pNet, char * pName )
{
    if ( Ntk_NetworkIsModeShort( pNet ) )
        return Ntk_NetworkFindNodeByNameShort( pNet, pName );
    return Ntk_NetworkFindNodeByNameLong( pNet, pName );
}

/**Function*************************************************************

  Synopsis    [Returns the pointer to the node using its long name.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NetworkFindNodeByNameLong( Ntk_Network_t * pNet, char * pName )
{
    Ntk_Node_t * pNode;
    int NumId;
    // look for the regular name in the table
    if ( st_lookup( pNet->tName2Node, pName, (char**)&pNode ) )
        return pNode;
    // look for a long printable name
    if ( pName[0] >= '[' && pName[1] >= '0' && pName[1] <= '9' )
    {
        NumId = atoi(pName + 1);
        return Ntk_NetworkFindNodeById( pNet, NumId );
    }
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Returns the pointer to the node using its short name.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NetworkFindNodeByNameShort( Ntk_Network_t * pNet, char * pName )
{
    char Buffer[20];
    int NumId, Number;
    // look for a short printable name
    if ( pName[0] >= 'a' && pName[0] <= 'z' )
    {
        if ( pName[1] != '\0' && (pName[1] < '0' || pName[1] > '9') )
            return NULL;
        // make sure that there are no trailing chars
        if ( pName[1] == 0 )
        {
            Number = 0;
            sprintf( Buffer, "%c", pName[0] );
        }
        else
        {
            Number = atoi(pName + 1);
            sprintf( Buffer, "%c%d", pName[0], Number );
        }
        if ( strcmp( pName, Buffer ) != 0 )
            return NULL;
        // get the node pointer by the node ID
        NumId = Number * 26 + (pName[0] - 'a') + 1;
        return Ntk_NetworkFindNodeById( pNet, NumId );
    }
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Returns the pointer to the node if the node name is known.]

  Description [Checks the internal hash table of the network for existance
  of the node with the given name. If the node exists, returns the pointer
  to the node structure. If the node does not exist, returns NULL.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NetworkFindNodeById( Ntk_Network_t * pNet, int Id )
{
    if ( Id > 0 && Id < pNet->nIds )
        return pNet->pId2Node[Id];
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Registers the name with the string memory manager.]

  Description [This function should be used to register all names
  permanentsly stored with the network. The pointer returned by
  this procedure contains the copy of the name, which should be used
  in all network manipulation procedures.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Ntk_NetworkRegisterNewName( Ntk_Network_t * pNet, char * pName )
{
    char * pRegisteredName;
    pRegisteredName = Extra_MmFlexEntryFetch( pNet->pNames, strlen(pName) + 1 );
    strcpy( pRegisteredName, pName );
    return pRegisteredName;
}

/**Function*************************************************************

  Synopsis    [Renames all nodes of the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkRenameNodes( Ntk_Network_t * pNet )
{
    Ntk_Node_t * pNode;
    char * pNetName, * pSpecName;
    int nNodes, nChars, iNode;

    // reset the hash table, hashing current node names into pointers
    st_free_table( pNet->tName2Node );
    pNet->tName2Node = st_init_table(strcmp, st_strhash);

    // save network's name
    pNetName  = util_strsav( pNet->pName );
    pSpecName = util_strsav( pNet->pSpec );

    // reset the name storage
    Extra_MmFlexStop( pNet->pNames, 0 );
    pNet->pNames = Extra_MmFlexStart( 10000, 1000 );

    // register the name 
    if ( pNetName )
        pNet->pName = Ntk_NetworkRegisterNewName( pNet, pNetName );
    FREE( pNetName );
    // register the network spec file name
    if ( pSpecName )
        pNet->pSpec = Ntk_NetworkRegisterNewName( pNet, pSpecName );
    FREE( pSpecName );


    // get the number of chars in the node name
    nNodes = Ntk_NetworkReadNodeTotalNum( pNet );
	for ( nChars = 0; nNodes; nNodes /= 26, nChars++ );

    // go through the nodes and set their names
    iNode = 0;
    Ntk_NetworkForEachCi( pNet, pNode )
        Ntk_NetworkAssignNewName( pNode, iNode++, nChars );
    Ntk_NetworkForEachCo( pNet, pNode )
        Ntk_NetworkAssignNewName( pNode, iNode++, nChars );
    Ntk_NetworkForEachNode( pNet, pNode )
        Ntk_NetworkAssignNewName( pNode, iNode++, nChars );

    // reorder the nodes and the fanins
    Ntk_NetworkOrderFanins( pNet );
    if ( !Ntk_NetworkCheck( pNet ) )
       fprintf( Ntk_NetworkReadMvsisOut(pNet), "Ntk_NetworkRenameNodes(): Network check has failed.\n" );
}

/**Function*************************************************************

  Synopsis    [Assign a simple name to the node.]

  Description [This procedure ignores whatever name the node may
  have had and assigns the new name using a simple naming scheme.
  Should not be called only from Ntk_NetworkRenameNodes(). ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkAssignNewName( Ntk_Node_t * pNode, int iNum, int nChars )
{
    Ntk_Network_t * pNet;
    char Name[10], * pName;
    int i;

    // derive a simple name
//    sprintf( Name, "%c%0*d", 'a' + (pNode->Id - 1) % 26, nChars, (pNode->Id - 1) / 26 );
    for ( i = 0; i < nChars; i++ )
    {
        Name[nChars-1-i] = 'a' + iNum % 26;
        iNum = iNum / 26;
    }
    Name[nChars] = 0;
    
    // register this name with the name manager
    pNet = pNode->pNet;
    pName = Ntk_NetworkRegisterNewName( pNet, Name );
    // set the name
    pNode->pName = pName;
    if ( st_insert( pNet->tName2Node, pName, (char *)pNode ) )
    {
        assert( 0 ); // why the brand new names should be in the table?
    }
}

/**Function*************************************************************

  Synopsis    [Collects one node by name.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NetworkCollectNodeByName( Ntk_Network_t * pNet, char * pName, int fCollectCis )
{
    FILE * pFile ;
    Ntk_Node_t * pNode, * pFanin; 

    pFile = Ntk_NetworkReadMvsisOut( pNet );
    pNode = Ntk_NetworkFindNodeByNameWithMode( pNet, pName );
    if ( pNode == NULL )
    {
        fprintf( pFile, "Cannot find node \"%s\".\n", pName );
        return NULL;
    }
    if ( Ntk_NodeHasCoName(pNode) )
    {
        fprintf( pFile, "Cannot find node \"%s\".\n", pName );
        return NULL;
    }
    if ( pNode->Type == MV_NODE_CI && !fCollectCis )
    {
        fprintf( pFile, "Skipping CI node \"%s\".\n", pName );
        return NULL;
    }
    if ( pNode->Type == MV_NODE_CO )
    {
        pFanin = Ntk_NodeReadFaninNode( pNode, 0 );
        if ( Ntk_NodeHasCoName(pFanin) )
            return pFanin;
        else
            return pNode;
    }
    else
        return pNode;
}

/**Function*************************************************************

  Synopsis    [Collects the input and output names of the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkCollectIoNames( Ntk_Network_t * pNet, char *** ppsLeaves, char *** ppsRoots )
{
    char ** psLeaves;
    char ** psRoots;
    Ntk_Node_t * pNode;
    int i;

    // collect CI names
    psLeaves    = ALLOC( char *, Ntk_NetworkReadCiNum(pNet) );
    i = 0;
    Ntk_NetworkForEachCi( pNet, pNode )
        psLeaves[i++] = pNode->pName;
    assert( i == Ntk_NetworkReadCiNum(pNet) ); 

    // collect CO names
    psRoots    = ALLOC( char *, Ntk_NetworkReadCoNum(pNet) );
    i = 0;
    Ntk_NetworkForEachCo( pNet, pNode )
        psRoots[i++] = pNode->pName;
    assert( i == Ntk_NetworkReadCoNum(pNet) ); 

    *ppsLeaves = psLeaves;
    *ppsRoots  = psRoots;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



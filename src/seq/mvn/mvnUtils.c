/**CFile****************************************************************

  FileName    [mvnUtils.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Sequential manipulation on the level of the MV network.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvnUtils.c,v 1.4 2004/02/19 03:11:20 alanmi Exp $]

***********************************************************************/

#include "mvnInt.h"

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
int Mvn_ReadNamePairs( FILE * pOutput, Ntk_Network_t * pNet1, Ntk_Network_t * pNet2, 
      char * argv[], int argc, char * ppNames1[], char * ppNames2[] )
{
    Ntk_Node_t * pNode1, * pNode2;
    char * pPart1, * pPart2;
    int i;

    // go through the equations;
    for ( i = 0; i < argc; i++ )
    {
        pPart1 = strtok( argv[i], " =\n" );
        pPart2 = strtok( NULL,    " =\n" );
        // make sure that the names of the PI/PO pairs
        pNode1 = Ntk_NetworkFindNodeByName( pNet1, pPart1 );
        if ( pNode1 == NULL )
        {
	        fprintf( pOutput, "Equation %d: There is no node \"%s\" in the first network\n", i+1, pPart1 );
            return -1;
        }
        if ( Ntk_NodeReadType(pNode1) == MV_NODE_INT )
        {
	        fprintf( pOutput, "Equation %d: The node \"%s\" in the first network is not a PI/PO\n", i+1, pPart1 );
            return -1;
        }

        pNode2 = Ntk_NetworkFindNodeByName( pNet2, pPart2 );
        if ( pNode2 == NULL )
        {
	        fprintf( pOutput, "Equation %d: There is no node \"%s\" in the second network\n", i+1, pPart2 );
            return -1;
        }
        if ( Ntk_NodeReadType(pNode2) == MV_NODE_INT )
        {
	        fprintf( pOutput, "Equation %d: The node \"%s\" in the second network is not a PI/PO\n", i+1, pPart2 );
            return -1;
        }
        // set the names
        ppNames1[i] = util_strsav( pPart1 );
        ppNames2[i] = util_strsav( pPart2 );
    }
    return argc;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



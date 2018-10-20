/**CFile****************************************************************

  FileName    [ioInt.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Internal declarations of the input/output package.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ioInt.h,v 1.25 2005/06/02 03:34:08 alanmi Exp $]

***********************************************************************/

#ifndef __IO_INT_H__
#define __IO_INT_H__

/*
    This univerisal BLIF parser works in several steps:
    (1) "ioReadFile.c" : load file contents into memory, delete comments
    (2) "ioReadLines.c": sort the dot-statements and pre-parse nodes
    (3) "ioReadNet.c"  : create the network structure without functionality
    (4) "ioReadNode.c" : parse tables and get node functionality (MDD, MVSOP)
*/

/* 
    One of the problems with BLIF-MVS is that if a PI is not used by any
    internal nodes in the current network, there is no place where its 
    number of values is stored... For comparison, in BLIF-MV, we can have
    .mv line even if the PI does not belong to the support of any node.
*/

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "mv.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

// the temporary structures used by the reader
typedef struct IoReadStruct        Io_Read_t;   // all reading info
typedef struct IoNodeStruct        Io_Node_t;   // node info
typedef struct IoLatchStruct       Io_Latch_t;  // latch info
typedef struct IoVarStruct         Io_Var_t;    // var value names


struct IoReadStruct
{
    // the current framework
    Mv_Frame_t * pMvsis;      // the current framework
    // general info about file
    char *       FileName;       // the name of the file
    int          FileSize;       // the size of the file
    int          Type;           // the file type
    char *       pFileContents;  // the contents of the file
    // storage for all lines
    char **      pLines;         // the beginning of all lines in the file
    int          nLines;         // the number of all lines in the file
    // the input file sections
    int          LineModel;      // the .model directive line
    int          LineSpec;       // the .spec directive line
    int          LineExdc;       // the .exdc directive line (if present)
    int          LineEnd;        // the .end directive line
    // dot-statements
    int *        pDots;          // the beginning of all dot-statements
    int          nDots;          // the number of all dot-statements in the file
    // .inputs statements
    int *        pDotInputs;     // the beginning of all .inputs statements
    int          nDotInputs;     // the number of all .inputs statements
    // .outputs statements
    int *        pDotOutputs;    // the beginning of all .outputs statements
    int          nDotOutputs;    // the number of all .outputs statements
    array_t *    aOutputs;       // the array of outputs
    // .node/.table statements
    Io_Node_t *  pIoNodes;       // the node info struct for each node
    int          nDotNodes;      // the number of .node/.table directives in the file
    int          nDotDefs;       // the number of .default lines
    // .mv statements (BLIF-MV)
    int *        pDotMvs;        // the beginning of each .mv-directive in BLIF-MV file
    int          nDotMvs;        // the number of .mv-directive in BLIF-MV file
    st_table *   tName2Values;   // mapping of node name into num values (only for non-binary nodes) 
    st_table *   tName2Var;      // mapping of node name into Io_Var_t representing symbolic names (for BLIF-MV files)
    // .latch statements
    Io_Latch_t * pIoLatches;     // the latch descriptions
    int          nDotLatches;    // the number of latch descriptions
    int          nDotReses;      // the number of reset lines
    st_table *   tLatch2ResLine; // mapping of latch output name into its reset line number
    // .input_arrival statements
    char *       pLineDefaultInputArrival; // the line with the .default_input_arrival keyword
    int *        pDotInputArr;	 // the beginning of all .input_arrival statements
    int		 nDotInputArr;   // the number of all .input_arrival statements
    // current processing info
    Ntk_Network_t * pNet;        // the primary network
    int          fParsingExdcNet;// this flag is on, when we are parsing EXDC network
    Ntk_Node_t * pNodeCur;       // the node that is currently considere
    int          LineCur;        // the line currently parsed
    // the error message
    FILE *       Output;         // the output stream
    char *       sError;         // the error string generated during parsing
    // multi-purpose storage
    int          nFaninsMax;     // the largest number of fanins
    int          nValuesMax;     // the largest number of values;
    int *        pArray;         // temporary array of the size (nFaninsMax * nValuesMax)
    Mvc_Cover_t* pCover;         // the temporary set used to store MV cubes
    Mvc_Cube_t * pCube;          // the temporary set used to store MV cubes
    char **      pTokens;        // the temporary tokens
};

struct IoNodeStruct
{
    // the information about the node after preparsing
    int          LineNames;      // the number of the .names line of this node
    int          LineTable;      // the number of the first line in the table of this node 
    int          nLines;         // the number of lines in the table
    short        nValues;        // the number of values
    short        Default;        // the default value
    // the information for node construction
    char *       pOutput;        // the name of the output
//    Io_Var_t *   pVar;           // the value names (for BLIF-MV)
};

struct IoVarStruct
{
    int          nValues;        // the number of values
    char **      pValueNames;    // the names of the values (for BLIF-MV)
};

struct IoLatchStruct
{
    char *       pInputName;     // the latch input name
    char *       pOutputName;    // the latch output name
    int          LineLatch;      // the latch line
    int          ResValue;       // binary reset value
    Io_Node_t    IoNode;         // the Io_Node_t struct of the reset table
};

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/*=== ioRead.c =======================================================*/
extern void            Io_ReadPrintErrorMessage( Io_Read_t * p );
extern void            Io_ReadStructCleanTables( Io_Read_t * p );
extern void            Io_ReadStructAllocateAdditional( Io_Read_t * p );
/*=== ioReadFile.c ===================================================*/
extern int             Io_ReadFile( Io_Read_t * p );
extern int             Io_ReadFileMarkLinesAndDots( Io_Read_t * p );
extern char *          Io_ReadFileGetSimilar( char * pFileName, char * pS1, char * pS2, char * pS3, char * pS4, char * pS5 );
extern char *          Io_ReadFileFileContents( char * FileName, int * pFileSize );
extern void            Io_ReadFileRemoveComments( char * pBuffer, int * pnDots, int * pnLines );
/*=== ioReadLines.c ==================================================*/
extern int             Io_ReadLines( Io_Read_t * p );
extern int             Io_ReadLinesAddNumValues( Io_Read_t * p, char * pNodeName, int nValues );
/*=== ioReadNet.c ====================================================*/
extern Ntk_Network_t * Io_ReadNetworkStructure( Io_Read_t * p );
/*=== ioReadNode.c ===================================================*/
extern int             Io_ReadNodeFunctions( Io_Read_t * p, Ntk_Node_t * pNode );
/*=== ioStats.c ======================================================*/
extern Ntk_Network_t * Io_ReadNetworkBench( Mv_Frame_t * pMvsis, char * FileName );
/*=== ioWrite.c ======================================================*/
/*=== ioWriteSplit.c =================================================*/
extern void            IoNetworkSplit( Ntk_Network_t * pNet, int Output, int OutSuppMin, 
                             int NodeFanMax, bool fAllInputs, bool fWriteBlif, bool fVerbose );
/*=== ioWriteBench.c =================================================*/
extern void            Io_WriteNetworkBench( Ntk_Network_t * pNet, char * FileName );

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

#endif


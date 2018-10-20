/**CFile****************************************************************

  FileName    [aioInt.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Internal declarations of the input/output package.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: aioInt.h,v 1.1 2004/02/19 03:06:41 alanmi Exp $]

***********************************************************************/

#ifndef __AIO_INT_H__
#define __AIO_INT_H__

/*
    This univerisal BLIF parser works in several steps:
    (1) "ioReadFile.c" : load file contents into memory, delete comments
    (2) "ioReadLines.c": sort the dot-statements and pre-parse nodes
    (3) "ioReadNet.c"  : create the network structure without functionality
    (4) "ioReadNode.c" : parse tables and get node functionality (MDD, MVSOP)
*/

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "mv.h"
#include "aut.h"
#include "aio.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

// the temporary structures used by the reader
typedef struct AioReadStruct        Aio_Read_t;   // all reading info
typedef struct AioNodeStruct        Aio_Node_t;   // node info
typedef struct AioLatchStruct       Aio_Latch_t;  // latch info
typedef struct AioVarStruct         Aio_Var_t;    // var value names


struct AioReadStruct
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
    Aio_Node_t *  pAioNodes;       // the node info struct for each node
    int          nDotNodes;      // the number of .node/.table directives in the file
    int          nDotDefs;       // the number of .default lines
    // .mv statements (BLIF-MV)
    int *        pDotMvs;        // the beginning of each .mv-directive in BLIF-MV file
    int          nDotMvs;        // the number of .mv-directive in BLIF-MV file
    st_table *   tName2Values;   // mapping of node name into num values (only for non-binary nodes) 
    st_table *   tName2Var;      // mapping of node name into Aio_Var_t representing symbolic names (for BLIF-MV files)
    // .latch statements
    Aio_Latch_t * pAioLatches;     // the latch descriptions
    int          nDotLatches;    // the number of latch descriptions
    int          nDotReses;      // the number of reset lines
    st_table *   tLatch2ResLine; // mapping of latch output name into its reset line number
    // current processing info
    Aut_Auto_t * pAut;          // the primary network
    int          fParsingExdcNet;// this flag is on, when we are parsing EXDC network
    Ntk_Node_t * pNodeCur;       // the node that is currently considered
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
    Aut_Man_t *  pManDd;
    Aio_Node_t * pIoNodeO;
    Aio_Node_t * pIoNodeN;
    Vm_VarMap_t * pVm;
    int fEmpty;
};

struct AioNodeStruct
{
    // the information about the node after preparsing
    int          LineNames;      // the number of the .names line of this node
    int          LineTable;      // the number of the first line in the table of this node 
    int          nLines;         // the number of lines in the table
    short        nValues;        // the number of values
    short        Default;        // the default value
    // the information for node construction
    char *       pOutput;        // the name of the output
//    Aio_Var_t *   pVar;           // the value names (for BLIF-MV)
    char *       pDefName;
};

struct AioVarStruct
{
    int          nValues;        // the number of values
    char **      pValueNames;    // the names of the values (for BLIF-MV)
};

struct AioLatchStruct
{
    char *       pInputName;     // the latch input name
    char *       pOutputName;    // the latch output name
    int          LineLatch;      // the latch line
    int          ResValue;       // binary reset value
    Aio_Node_t    AioNode;         // the Aio_Node_t struct of the reset table
};

extern char *          Io_ReadFileFileContents( char * FileName, int * pFileSize );
extern void            Io_ReadFileRemoveComments( char * pBuffer, int * pnDots, int * pnLines );

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/*=== aioRead.c =======================================================*/
/*=== aioReadAut.c ======================================================*/
extern Aut_Auto_t *   Aio_AutoReadAut( Mv_Frame_t * pMvsis, char * FileName, Aut_Man_t * pManDd );
/*=== aioReadBlifMv.c ======================================================*/
extern Aut_Auto_t *   Aio_AutoReadBlifMv( Mv_Frame_t * pMvsis, char * FileName, Aut_Man_t * pManDd );
extern void           Aio_ReadPrintErrorMessage( Aio_Read_t * p );
extern void           Aio_ReadStructCleanTables( Aio_Read_t * p );
/*=== aioReadFile.c ===================================================*/
extern int            Aio_ReadFile( Aio_Read_t * p );
extern int            Aio_ReadFileMarkLinesAndDots( Aio_Read_t * p );
/*=== aioReadLines.c ==================================================*/
extern int            Aio_ReadLines( Aio_Read_t * p );
extern int            Aio_ReadLinesAddNumValues( Aio_Read_t * p, char * pNodeName, int nValues );
/*=== aioReadNet.c ====================================================*/
extern Aut_Auto_t *   Aio_ReadNetworkStructure( Aio_Read_t * p );
/*=== aioReadNode.c ===================================================*/
extern int            Aio_ReadNodeFunctions( Aio_Read_t * p, Aut_Auto_t * pAut );

/*=== aioWrite.c =======================================================*/
/*=== aioWriteBlifMv.c ======================================================*/
extern void           Aio_AutoWriteBlifMv( Aut_Auto_t * pAut, char * FileName, bool fWriteFsm );
/*=== aioWriteAut.c ======================================================*/
extern void           Aio_AutoWriteAut( Aut_Auto_t * pAut, char * FileName, bool fWriteFsm );

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

#endif


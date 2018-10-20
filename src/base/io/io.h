/**CFile****************************************************************

  FileName    [io.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [External declarations of the input/output package.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: io.h,v 1.19 2005/02/28 05:34:23 alanmi Exp $]

***********************************************************************/

#ifndef __IO_H__
#define __IO_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "mv.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

// the file types this reader can read
enum { IO_FILE_NONE, IO_FILE_BLIF, IO_FILE_BLIF_MVS, IO_FILE_BLIF_MV, IO_FILE_PLA, IO_FILE_PLA_MV };

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/*=== io.c ==========================================================*/
extern void            Io_Init( Mv_Frame_t * pMvsis );
extern void            Io_End( Mv_Frame_t * pMvsis );
/*=== ioFileOpen.c ==================================================*/
extern FILE *          Io_FileOpen(const char * FileName, const char * PathVar, const char *Mode);
/*=== ioRead.c ======================================================*/
extern Ntk_Network_t * Io_ReadNetwork( Mv_Frame_t * pMvsis, char * FileName );
/*=== ioReadPla.c ======================================================*/
extern Ntk_Network_t * Io_ReadPla( Mv_Frame_t * pMvsis, char * FileName );
extern int             Io_ReadFileType( char * FileName );
/*=== ioReadPlaMv.c ======================================================*/
extern Ntk_Network_t * Io_ReadPlaMv( Mv_Frame_t * pMvsis, char * FileName, bool fBinaryOutput );
/*=== ioReadKiss.c ======================================================*/
extern Ntk_Network_t * Io_ReadKiss( Mv_Frame_t * pMvsis, char * FileName, bool fAut );
/*=== ioWrite.c =====================================================*/
extern void            Io_WriteNetwork( Ntk_Network_t * pNet, char * FileName, int FileType, bool fWriteLatches );
extern void            Io_WriteNetworkOne( FILE * pFile, Ntk_Network_t * pNet, int FileType, bool fWriteLatches );
extern void            Io_WriteNetworkNode( FILE * pFile, Ntk_Node_t * pNode, int FileType, int fLatch );
extern void            Io_WriteNetworkCis( FILE * pFile, Ntk_Network_t * pNet, bool fWriteLatches );
extern void            Io_WriteNetworkCos( FILE * pFile, Ntk_Network_t * pNet, bool fWriteLatches );
extern void            Io_WriteNetworkCisSpecial( FILE * pFile, Ntk_Network_t * pNet );
/*=== ioWritePla.c =====================================================*/
extern void            Io_WritePla( Ntk_Network_t * pNet, char * FileName );
/*=== ioWritePlaMv.c =====================================================*/
extern void            Io_WritePlaMv( Ntk_Network_t * pNet, char * NodeName, char * FileName );
/*=== ioStats.c =====================================================*/
extern void            Io_PrintNetworkStats( FILE * pFile, Ntk_Network_t * pNet );
/*=== ioWriteMapped.c =====================================================*/
extern void            Io_NetworkWriteMappedBlif( Ntk_Network_t * pNet, char * pFileNameOut );

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

#endif


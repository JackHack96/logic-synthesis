/**CFile****************************************************************

  FileName    [mvtypes.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [special types used across MVSIS.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvtypes.h,v 1.2 2003/05/27 23:14:19 alanmi Exp $]

***********************************************************************/

#ifndef __MVTYPES_H__
#define __MVTYPES_H__

////////////////////////////////////////////////////////////////////////
///                         TYPEDEFS                                 ///
////////////////////////////////////////////////////////////////////////

#ifdef bool
#undef bool
#endif
typedef int                        bool;

#ifdef uint8
#undef uint8
#endif
typedef unsigned char              uint8;

#ifdef uint16
#undef uint16
#endif
typedef unsigned short int         uint16;

#ifdef uint32
#undef uint32
#endif
typedef unsigned int               uint32;


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

#endif

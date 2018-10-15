/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/octio/octio.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:47 $
 *
 */
#ifdef OCT
#include "copyright.h"
#endif
#include "sis.h"
#include "octio.h"

/*
 *  called when the program starts up
 */
init_octio()
{
#ifdef OCT
    com_add_command("write_oct", external_write_oct, 0);
    com_add_command("read_oct", external_read_oct, 1);
#endif
}

/*
 *  called just before the program terminates
 */
end_octio()
{
}






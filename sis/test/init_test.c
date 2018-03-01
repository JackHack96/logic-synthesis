/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/test/init_test.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:52 $
 *
 */
#include "sis.h"
#include "test_int.h"


/*
 *  called when the program starts up
 */
init_test()
{
    com_add_command("_test", com_test, /* changes_network */ 1);
}

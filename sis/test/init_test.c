
#include "sis.h"
#include "test_int.h"


/*
 *  called when the program starts up
 */
init_test()
{
    com_add_command("_test", com_test, /* changes_network */ 1);
}

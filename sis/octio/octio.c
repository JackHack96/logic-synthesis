
#ifdef OCT
#include "../port/copyright.h"
#endif

#include "octio.h"
#include "sis.h"

/*
 *  called when the program starts up
 */
init_octio() {
#ifdef OCT
  com_add_command("write_oct", external_write_oct, 0);
  com_add_command("read_oct", external_read_oct, 1);
#endif
}

/*
 *  called just before the program terminates
 */
end_octio() {}

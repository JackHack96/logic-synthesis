
#ifndef OCTIO_H
#define OCTIO_H

#include "network.h"

#define SIS_PKG_NAME "oct/sis"

extern int external_read_oct(network_t **, int, char **);

extern int external_write_oct(network_t **, int, char **);

extern char *optProgName;
#endif

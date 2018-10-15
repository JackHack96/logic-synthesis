
#ifndef OCTIO_H
#define OCTIO_H

#include "network.h"

#define SIS_PKG_NAME "oct/sis"

int external_read_oct(network_t **, int, char **);

int external_write_oct(network_t **, int, char **);

char *optProgName;
#endif

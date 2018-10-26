
#ifndef OCTIO_H
#define OCTIO_H

#define SIS_PKG_NAME "oct/sis"

EXTERN int external_read_oct  ARGS((network_t **,int,char**));
EXTERN int external_write_oct ARGS((network_t **,int,char**));

extern char *optProgName;
#endif

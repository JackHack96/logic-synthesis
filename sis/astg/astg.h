/* astg.h -- exported programming interface to ASTG package. */

#ifndef ASTG_H
#define ASTG_H

typedef char astg_t;

void init_astg();

void end_astg();

astg_t *astg_dup(astg_t *);

void astg_free(astg_t *);

#endif /* ASTG_H */

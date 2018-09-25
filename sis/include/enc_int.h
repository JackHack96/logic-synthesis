#ifndef ENC_INT_H
#define ENC_INT_H

#include "enc.h"

#define SOP_ALLOC 100

extern void input_cons();

extern int filter_cons();

extern int read_cons();

extern dic_family_t *gen_iter();

extern void dic_family_free();

extern dic_family_t *dic_family_copy();

extern void dic_family_print();

extern void dic_family_add();

extern void dic_family_add_contain();

extern void dic_family_add_irred();

extern void dic_free();

extern void dic_print();

extern bool is_prime();

extern bool dicp_implies();

extern bool dicp_equal();

extern void print_min_cover();

#endif
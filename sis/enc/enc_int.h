#ifndef ENC_INT_H
#define ENC_INT_H

#include "enc.h"

#define SOP_ALLOC 100

void input_cons();

int filter_cons();

int read_cons();

dic_family_t *gen_iter();

void dic_family_free();

dic_family_t *dic_family_copy();

void dic_family_print();

void dic_family_add();

void dic_family_add_contain();

void dic_family_add_irred();

void dic_free();

void dic_print();

bool is_prime();

bool dicp_implies();

bool dicp_equal();

void print_min_cover();

#endif
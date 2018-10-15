#ifndef MAXFLOW_INT_H
#define MAXFLOW_INT_H

/*
 * functions that will be used internal to the package
 */
void mf_error();

char *MF_calloc();

void get_cutset();

#define LABELLED 1
#define MARKED 2
#define FICTITIOUS 4
#define CUR_TRACE 8

#define MAX_FLOW 100000000
#define MF_HASHSIZE 399
#define MF_MAXSTR 256

/*
 * miscellaneous marcos
 */
#define MF_ALLOC(num, type) ((type *)MF_calloc((int)(num), sizeof(type)))

#endif
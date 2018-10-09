#ifndef GENLIB_H
#define GENLIB_H
/* file @(#)genlib.h	1.1                      */
/* last modified on 5/29/91 at 12:35:24   */
/* exported functions to mis ... */

#include <stdio.h>

int genlib_parse_library(FILE *fp, char *infile, FILE *outfile, int use_nand);

int init_genlib();

int end_genlib();

#endif
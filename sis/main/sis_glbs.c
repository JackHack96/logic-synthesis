/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/main/sis_glbs.c,v $
 * $Author: pchong $
 * $Revision: 1.3 $
 * $Date: 2004/02/18 18:38:31 $
 *
 */
#include <stdio.h>
#include "ansi.h"
#include "array.h"

FILE *siserr = 0;
FILE *sisout = 0;
char *program_name = 0;

FILE *sishist = 0;
array_t *command_hist = 0;

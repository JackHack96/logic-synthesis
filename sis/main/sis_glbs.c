
#include <stdio.h>

#include "array.h"

FILE *siserr       = 0;
FILE *sisout       = 0;
char *program_name = 0;

FILE    *sishist      = 0;
array_t *command_hist = 0;

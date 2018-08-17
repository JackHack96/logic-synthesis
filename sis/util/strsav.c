#include <stdio.h>
#include "../include/util.h"


/*
 *  util_strsav -- save a copy of a string
 */
char *util_strsav(char *s) {
    return strcpy(ALLOC(char, strlen(s) + 1), s);
}

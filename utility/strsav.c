#include "../port/copyright.h"
#include "../port/port.h"
#include "../utility/utility.h"

/*
 *  util_strsav -- save a copy of a string
 */
char *util_strsav(char *s) { return strcpy(ALLOC(char, strlen(s) + 1), s); }

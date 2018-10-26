
#ifndef ERROR_H
#define ERROR_H

void error_init(void);

void error_append(char *);

char *error_string(void);

void error_cleanup(void);

#endif

#ifndef ERROR_H
#define ERROR_H

extern void error_init(void);

extern void error_append(char *);

extern char *error_string(void);

extern void error_cleanup(void);

#endif
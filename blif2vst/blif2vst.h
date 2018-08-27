#ifndef SIS_BLIF2VST_H
#define SIS_BLIF2VST_H

#define MAX_TOKEN_LENGTH 256
#define MAX_NAME_LENGTH 32

/*
 * Following are the structures containing all the info extracted from
 * the library (all the pins, the inputs, the outputs and, if needed, the clock)
 */
struct ports {
  char name[MAX_NAME_LENGTH];
};

struct cell {
  char name[MAX_NAME_LENGTH];
  int npins;
  char used;
  struct ports *formals;
  struct cell *next;
};

struct instance {
  struct cell *what;
  struct ports *actuals;
  struct instance *next;
};

/* list of formal names, is used in get_port when multiple   *
 * definition are given, as    a,b,c : IN BIT;              */
struct TMPstruct {
  char name[MAX_NAME_LENGTH];
  int num;
  struct TMPstruct *next;
};

struct BITstruct {
  char name[MAX_NAME_LENGTH + 10];
  struct BITstruct *next;
};

struct VECTstruct {
  char name[MAX_NAME_LENGTH];
  int start;
  int end;
  char dir;
  struct VECTstruct *next;
};

struct TYPEterms {
  struct BITstruct *BITs;
  struct VECTstruct *VECTs;
};

struct MODELstruct {
  char name[MAX_NAME_LENGTH];
  struct TYPEterms *Inputs;
  struct TYPEterms *Outputs;
  struct TYPEterms *Internals;
  struct instance *Net;
  struct MODELstruct *next;
};

#if defined(linux)

void add_bit(struct BITstruct **, char *);

#endif

// Procedures to exit or to display warnings

/**
 * Closes all the files and flushes the memory
 */
void close_all();

/**
 * Displays an error message, and then exits
 * @param msg Message to printout before exiting
 */
void error(char *msg);

/**
 * Puts a message on stderr, writes the current line
 * and then sends the current token back
 * @param name Message
 */
void warning(char *name);

/**
 * Sends to stderr a message and then exits
 * @param msg Message to print
 * @param obj Token that generates the error
 */
void syntax_error(char *msg, char *obj);

// General procedures

/**
 * Compares two strings (in case insensitive mode),
 * it also skips initial and final double-quote
 * @param name First string, tipically the token
 * @param keywrd Second string, tipically a keyword
 * @return 1 if the string match, 0 otherwise
 */
char keyword_compare(char *name, char *keywrd);

/**
 * Returns a pointer to an element of the list of
 * gates that matches up the name given, if there
 * isn't a match a null pointer is returned.
 * @param name Name to match
 * @return A (void *) pointer
 */
struct cell *match_gate(char *name);

void release_bit(struct BITstruct *ptr);

struct cell *new_cell(char *name, struct BITstruct *ports);

struct instance *new_instance(struct cell *cell);

struct TYPEterms *new_type();

struct MODELstruct *new_model();

void add_vect(struct VECTstruct **VECTptr, char *name, int a, int b);

void add_bit(struct BITstruct **BITptr, char *name);

struct BITstruct *is_here(char *name, struct BITstruct *ptr);

/**
 * Parse new token in the given string
 * @param tok String with new token
 */
void get_token(char *tok);

void print_gates(struct cell *cell);

/**
 * Tokenizer to scan the library file
 * @param lib Library file
 * @param tok Filled with the new token
 */
void get_lib_token(FILE *lib, char *tok);

/**
 * Scans the library to get the names of the cells,
 * the output pins and the clock signals of latches
 * @param lib_name Name of library file
 * @return Cell structure
 */
struct cell *scan_library(char *lib_name);

/**
 * Get the options from the command line, open the input and
 * the output file and read params from the library file
 *
 * @param argc Number of arguments
 * @param argv Arguments string array
 */
void check_args(int argc, char **argv);

/**
 * Check if a token is a decimal number
 * @param string Token to check
 * @return Converted integer, 0 otherwise
 */
int dec_number(char *string);

void get_signals(struct TYPEterms *TYPEptr);

void order_vectors(struct VECTstruct *VECTptr);

void print_signals(struct MODELstruct *model);

void print_net(struct instance *cell);

struct instance *get_names(char *name, char type, struct MODELstruct *model);

void print_vst(struct MODELstruct *model);

void parse_file();

#endif // SIS_BLIF2VST_H

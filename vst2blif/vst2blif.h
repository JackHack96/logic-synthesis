#ifndef SIS_VST2BLIF_H
#define SIS_VST2BLIF_H

#define MAXTOKENLEN 256
#define MAXNAMELEN 32

/*
 * These are the test used in the tokenizer, please refer
 * to the graph included in the documentation
 */
enum TOKEN_STATES { tZERO, tTOKEN, tREM1, tREM2, tSTRING, tEOF };

/*
 * Following are the structures containing all the info extracted from
 * the library (all the pins, the inputs, the outputs and, if needed, the clock)
 */

struct PortName {
  char name[MAXNAMELEN];
  int notused; // this flag is used to check if a formal terminal has been
               // already connected
  struct PortName *next;
};

struct Ports {
  char name[MAXNAMELEN];
};

struct Cell {
  char name[MAXNAMELEN];
  int npins;
  char type;
  struct SIGstruct *io;
  struct Ports *formals;
  struct Cell *next;
};

struct LibCell {
  char name[MAXNAMELEN];
  int npins;
  char clk[MAXNAMELEN];
  struct Ports *formals;
  struct LibCell *next;
};

struct Instance {
  char name[MAXNAMELEN];
  struct Cell *what;
  struct Ports *actuals;
  struct Instance *next;
};

struct BITstruct {
  char name[MAXNAMELEN + 10];
  char dir;
  struct BITstruct *next;
};

struct SIGstruct {
  char name[MAXNAMELEN];
  char dir;
  int start;
  int end;
  struct SIGstruct *next;
};

struct TERMstruct {
  char name[MAXNAMELEN];
  int start;
  int end;
};

struct ENTITYstruct {
  char name[MAXNAMELEN];
  struct Cell *Components;
  struct Cell *EntityPort;
  struct SIGstruct *Internals;
  struct Instance *Net;
};

/**
 * Closes all the files and flushes the used memory
 */
void close_all();

/**
 * Displays an error message, and then exits
 * @param msg Message to print before exiting
 */
void print_error(char *msg);

/**
 * Compares to strings (in case insensitive)
 * @param name First string (tipically the token)
 * @param keywrd Second string (tipically a keyword)
 * @return 1 if the strings match, 0 otherwise
 */
int kwrd_cmp(char *name, char *keywrd);

/**
 * Gets the options from the command line, opens the
 * input and the output files and then read the
 * parameters from the library file
 * @param argc Number of arguments
 * @param argv Array of arguments
 */
void check_args(int argc, char **argv);

/**
 * This is the tokenizer, see the graph to understand
 * how it works
 * @param tok Pointer to the final buffer, which is a
 * copy of the internal token
 */
void get_next_token(char *tok);

void releaseBIT(struct BITstruct *ptr);

void releaseSIG(struct SIGstruct *ptr);

void addBIT(struct BITstruct **BITptr, char *name, char dir);

void addSIG(struct SIGstruct **SIGptr, char *name, char dir, int start,
            int end);

/**
 * Puts a message on stderr, write the current line
 * and then sends the current token back
 * @param msg Message to print on stderr
 */
void warning(char *msg);

/**
 * Sends to stderr a message then gets tokens until
 * a gicen one is reached
 * @param name Message to print
 * @param next Token to reach
 */
void vst_error(char *name, char *next);

/**
 * Checks if a token is a decimal number
 * @param string Token to check
 * @return Converted integer, or 0 if the string is
 * not a number
 */
int dec_number(char *string);

/**
 * A kind debugging procedure
 * @param cell Library file
 */
void print_gates(struct LibCell *cell);

/**
 * Scans the genlib data structure to find the type
 * of the cell
 * @param cell Library file
 * @param name Name of cell to match
 * @return Type of cell
 */
char get_type_of_cell(struct LibCell *cell, char *name);

/**
 * Tokenizer to scan the library file
 * @param Lib Library file
 * @param tok Filled with the new token
 */
void get_lib_token(FILE *Lib, char *tok);

/**
 * Check if a name has been already used in the expression
 * @param name Name to check
 * @param ptr Pointer to list of formals
 * @return Pointer if the name is used, NULL otherwise
 */
struct BITstruct *is_here(char *name, struct BITstruct *ptr);

struct LibCell *new_lib_cell(char *name, struct BITstruct *ports, int latch);

/**
 * Scans the library to get cells' names, their output
 * pins and the clock signals of the latches
 * @param LibName Name of library file
 * @return LibCell structure
 */
struct LibCell *scan_library(char *LibName);

/**
 * Returns a pointer to an element of the list of gates
 * that matches given name, otherwise return null
 * @param name Name to match
 * @param LIBRARY genlib data struct
 * @return A (void *) pointer
 */
struct LibCell *what_lib_gate(char *name, struct LibCell *LIBRARY);

struct Cell *new_cell(char *name, struct SIGstruct *Bports,
                      struct BITstruct *Fports, struct LibCell *Genlib);

/**
 * Returns a pointer to an element of the list of gates
 * that matches given name, otherwise return null
 * @param name Name to match
 * @param LIBRARY genlib data struct
 * @return A (void *) pointer
 */
struct Cell *what_gate(char *name, struct Cell *LIBRARY);

/**
 * Gets the port definition of an ENTITY or a COMPONENT
 * @param Cname Cell name
 * @return Cell struct
 */
struct Cell *get_port(char *Cname);

/**
 * Parses the entity statement
 * @param Entity Cell structure
 */
void get_entity(struct Cell **Entity);

/**
 * Parses the component statement
 * @param cell Cell structure
 */
void get_component(struct Cell **cell);

/**
 * Skips the signal definitions
 * @param Internals Signal structure
 */
void get_signal(struct SIGstruct **Internals);

void fill_term(struct TERMstruct *TERM, struct ENTITYstruct *Entity, int which,
               struct Cell *WhatCell);

/**
 * Gets a name of an actual terminal, that can be a single
 * token or 3 tokens long (if it's an element of a vector)
 */
void get_name(struct TERMstruct *TERM, struct ENTITYstruct *Entity, int which,
              struct Cell *WhatCell);

void change_internal(struct SIGstruct *Intern, char *name);

/**
 * Parses the netlist
 */
struct Instance *get_instance(char *name, struct ENTITYstruct *Entity);

/**
 * Parses the structure 'ARCHITECTURE'
 */
void get_architecture(struct ENTITYstruct *ENTITY);

void print_signals(char *msg, char typ, struct SIGstruct *Sptr);

void print_ordered_signals(struct Ports *Sptr, struct LibCell *Lptr);

void print_sls(struct ENTITYstruct *Entity);

/**
 * Switches between the two main states of the program:
 * the ENTITY parsing and the ARCHITECTURE parsing
 */
void parse_file();

#endif // SIS_VST2BLIF_H

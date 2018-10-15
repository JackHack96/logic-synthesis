#ifndef COM_INT_H
#define COM_INT_H

#include <setjmp.h>
#include <signal.h>
#include "network.h"
#include "list.h"
#include "avl.h"

#define MAX_STR 1024

typedef int (*PFI)();

typedef struct command_descr_struct command_descr_t;
struct command_descr_struct {
  char *name;
  PFI command_fp;
  int changes_network;
};

typedef struct alias_descr_struct alias_descr_t;
struct alias_descr_struct {
  char *name;
  int argc;
  char **argv;
};

typedef struct network_info_struct network_info_t;
struct network_info_struct {
  network_t *network;
  network_t *original;
  lsList history;
};

/* Strings for communicating with a graphical front end, used in commisc.c */

#define COM_GRAPHICS_MSG_START "#START-GRAPHICS-COMMAND\n"
#define COM_GRAPHICS_MSG_END "#END-GRAPHICS-COMMAND\n"

char sis_hist_char;
char sis_shell_char;

avl_tree *command_table;
avl_tree *flag_table;
avl_tree *alias_table;
network_t *backup_network;

int com_alias();

int com_unalias();

int com_execute();

int com_help();

int com_time();

int com_echo();

int com_quit();

int com_set_variable();

int com_unset_variable();

int com_source();

int com_undo();

void com_free_argv();

void com_alias_free();

void com_command_free();

char *fgets_filec();

char *hist_subst();

void print_prompt();

#endif //COM_INT_H
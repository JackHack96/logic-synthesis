
#include <setjmp.h>
#include <signal.h>

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

extern char sis_hist_char;
extern char sis_shell_char;

extern avl_tree *command_table;
extern avl_tree *flag_table;
extern avl_tree *alias_table;
extern network_t *backup_network;

extern int com_alias();

extern int com_unalias();

extern int com_execute();

extern int com_help();

extern int com_time();

extern int com_echo();

extern int com_quit();

extern int com_set_variable();

extern int com_unset_variable();

extern int com_source();

extern int com_undo();

extern void com_free_argv();

extern void com_alias_free();

extern void com_command_free();

extern char *fgets_filec();

extern char *hist_subst();

extern void print_prompt();

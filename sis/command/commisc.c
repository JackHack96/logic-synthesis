
#include "com_int.h"
#include "sis.h"

/* If graphics commands are enabled, this is	*/
/* the stream where they should be written to.	*/

static FILE *com_graphics_stream = NULL;
static int com_graphics_is_opened = 0;

/* ARGSUSED */
int com_time(network_t **network, int argc, char **argv) {
  static long last_time = 0;
  long time;

  if (argc != 1)
    goto usage;

  time = util_cpu_time();
  fprintf(sisout, "elapse: %2.1f seconds, total: %2.1f seconds\n",
          (time - last_time) / 1000.0, time / 1000.0);
  last_time = time;
  return 0;

usage:
  fprintf(siserr, "usage: time\n");
  return 1;
}

/* ARGSUSED */
int com_usage(network, argc, argv) network_t **network;
int argc;
char **argv;
{
  if (argc != 1) {
    (void)fprintf(siserr, "usage: usage -- give CPU usage statistics\n");
    return 1;
  }

  util_print_cpu_stats(sisout);
  return 0;
}

/* ARGSUSED */
int com_echo(network_t **network, int argc, char **argv) {
  int i;

  for (i = 1; i < argc; i++) {
    (void)fprintf(sisout, "%s ", argv[i]);
  }
  (void)fprintf(sisout, "\n");
  return 0;
}

/*
 * A return value of -1 indicates a quick quit, -2 return frees the memory
 */
int com_quit(network_t **network, int argc, char **argv) {
  if (argc == 2 && strncmp(argv[1], "-s", 2) == 0) {
    return -2;
  }
  return -1;
}

/* ARGSUSED */
static int com_infinite_loop(network_t **network, int argc, char **argv) {
  *network = 0;
  for (;;)
    ;
}

/* ARGSUSED */
static int com_save_image(network_t *network, int argc, char **argv) {
  FILE *fp;
  char *file1, *file2;

  if (argc != 2) {
    (void)fprintf(stderr, "usage: save filename\n");
    return 1;
  }

  /* get current executable name by searching the path ... */
  file1 = util_path_search(program_name);
  if (file1 == 0) {
    (void)fprintf(siserr, "cannot locate current executable\n");
    return 1;
  }

  /* users name for the new executable -- perform tilde-expansion */
  fp = com_open_file(argv[1], "w", &file2, /* silent */ 0);
  if (fp == NULL)
    return 1;
  if (fp != stdout)
    (void)fclose(fp);

  if (!util_save_image(file1, file2)) {
    (void)fprintf(stderr, "error occured during save ...\n");
    return 1;
  }
  FREE(file1);
  FREE(file2);
  return 0;
}

/* ARGSUSED */
int com_which(network_t **network, int argc, char **argv) {
  FILE *fp;
  char *filename;

  if (argc != 2)
    return 1;

  fp = com_open_file(argv[1], "r", &filename, 0);
  if (fp != 0) {
    (void)fprintf(sisout, "%s\n", filename);
    (void)fclose(fp);
  }
  FREE(filename);
  return 0;
}

/* ARGSUSED */
int com_best(network_t **network, int argc, char **argv) {
  static int bst_count = -1;
  static network_t *bst_network;
  int count = 0;
  node_t *node;
  lsGen gen;

  foreach_node(*network, gen, node) if (node->type == INTERNAL) count +=
      factor_num_literal(node);

  if (bst_count < 0) {
    bst_network = network_dup(*network);
    bst_count = count;
  } else if (count <= bst_count) {
    network_free(bst_network);
    bst_network = network_dup(*network);
    bst_count = count;
  } else {
    network_free(*network);
    *network = network_dup(bst_network);
  }
  return 0;
}

/*ARGSUSED*/
int com_history(network_t **network, int argc, char **argv) {
  int i, num, lineno;
  int size;
  if (argc > 3) {
  usage:
    (void)fprintf(siserr, "usage: history [-h] [num]\n");
    return (1);
  }
  num = 30;
  lineno = 1;
  for (i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      if (argv[i][1] == 'h') {
        lineno = 0;
      } else {
        goto usage;
      }
    } else {
      num = atoi(argv[i]);
      if (num <= 0) {
        goto usage;
      }
    }
  }
  size = array_n(command_hist);
  num = (num < size) ? num : size;
  for (i = size - num; i < size; i++) {
    if (lineno != 0) {
      (void)fprintf(sisout, "%d\t", i + 1);
    }
    (void)fprintf(sisout, "%s\n",

                  array_fetch(char *, command_hist, i)

    );
  }
  return (0);
}

int com_graphics_enabled(void) {
  /*	Returns 1 if graphics commands are enabled.  Other packages should
      not add graphics commands if this function returns 0. */

  return (com_graphics_stream != NULL);
}

FILE *com_graphics_open(char *type, char *title, char *cmd) {
  /*	Start a graphics command and return the stream to use. */

  assert(!com_graphics_is_opened);
  assert(com_graphics_stream != NULL);
  com_graphics_is_opened = 1;
  fputs(COM_GRAPHICS_MSG_START, com_graphics_stream);
  fprintf(com_graphics_stream, "%s\t%s\t%s\n", type, cmd, title);
  return com_graphics_stream;
}

void com_graphics_close(FILE *stream) {
  /*	Finish the graphics command with a graphics trailer. */

  assert(stream == com_graphics_stream);
  fputs(COM_GRAPHICS_MSG_END, stream);
  fflush(stream);
  com_graphics_is_opened = 0;
}

void com_graphics_exec(char *type, char *title, char *cmd, char *data) {
  /*	Convenience routine for sending simple graphics commands. */

  if (com_graphics_enabled()) {
    FILE *fp = com_graphics_open(type, title, cmd);
    fputs(data, fp);
    fputc('\n', fp);
    com_graphics_close(fp);
  }
}

void com_graphics_help(void) {
  /*	If graphics is enabled, send list of help topics. */

  avl_generator *gen;
  char *key;
  FILE *fp;

  if (!com_graphics_enabled())
    return;
  fp = com_graphics_open("sis", "sis", "commands");
  avl_foreach_item(command_table, gen, AVL_FORWARD, &key, NIL(char *)) {
    fprintf(fp, "%s\n", key);
  }
  com_graphics_close(fp);
}

/* graphics_flag is the file descriptor for graphics commands, zero means
   graphics is disabled.  Other packages can call com_graphics_enabled to
   selectively add graphics commands. */

int init_command(int graphics_flag) {
  char *path;
  char *lib_name;
  FILE *gfp;

  if (graphics_flag != 0) {
    com_graphics_stream = (graphics_flag == 1) ? stdout : stderr;
    /* Open the main sis command window. */
    gfp = com_graphics_open("cmd", "cmd", "new");
    fprintf(gfp, ".version\t%s\n",

            sis_version()

    );
    com_graphics_close(gfp);
  }

  command_table = avl_init_table(strcmp);
  flag_table = avl_init_table(strcmp);
  alias_table = avl_init_table(strcmp);

  com_add_command("alias", com_alias, 0);
  com_add_command("echo", com_echo, 0);
  com_add_command("help", com_help, 0);
  com_add_command("quit", com_quit, 0);
  com_add_command("save", com_save_image, 0);
  com_add_command("source", com_source, 0);
  com_add_command("_iloop", com_infinite_loop, 1);
  com_add_command("undo", com_undo, 0);
  com_add_command("set", com_set_variable, 0);
  com_add_command("unalias", com_unalias, 0);
  com_add_command("unset", com_unset_variable, 0);
  com_add_command("time", com_time, 0);
  com_add_command("usage", com_usage, 0);
  com_add_command("history", com_history, 0);
  com_add_command("_which", com_which, 0);
  com_add_command("_best", com_best, 1);

  /* set the default open_path */
  lib_name = sis_library();
  path = ALLOC(char, strlen(lib_name) + 20);
  sprintf(path, "set open_path .:%s", lib_name);
  com_execute(NULL, path);
  FREE(lib_name);
  FREE(path);
}

int end_command(void) {
  avl_free_table(flag_table, free, free);
  avl_free_table(command_table, (void (*)())0, com_command_free);
  avl_free_table(alias_table, (void (*)())0, com_alias_free);
  if (backup_network != NIL(network_t)) {
    network_free(backup_network);
  }
  error_cleanup();
}

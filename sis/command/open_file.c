
#include "com_int.h"
#include "sis.h"

FILE *com_open_file(filename, mode, real_filename_p, silent) char *filename;
char *mode;
char **real_filename_p;
int silent;
{
  char *real_filename, *path, *user_path;
  char *lib_name;
  FILE *fp;

  if (strcmp(filename, "-") == 0) {
    if (strcmp(mode, "w") == 0) {
      real_filename = util_strsav("stdout");
      fp = stdout;
    } else {
      real_filename = util_strsav("stdin");
      fp = stdin;
    }
  } else {
    real_filename = NIL(char);
    if (strcmp(mode, "r") == 0) {
      user_path = com_get_flag("open_path");
      if (user_path != NIL(char)) {
        lib_name = sis_library();
        path = ALLOC(char, strlen(user_path) + strlen(lib_name) + 10);
        (void)sprintf(path, "%s:%s", user_path, lib_name);
        /* If the filename begins with ./, ../, ~/, or /, AND the file doesn't
           actually exist, then SIS will look in the open path (which includes
           the sis library) for the file.  This could lead to unexpected
           behavior: the user is looking for ./msu.genlib, and since that isn't
           there, the users gets sis_lib/msu.genlib, and no error is reported.
           The following pseudo code fixes this:

                        if (the beginning of file_name is : ./ || ../ || ~/ ||
           /) { real_filename = util_file_search(filename, NIL(char), "r"); }
           else
        */
        real_filename = util_file_search(filename, path, "r");
        FREE(path);
        FREE(lib_name);
      }
    }
    if (real_filename == NIL(char)) {
      real_filename = util_tilde_expand(filename);
    }
    if ((fp = fopen(real_filename, mode)) == NULL) {
      if (!silent) {
        perror(real_filename);
      }
    }
  }
  if (real_filename_p != 0) {
    *real_filename_p = real_filename;
  } else {
    FREE(real_filename);
  }
  return fp;
}

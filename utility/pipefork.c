#include "../port/copyright.h"
#include "../port/port.h"
#include <sys/wait.h>
#include <unistd.h>

/*
 * util_pipefork - fork a command and set up pipes to and from
 *
 * Rick L Spickelmier, 3/23/86
 * Richard Rudell, 4/6/86
 * Rick L Spickelmier, 4/30/90, got rid of slimey vfork semantics
 *
 * Returns:
 *   1 for success, with toCommand and fromCommand pointing to the streams
 *   0 for failure
 */

/**
 * Fork (using execvp(3)) the program argv[0] with argv[1] ... argv[n] as
 * arguments (argv[n+1] is set to NIL(char) to indicate the end of the list).
 * Set up two-way pipes between	the child process and the parent, returning file
 * pointer 'toCommand' which can be used to write information to the child, and
 * the file pointer `fromCommand' which can be used to read information from the
 * child.  As always with unix pipes,	watch out for dead-locks.
 * @return Returns 1 for success, 0 if any failure occured forking the child.
 */
int util_pipefork(char **argv, FILE **toCommand, FILE **fromCommand, int *pid) {
#ifdef unix
  int forkpid, mywaitpid;
  int topipe[2], frompipe[2];
  char buffer[1024];
  int status;

  /* create the PIPES...
   * fildes[0] for reading from command
   * fildes[1] for writing to command
   */
  (void)pipe(topipe);
  (void)pipe(frompipe);

  if ((forkpid = vfork()) == 0) {
    /* child here, connect the pipes */
    (void)dup2(topipe[0], fileno(stdin));
    (void)dup2(frompipe[1], fileno(stdout));

    (void)close(topipe[0]);
    (void)close(topipe[1]);
    (void)close(frompipe[0]);
    (void)close(frompipe[1]);

    (void)execvp(argv[0], argv);
    (void)sprintf(buffer, "util_pipefork: can not exec %s", argv[0]);
    perror(buffer);
    (void)_exit(1);
  }

  if (pid) {
    *pid = forkpid;
  }

  mywaitpid = wait3(&status, WNOHANG, 0);

  /* parent here, use slimey vfork() semantics to get return status */
  if (mywaitpid == forkpid && WIFEXITED(status)) {
    return 0;
  }
  if ((*toCommand = fdopen(topipe[1], "w")) == NULL) {
    return 0;
  }
  if ((*fromCommand = fdopen(frompipe[0], "r")) == NULL) {
    return 0;
  }
  (void)close(topipe[0]);
  (void)close(frompipe[1]);
  return 1;
#else
  (void)fprintf(stderr,
                "util_pipefork: not implemented on your operating system\n");
  return 0;
#endif
}

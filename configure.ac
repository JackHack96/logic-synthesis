# Initialization.

AC_PREREQ([2.5])
AC_INIT([SIS], [1.3.6], [pchong@ic.eecs.berkeley.edu])
AM_INIT_AUTOMAKE([foreign no-dependencies])
AC_CONFIG_SRCDIR([sis/node/node.c])
AM_CONFIG_HEADER([config.h])
AC_COPYRIGHT([
Copyright (c) 1994-2005 The Regents of the University of California.
All Rights Reserved.

Permission to use, copy, modify, and distribute this software and its
documentation for educational, research and non-profit purposes,
without fee, and without a written agreement is hereby granted,
provided that the above copyright notice, this paragraph and the
following three paragraphs appear in all copies.

Permission to incorporate this software into commercial products may
be obtained by contacting the University of California.
	    Brian Donohue
	    Contracts Administrator
	    6701 San Pablo Avenue #218
	    Berkeley, CA 94720-5600
	    (510) 642-3128
	    donohue@uclink.berkeley.edu

This software program and documentation are copyrighted by The Regents
of the University of California. The software program and
documentation are supplied "as is", without any accompanying services
from The Regents. The Regents does not warrant that the operation of
the program will be uninterrupted or error-free. The end-user
understands that the program was developed for research purposes and
is advised not to rely exclusively on the program for any reason.

IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY
FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES,
INCLUDING LOST PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND
ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA HAS BEEN
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. THE UNIVERSITY OF
CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE. THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATIONS TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
])

# Configuration options.

enable_cmubdd="yes"
# UCB BDD package breaks power package.
#AC_ARG_ENABLE([ucbbdd], AC_HELP_STRING([--enable-ucbbdd],
#    [use UCB BDD package (default: no)]), , [enable_ucbbdd="no"])
AC_ARG_WITH([cudd], AC_HELP_STRING([--with-cudd=CUDD_DIR],
    [directory with CUDD sources (default: none)]), , [with_cudd=""])

# Check configuration consistency.

if test "$with_cudd" ; then
  AC_CHECK_FILE(["$with_cudd/cudd/cuddInt.h"], ,
      AC_MSG_ERROR([cannot find cudd/cuddInt.h in "$with_cudd"
use --with-cudd=CUDD_DIR with the correct CUDD source directory]))
  enable_ucbbdd="no"
  enable_cmubdd="no"
fi
if test "$enable_ucbbdd" = "yes" ; then
  enable_cmubdd="no"
fi

# Checks for programs.

AC_LANG([C])
AC_PROG_CC
AC_PROG_YACC
AM_PROG_LEX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB

# Checks for libraries.

#AC_CHECK_LIB([m], [sqrt])

# Checks for header files.

AC_PATH_X
AC_HEADER_STDC
AC_HEADER_DIRENT
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([unistd.h])
#AC_CHECK_HEADERS([fcntl.h limits.h memory.h sgtty.h stddef.h \
#    strings.h sys/file.h sys/ioctl.h sys/param.h sys/time.h termio.h \
#    termios.h])

# Checks for typedefs, structures, and compiler characteristics.

AC_C_CONST

# Checks for library functions.

AC_TYPE_SIGNAL
AC_CHECK_FUNCS([memcpy memset pclose popen strchr strrchr])

# Generate variables.

AM_CONDITIONAL([SIS_COND_X], [test -z "$no_x"])
AM_CONDITIONAL([SIS_COND_CMUBDD], [test "$enable_cmubdd" = "yes"])
AM_CONDITIONAL([SIS_COND_UCBBDD], [test "$enable_ucbbdd" = "yes"])
AM_CONDITIONAL([SIS_COND_CUDD], [test "$with_cudd"])

AC_SUBST([SIS_X_INCLUDES], ["$x_includes"])
AC_SUBST([SIS_X_LIBRARIES], ["$x_libraries"])
AC_SUBST([SIS_CUDDDIR], ["$with_cudd"])

AC_SUBST([SIS_DOCDIR], ["$datadir/$PACKAGE/doc"])
AC_SUBST([SIS_HISTDIR], ["$datadir/$PACKAGE/historical"])
AC_SUBST([SIS_SISLIBDIR], ["$datadir/$PACKAGE/sis_lib"])

# Generate output.

AC_CONFIG_FILES([Makefile
    blif2vst/Makefile
    doc/Makefile
    espresso/Makefile
    espresso/examples/Makefile
    jedi/Makefile
    nova/Makefile
    options/Makefile
    port/Makefile
    sis/Makefile
    sis/array/Makefile
    sis/astg/Makefile
    sis/atpg/Makefile
    sis/avl/Makefile
    sis/bdd_cmu/Makefile
    sis/bdd_cmu/bdd_cmu/Makefile
    sis/bdd_cmu/bdd_port/Makefile
    sis/bdd_cmu/mem/Makefile
    sis/bdd_cudd/Makefile
    sis/bdd_ucb/Makefile
    sis/clock/Makefile
    sis/command/Makefile
    sis/decomp/Makefile
    sis/delay/Makefile
    sis/doc/Makefile
    sis/enc/Makefile
    sis/error/Makefile
    sis/espresso/Makefile
    sis/extract/Makefile
    sis/factor/Makefile
    sis/gcd/Makefile
    sis/genlib/Makefile
    sis/graph/Makefile
    sis/graphics/Makefile
    sis/include/Makefile
    sis/io/Makefile
    sis/latch/Makefile
    sis/linsolv/Makefile
    sis/list/Makefile
    sis/lsort/Makefile
    sis/main/Makefile
    sis/map/Makefile
    sis/maxflow/Makefile
    sis/mincov/Makefile
    sis/minimize/Makefile
    sis/network/Makefile
    sis/node/Makefile
    sis/ntbdd/Makefile
    sis/octio/Makefile
    sis/order/Makefile
    sis/phase/Makefile
    sis/pld/Makefile
    sis/power/Makefile
    sis/resub/Makefile
    sis/retime/Makefile
    sis/seqbdd/Makefile
    sis/sim/Makefile
    sis/simplify/Makefile
    sis/sis_lib/Makefile
    sis/sis_lib/help/Makefile
    sis/sparse/Makefile
    sis/speed/Makefile
    sis/st/Makefile
    sis/stg/Makefile
    sis/test/Makefile
    sis/timing/Makefile
    sis/util/Makefile
    sis/var_set/Makefile
    sred/Makefile
    stamina/Makefile
    stamina/hash/Makefile
    stamina/mimi/Makefile
    utility/Makefile
    vst2blif/Makefile
    xsis/Makefile])
AC_OUTPUT

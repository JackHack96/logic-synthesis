
#include "sis.h"

#ifndef CUR_DATE
#define CUR_DATE "<compile date not supplied>"
#endif

#ifndef CUR_VER
#ifdef SIS
#define CUR_VER "UC Berkeley, " PACKAGE_STRING
#else
#define CUR_VER "UC Berkeley, MIS Release 2.2"
#endif
#endif

#ifndef LIBRARY
#define LIBRARY "/projects/sis/sis/common/sis_lib"
#endif

/*
 * Returns the date in a brief format assuming its coming from
 * the program `date'.
 */
static char *proc_date(char *datestr) {
    static char result[25];
    char        day[10], month[10], zone[10], *at;
    int         date, hour, minute, second, year;

    if (sscanf(datestr, "%s %s %2d %2d:%2d:%2d %s %4d", day, month, &date, &hour,
               &minute, &second, zone, &year) == 8) {
        if (hour >= 12) {
            if (hour >= 13)
                hour -= 12;
            at = "PM";
        } else {
            if (hour == 0)
                hour = 12;
            at       = "AM";
        }
        (void) sprintf(result, "%d-%3s-%02d at %d:%02d %s", date, month, year % 100,
                       hour, minute, at);
        return result;
    } else {
        return datestr;
    }
}

/*
 * Returns the current SIS version string to the caller.
 */

char *sis_version() {
    static char version[1024];

    (void) sprintf(version, "%s (compiled %s)", CUR_VER, proc_date(CUR_DATE));
    return version;
}

/*
 *  Returns the SIS library path (usually ~cad/lib/sis/lib) to the caller
 */

char *sis_library() { return util_tilde_expand(LIBRARY); }

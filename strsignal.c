/** \file strsignal.c -- return text for given signal.
 * \author Matthias Andree <matthias.andree@gmx.de>
 * (C) Copyright 2006 by Matthias Andree
 *
 * GNU GENERAL PUBLIC LICENSE V2 (by reference)
 */

#include "config.h"
#include <signal.h>

/** If the system supports it, return the spelled-out name of signal
 * \a sig. Otherwise, return just the signal number.
 * \return a string that the user must not modify. It may be
 * overwritten by the next call to this function.
 */
char *strsignal(int sig)
{
    static char buf[24];
#ifdef SYS_SIGLIST_DECLARED
    if (sig > 0 && sig < NSIG) {
	return sys_siglist[sig];
    }
#endif
    snprintf(buf, sizeof(buf), "%d", sig);
    return buf;
}

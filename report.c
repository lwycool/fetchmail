/* report.c -- report function for noninteractive utilities
 *
 * For license terms, see the file COPYING in this directory.
 *
 * This code is distantly descended from the error.c module written by
 * David MacKenzie <djm@gnu.ai.mit.edu>.  It was redesigned and
 * rewritten by Dave Bodenstab, then redesigned again by ESR, then
 * bludgeoned into submission for SunOS 4.1.3 by Chris Cheyney
 * <cheyney@netcom.com>.  It works even when the return from
 * vprintf(3) is unreliable.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <stdio.h>
#include <errno.h>
#include <string.h>
#if defined(HAVE_SYSLOG)
#include <syslog.h>
#endif
#include "i18n.h"
#include "fetchmail.h"

#include <stdarg.h>

#define MALLOC(n)	xmalloc(n)
#define REALLOC(n,s)	xrealloc(n,s)

/* If NULL, report will flush stderr, then print on stderr the program
   name, a colon and a space.  Otherwise, report will call this
   function without parameters instead.  */
static void (*report_print_progname) (void);

/* Used by report_build() and report_complete() to accumulate partial messages.
 */
static unsigned int partial_message_size = 0;
static unsigned int partial_message_size_used = 0;
static char *partial_message;
static unsigned use_stderr;
static unsigned int use_syslog;

/* This variable is incremented each time `report' is called.  */
static unsigned int report_message_count;

#ifdef _LIBC
/* In the GNU C library, there is a predefined variable for this.  */

# define program_name program_invocation_name
# include <errno.h>

#else

# if !HAVE_STRERROR && !defined(strerror)
char *strerror (int errnum)
{
    extern char *sys_errlist[];
    extern int sys_nerr;

    if (errnum > 0 && errnum <= sys_nerr)
	return sys_errlist[errnum];
    return GT_("Unknown system error");
}
# endif	/* HAVE_STRERROR */
#endif	/* _LIBC */

/* Print the program name and error message MESSAGE, which is a printf-style
   format string with optional args.
   If ERRNUM is nonzero, print its corresponding system error message. */

void
report (FILE *errfp, const char *message, ...)
{
    va_list args;

    /* If a partially built message exists, print it now so it's not lost.  */
    if (partial_message_size_used != 0)
    {
	partial_message_size_used = 0;
	report (errfp, GT_("%s (log message incomplete)"), partial_message);
    }

#if defined(HAVE_SYSLOG)
    if (use_syslog)
    {
	int priority;

	va_start(args, message);
	priority = (errfp == stderr) ? LOG_ERR : LOG_INFO;

#ifdef HAVE_VSYSLOG
	vsyslog (priority, message, args);
#else
	{
	    /* XXX FIXME: use snprintf twice, to obtain buffer size,
	     * and then allocate and format, and then syslog the
	     * complete message */
	    char *a1 = va_arg(args, char *);
	    char *a2 = va_arg(args, char *);
	    char *a3 = va_arg(args, char *);
	    char *a4 = va_arg(args, char *);
	    char *a5 = va_arg(args, char *);
	    char *a6 = va_arg(args, char *);
	    char *a7 = va_arg(args, char *);
	    char *a8 = va_arg(args, char *);
	    syslog (priority, message, a1, a2, a3, a4, a5, a6, a7, a8);
	}
#endif

	va_end(args);
    }
    else
#endif
    {
	if (report_print_progname)
	    (*report_print_progname) ();
	else
	{
	    fflush (errfp);
	    if ( *message == '\n' )
	    {
		fputc( '\n', errfp );
		++message;
	    }
	    fprintf (errfp, "%s: ", program_name);
	}

	va_start(args, message);
# if defined(HAVE_VPRINTF) || defined(_LIBC)
	vfprintf (errfp, message, args);
# else
	_doprnt (message, args, errfp);
# endif
	va_end (args);
	fflush (errfp);
    }
    ++report_message_count;
}

/*
 * Calling report_init(1) causes report_build and report_complete to write
 * to errfp without buffering.  This is needed for the ticker dots to
 * work correctly.
 */
void report_init(int mode)
{
    switch(mode)
    {
    case 0:			/* errfp, buffered */
    default:
	use_stderr = FALSE;
	use_syslog = FALSE;
	break;

    case 1:			/* errfp, unbuffered */
	use_stderr = TRUE;
	use_syslog = FALSE;
	break;

#ifdef HAVE_SYSLOG
    case -1:			/* syslogd */
	use_stderr = FALSE;
	use_syslog = TRUE;
	break;
#endif /* HAVE_SYSLOG */
    }
}

/* Build an report message by appending MESSAGE, which is a printf-style
   format string with optional args, to the existing report message (which may
   be empty.)  The completed report message is finally printed (and reset to
   empty) by calling report_complete().
   If an intervening call to report() occurs when a partially constructed
   message exists, then, in an attempt to keep the messages in their proper
   sequence, the partial message will be printed as-is (with a trailing 
   newline) before report() prints its message. */

static void rep_ensuresize(void) {
    /* Make an initial guess for the size of any single message fragment.  */
    if (partial_message_size == 0)
    {
	partial_message_size_used = 0;
	partial_message_size = 2048;
	partial_message = MALLOC (partial_message_size);
    }
    else
	if (partial_message_size - partial_message_size_used < 1024)
	{
	    partial_message_size += 2048;
	    partial_message = REALLOC (partial_message, partial_message_size);
	}
}

void
report_build (FILE *errfp, const char *message, ...)
{
    va_list args;
    int n;

    rep_ensuresize();

    va_start(args, message);
    for ( ; ; )
    {
	n = vsnprintf (partial_message + partial_message_size_used,
		       partial_message_size - partial_message_size_used,
		       message, args);

	if (n < partial_message_size - partial_message_size_used)
        {
	    partial_message_size_used += n;
	    break;
	}

	partial_message_size += 2048;
	partial_message = REALLOC (partial_message, partial_message_size);
    }
    va_end (args);

    if (use_stderr && partial_message_size_used != 0)
    {
	partial_message_size_used = 0;
	fputs(partial_message, errfp);
    }
}

/* Complete a report message by appending MESSAGE, which is a printf-style
   format string with optional args, to the existing report message (which may
   be empty.)  The completed report message is then printed (and reset to
   empty.) */

void
report_complete (FILE *errfp, const char *message, ...)
{
    va_list args;
    int n;

    rep_ensuresize();

    va_start(args, message);
    for ( ; ; )
    {
	n = vsnprintf (partial_message + partial_message_size_used,
		       partial_message_size - partial_message_size_used,
		       message, args);

	if (n < partial_message_size - partial_message_size_used)
        {
	    partial_message_size_used += n;
	    break;
	}

	partial_message_size += 2048;
	partial_message = REALLOC (partial_message, partial_message_size);
    }
    va_end (args);

    /* Finally... print it.  */
    partial_message_size_used = 0;

    if (use_stderr)
    {
	fputs(partial_message, errfp);
	fflush (errfp);

	++report_message_count;
    }
    else
	report(errfp, "%s", partial_message);
}

/* Sometimes we want to have at most one error per line.  This
   variable controls whether this mode is selected or not.  */
static int error_one_per_line;

void
report_at_line (FILE *errfp, int errnum, const char *file_name,
	       unsigned int line_number, const char *message, ...)
{
    va_list args;

    if (error_one_per_line)
    {
	static const char *old_file_name;
	static unsigned int old_line_number;

	if (old_line_number == line_number &&
	    (file_name == old_file_name || !strcmp (old_file_name, file_name)))
	    /* Simply return and print nothing.  */
	    return;

	old_file_name = file_name;
	old_line_number = line_number;
    }

    if (report_print_progname)
	(*report_print_progname) ();
    else
    {
	fflush (errfp);
	if ( *message == '\n' )
	{
	    fputc( '\n', errfp );
	    ++message;
	}
	fprintf (errfp, "%s:", program_name);
    }

    if (file_name != NULL)
	fprintf (errfp, "%s:%u: ", file_name, line_number);

    va_start(args, message);
# if defined(HAVE_VPRINTF) || defined(_LIBC)
    vfprintf (errfp, message, args);
# else
    _doprnt (message, args, errfp);
# endif
    va_end (args);

    ++report_message_count;
    if (errnum)
	fprintf (errfp, ": %s", strerror (errnum));
    putc ('\n', errfp);
    fflush (errfp);
}

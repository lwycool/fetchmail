/*
    Copyright (C) Ben Kibbey <bjk@luxsci.net>

    For license terms, see the file COPYING in this directory.
*/
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <ctype.h>
#include <libpwmd.h>
#include "gettext.h"
#include "pwmd.h"

static void exit_with_pwmd_error(gpg_error_t rc)
{
    report(stderr, GT_("pwmd: ERR %i: %s\n"), rc, gpg_strerror(rc));

    if (pwm) {
	pwmd_close(pwm);
	pwm = NULL;
    }

    /* Don't exit if daemonized. There may be other active accounts. */
    if (isatty(STDOUT_FILENO))
	exit(PS_UNDEFINED);
}

static gpg_error_t status_cb(void *data, const char *line)
{
    (void)data;
    if (!strncmp(line, "LOCKED", 6))
	report(stderr, "pwmd(%s): %s\n", pwmd_file, line);

    return 0;
}

#define MAX_PWMD_ARGS 8
static gpg_error_t parse_socket_args (const char *str, char ***args)
{
  gpg_error_t rc = 0;
  char buf[2048], *b;
  const char *p;
  int n = 0;
  static char *result[MAX_PWMD_ARGS] = {0};

  for (n = 0; n < MAX_PWMD_ARGS; n++)
      result[n] = NULL;

  n = 0;

  for (b = buf, p = str; *p; p++)
    {
      if (*p == ',')
	{
	  *b = 0;
	  result[n] = xstrdup (buf);
	  if (!result[n])
	    {
	      rc = gpg_error (GPG_ERR_ENOMEM);
	      break;
	    }

	  b = buf;
	  *b = 0;
	  n++;
	  continue;
	}

      *b++ = *p;
    }

  if (!rc && buf[0])
    {
      *b = 0;
      result[n] = xstrdup (buf);
      if (!result[n])
	  rc = gpg_error (GPG_ERR_ENOMEM);
    }

  if (rc)
    {
	for (n = 0; n < MAX_PWMD_ARGS; n++)
	{
	    xfree(result[n]);
	    result[n] = NULL;
	}
    }
  else
    *args = result;

  return rc;
}

int connect_to_pwmd(const char *socketname, const char *socket_args,
		    const char *filename)
{
    gpg_error_t rc;

    pwmd_init();

    /* Try to reuse an existing connection for another account on the same pwmd
     * server. If the socket name or any socket arguments have changed then
     * reopen the connection. */
    if (!pwm || (socketname && !pwmd_socket)
	    || (!socketname && pwmd_socket) ||
            (socketname && pwmd_socket && strcmp(socketname, pwmd_socket))
            || (socket_args && !pwmd_socket_args)
            || (!socket_args && pwmd_socket_args)
            || (socket_args && pwmd_socket_args
                && strcmp (socket_args, pwmd_socket_args))) {
	pwmd_close(pwm);
	rc = pwmd_new("fetchmail", &pwm);
	if (rc) {
	    exit_with_pwmd_error(rc);
	    return 1;
	}

	rc = pwmd_setopt (pwm, PWMD_OPTION_SOCKET_TIMEOUT, 120);
	if (!rc)
	    rc = pwmd_setopt(pwm, PWMD_OPTION_STATUS_CB, status_cb);

	if (rc) {
	    exit_with_pwmd_error(rc);
	    return 1;
	}

	if (socket_args) {
	    char **args = NULL;
	    int n;

	    rc = parse_socket_args (socket_args, &args);
	    if (!rc) {
		rc = pwmd_connect(pwm, socketname, args[0], args[1], args[2],
				  args[3], args[4], args[5], args[6], args[7]);

		for (n = 0; n < MAX_PWMD_ARGS; n++)
		    xfree(args[n]);
	    }
	}
	else
            rc = pwmd_connect(pwm, socketname, NULL);

	if (rc) {
	    exit_with_pwmd_error(rc);
	    return 1;
	}

	pwmd_setopt(pwm, PWMD_OPTION_PINENTRY_DESC, NULL);
	rc = pwmd_command (pwm, NULL, NULL, NULL, NULL, "OPTION lock-timeout=300");
	if (rc) {
	    exit_with_pwmd_error(rc);
	    return 1;
	}

	rc = pwmd_setopt(pwm, PWMD_OPTION_LOCK_ON_OPEN, 0);
	if (rc) {
	    exit_with_pwmd_error(rc);
	    return 1;
	}
    }

    if (run.pinentry_timeout > 0) {
	rc = pwmd_setopt(pwm, PWMD_OPTION_PINENTRY_TIMEOUT,
		run.pinentry_timeout);
	if (rc) {
	    exit_with_pwmd_error(rc);
	    return 1;
	}
    }

    if (!pwmd_file || strcmp(filename, pwmd_file)) {
	/* Temporarily set so status_cb knows what file. */
	pwmd_file = filename;
	rc = pwmd_open(pwm, filename, NULL, NULL);
	pwmd_file = NULL;
	if (rc) {
	    exit_with_pwmd_error(rc);
	    return 1;
	}
    }

    /* May be null to use the default of ~/.pwmd/socket. */
    pwmd_socket = socketname;
    pwmd_socket_args = socket_args;
    pwmd_file = filename;
    return 0;
}

static int get_element(char **result, int required, const char *fmt, ...)
{
    gpg_error_t rc;
    va_list ap;
    char path[1002]; // ASSUAN_LINELENGTH

    *result = NULL;
    va_start(ap, fmt);
    vsnprintf(path, sizeof(path), fmt, ap);
    va_end(ap);
    rc = pwmd_command(pwm, result, NULL, NULL, NULL, "GET %s", path);

    if (rc) {
	if (gpg_err_code(rc) == GPG_ERR_ELEMENT_NOT_FOUND) {
	    char *tmp = xstrdup(path), *p;

            for (p = tmp; *p; p++) {
                if (*p == '\t')
                  *p = '/';
	    }

	    report(stderr, GT_("%spwmd(%s): %s: %s\n"),
		    required ? "" : GT_("WARNING: "), pwmd_file, tmp,
		    gpg_strerror(rc));
	    xfree(tmp);

	    if (!required)
		return 0;

	    pwmd_close(pwm);
	    pwm = NULL;

	    if (isatty(STDOUT_FILENO))
		exit(PS_SYNTAX);

	    return 1;
	}

	exit_with_pwmd_error(rc);
	return 1;
    }

    return 0;
}

int get_pwmd_elements(const char *account, int protocol, struct query *ctl)
{
    const char *prot1 = showproto(protocol);
    char *result, *prot;
    char *tmp = xstrdup(account);
    int i;

    prot = xstrdup(prot1);

    for (i = 0; prot[i]; i++)
	prot[i] = tolower(prot[i]);

    for (i = 0; tmp[i]; i++) {
	if (i && tmp[i] == '^')
	    tmp[i] = '\t';
    }

    i = get_element(&result, 1, "%s\t%s\thostname", tmp, prot);
    if (i)
	goto done;

    if (ctl->server.pollname != ctl->server.via)
	xfree(ctl->server.via);
    ctl->server.via = xstrdup(result);
    pwmd_free(result);

    xfree(ctl->server.queryname);
    ctl->server.queryname = xstrdup(ctl->server.via);

    xfree(ctl->server.truename);
    ctl->server.truename = xstrdup(ctl->server.queryname);

    /*
     * Server port. Fetchmail tries standard ports for known services so it
     * should be alright if this element isn't found. ctl->server.protocol is
     * already set. This sets ctl->server.service.
     */
    i = get_element(&result, 0, "%s\t%s\tport", tmp, prot);
    if (i)
	goto done;

    xfree(ctl->server.service);
    ctl->server.service = result ? xstrdup(result) : NULL;
    pwmd_free(result);

    i = get_element(&result, !isatty(STDOUT_FILENO), "%s\tusername", tmp);
    if (i)
	goto done;

    if (ctl->remotename)
	xfree(ctl->remotename);
    if (ctl->server.esmtp_name)
	xfree(ctl->server.esmtp_name);
    ctl->remotename = result ? xstrdup(result) : NULL;
    ctl->server.esmtp_name = result ? xstrdup(result) : NULL;
    pwmd_free(result);

    i = get_element(&result, !isatty(STDOUT_FILENO), "%s\tpassword", tmp);
    if (i)
	goto done;

    xfree(ctl->password);
    ctl->password = result ? xstrdup(result) : NULL;
    pwmd_free(result);

#ifdef SSL_ENABLE
    /* It is up to the user to specify the sslmode via the command line or via
     * the rcfile rather than requiring an extra element. */
    i = get_element(&result, 0, "%s\t%s\tsslfingerprint", tmp, prot);
    if (i)
	goto done;

    xfree(ctl->sslfingerprint);
    ctl->sslfingerprint = result ? xstrdup(result) : NULL;
    pwmd_free(result);
#endif

done:
    xfree(tmp);
    xfree(prot);
    return i ? 1 : 0;
}

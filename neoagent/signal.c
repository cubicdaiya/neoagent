/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *
 *  Use and distribution licensed under the BSD license.
 *  See the COPYING file for full text.
 *
 */

#include <signal.h>

#include "defines.h"

// private functions
static void na_setup_signals_common(sigset_t *ss);

static void na_setup_signals_common(sigset_t *ss)
{
    sigemptyset(ss);
    sigaddset(ss, SIGTERM);
    sigaddset(ss, SIGINT);
    sigaddset(ss, SIGHUP);
}

void na_setup_ignore_signals (void)
{
    if (sigignore(SIGPIPE) == -1) {
        NA_DIE_WITH_ERROR(NULL, NA_ERROR_FAILED_IGNORE_SIGNAL);
    }
}

void na_setup_signals_for_master(sigset_t *ss)
{
    na_setup_signals_common(ss);
    sigaddset(ss, SIGCONT);
    sigaddset(ss, SIGWINCH);
    sigaddset(ss, SIGUSR1);
    sigprocmask(SIG_BLOCK, ss, NULL);
}

void na_setup_signals_for_worker(sigset_t *ss)
{
    na_setup_signals_common(ss);
    sigaddset(ss, SIGUSR2);
    sigprocmask(SIG_BLOCK, ss, NULL);
}

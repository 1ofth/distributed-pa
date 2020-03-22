//
// Created by edem on 13.03.20.
//

#pragma once

#include <stdio.h>
#include "pa2345.h"
#include "banking.h"

static const char *const log_pipe_opened =
        "Pipe (rd %3d, wr %3d) has OPENED\n";

extern FILE *event_log;

void log_started(local_id local_pid) {
    fprintf(event_log, log_started_fmt, get_physical_time(), local_pid, getpid(), getppid(), 1);
    fflush(event_log);
    printf(log_started_fmt, get_physical_time(), local_pid, getpid(), getppid(), 1);
}

void log_received_all_started(local_id local_pid) {
    fprintf(event_log, log_received_all_started_fmt, get_physical_time(), local_pid);
    fflush(event_log);
    printf(log_received_all_started_fmt, get_physical_time(), local_pid);
}

void log_done(local_id local_pid) {
    fprintf(event_log, log_done_fmt, get_physical_time(), local_pid, 1);
    fflush(event_log);
    printf(log_done_fmt, get_physical_time(), local_pid, 1);
}

void log_received_all_done(local_id local_pid) {
    fprintf(event_log, log_received_all_done_fmt, get_physical_time(), local_pid);
    fflush(event_log);
    printf(log_received_all_done_fmt, get_physical_time(), local_pid);
}

#include <stdio.h>
#include <unistd.h>
#include "pa2345.h"
#include "distributed.h"

FILE *event_log;

void log_started(dist_process *dp) {
    fprintf(event_log, log_started_fmt, dp->time, dp->local_pid, dp->pid, getppid(), 0);
    fflush(event_log);
    printf(log_started_fmt, dp->time, dp->local_pid, dp->pid, getppid(), 0);
}

void log_received_all_started(dist_process *dp) {
    fprintf(event_log, log_received_all_started_fmt, dp->time, dp->local_pid);
    fflush(event_log);
    printf(log_received_all_started_fmt, dp->time, dp->local_pid);
}

void log_done(dist_process *dp) {
    fprintf(event_log, log_done_fmt, dp->time, dp->local_pid, 0);
    fflush(event_log);
    printf(log_done_fmt, dp->time, dp->local_pid, 0);
}

void log_received_all_done(dist_process *dp) {
    fprintf(event_log, log_received_all_done_fmt, dp->time, dp->local_pid);
    fflush(event_log);
    printf(log_received_all_done_fmt, dp->time, dp->local_pid);
}

#include <stdio.h>
#include <unistd.h>
#include "pa2345.h"
#include "banking.h"
#include "distributed.h"

FILE *event_log;

void log_started(dist_process *dp) {
    fprintf(event_log, log_started_fmt, get_physical_time(), dp->local_pid, dp->pid, getppid(), dp->balance);
    fflush(event_log);
    printf(log_started_fmt, get_physical_time(), dp->local_pid, dp->pid, getppid(), dp->balance);
}

void log_received_all_started(dist_process *dp) {
    fprintf(event_log, log_received_all_started_fmt, get_physical_time(), dp->local_pid);
    fflush(event_log);
    printf(log_received_all_started_fmt, get_physical_time(), dp->local_pid);
}

void log_done(dist_process *dp) {
    fprintf(event_log, log_done_fmt, get_physical_time(), dp->local_pid, dp->balance);
    fflush(event_log);
    printf(log_done_fmt, get_physical_time(), dp->local_pid, dp->balance);
}

void log_received_all_done(dist_process *dp) {
    fprintf(event_log, log_received_all_done_fmt, get_physical_time(), dp->local_pid);
    fflush(event_log);
    printf(log_received_all_done_fmt, get_physical_time(), dp->local_pid);
}

void log_transfer_in(TransferOrder *to) {
    fprintf(event_log, log_transfer_in_fmt, get_physical_time(), to->s_dst, to->s_amount, to->s_src);
    fflush(event_log);
    printf(log_transfer_in_fmt, get_physical_time(), to->s_dst, to->s_amount, to->s_src);
}

void log_transfer_out(TransferOrder *to) {
    fprintf(event_log, log_transfer_out_fmt, get_physical_time(), to->s_src, to->s_amount, to->s_dst);
    fflush(event_log);
    printf(log_transfer_out_fmt, get_physical_time(), to->s_src, to->s_amount, to->s_dst);
}

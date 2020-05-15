#pragma once

static const char *const log_pipe_opened =
        "Pipe (rd %3d, wr %3d) has OPENED\n";

extern FILE *event_log;

void log_started(dist_process *dp);

void log_received_all_started(dist_process *dp);

void log_done(dist_process *dp);

void log_received_all_done(dist_process *dp);

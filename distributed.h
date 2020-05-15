#pragma once

#include <sys/types.h>
#include <stdbool.h>
#include "ipc.h"
#include "MinPQ.h"

typedef struct {
    pid_t pid;
    local_id local_pid;
    int pipe_rd[MAX_PROCESS_ID + 1];
    int pipe_wr[MAX_PROCESS_ID + 1];
    timestamp_t time;
    int done_left;
    MinPQ requests_queue;
}  dist_process;

void move_local_time(dist_process *dp, timestamp_t events_time);

timestamp_t inc_local_time(dist_process *dp);

void receive_all(dist_process *dp, local_id curr);

extern int processes_total;
extern bool mutexl;

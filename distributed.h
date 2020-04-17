#pragma once

#include <sys/types.h>
#include "ipc.h"
#include "banking.h"

typedef struct {
    pid_t pid;
    local_id local_pid;
    int *pipe_rd;
    int *pipe_wr;
    balance_t balance;
    BalanceHistory *balance_history;
    timestamp_t time;
} dist_process;

void move_local_time(dist_process *dp, timestamp_t events_time);

void receive_all(dist_process *dp, local_id curr);

extern int processes_total;

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
} dist_process;

extern int processes_total;

#pragma once

#include "ipc.h"

typedef struct {
    timestamp_t time;
    local_id local_pid;
} Entry;

typedef struct {
    Entry elements[MAX_PROCESS_ID * MAX_PROCESS_ID];
    int size;
} MinPQ;

int cmp(Entry *left, Entry *right);

void push(MinPQ *pq, Entry entry);

Entry pop(MinPQ *pq);

Entry peek(MinPQ *pq);

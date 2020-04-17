
#include "ipc.h"
#include "distributed.h"

void move_local_time(dist_process *dp, timestamp_t events_time) {
    if (events_time > dp->time) {
        dp->time = events_time;
    }
    dp->time++;
}

void receive_all(dist_process *dp, local_id curr) {
    for (local_id j = 1; j < processes_total; ++j) {
        if (j != curr) {
            Message msg;
            receive(dp, j, &msg);
            move_local_time(dp, msg.s_header.s_local_time);
        }
    }
}

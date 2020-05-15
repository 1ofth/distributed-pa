#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "distributed.h"
#include "logger.h"
#include "cs_manager.h"
#include "pa2345.h"

int run_child(dist_process dp) {
    /* handle child process */

    dp.pid = getpid();
    // 1
    inc_local_time(&dp);
    Message msg = {
            .s_header = {
                    .s_type = STARTED,
                    .s_local_time = dp.time,
                    .s_magic = MESSAGE_MAGIC
            },
    };

    sprintf(msg.s_payload, log_started_fmt, dp.time, dp.local_pid, dp.pid, getppid(), 0);
    msg.s_header.s_payload_len = (uint16_t) strlen(msg.s_payload);
    msg.s_header.s_magic = MESSAGE_MAGIC;

    log_started(&dp);
    send_multicast(&dp, &msg);

    receive_all(&dp, dp.local_pid);
    log_received_all_started(&dp);

    // 2
    int loops = dp.local_pid * 5;
    for (int i = 1; i <= loops; i++) {
        if (mutexl) {
            request_cs_l(&dp);
        }
        char s_payload[64];
        fprintf(event_log, log_loop_operation_fmt, dp.local_pid, i, loops);
        fflush(event_log);
        memset(s_payload, 0, sizeof(char) * 64);
        sprintf(s_payload, log_loop_operation_fmt, dp.local_pid, i, loops);
        print(s_payload);
        if (mutexl) {
            release_cs_l(&dp);
        }
    }

    inc_local_time(&dp);
    memset(msg.s_payload, 0, msg.s_header.s_payload_len);
    msg.s_header.s_type = DONE;
    msg.s_header.s_local_time = dp.time;
    sprintf(msg.s_payload, log_done_fmt, dp.time, dp.local_pid, 0);
    msg.s_header.s_payload_len = (uint16_t) strlen(msg.s_payload);
    msg.s_header.s_magic = MESSAGE_MAGIC;

    log_done(&dp);
    send_multicast(&dp, &msg);

    while (dp.done_left != 0) {
        memset(msg.s_payload, 0, msg.s_header.s_payload_len);
        local_id from_id = (local_id) receive_any(&dp, &msg);
        move_local_time(&dp, msg.s_header.s_local_time);
        switch (msg.s_header.s_type) {
            case CS_REQUEST: {
                memset(msg.s_payload, 0, msg.s_header.s_payload_len);
                msg.s_header.s_type = CS_REPLY;
                msg.s_header.s_local_time = inc_local_time(&dp);
                msg.s_header.s_payload_len = 0;
                msg.s_header.s_magic = MESSAGE_MAGIC;

                send(&dp, from_id, &msg);
                break;
            }
            case DONE:
                dp.done_left--;
                break;

            default:
                break;

        }
    }

    log_received_all_done(&dp);
    fclose(event_log);
    return 0;
}

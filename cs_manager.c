#include <stdio.h>
#include <string.h>
#include "distributed.h"
#include "cs_manager.h"

int request_cs_l(dist_process *dp) {
    Message msg = {
            .s_header = {
                    .s_type = CS_REQUEST,
                    .s_local_time = inc_local_time(dp),
                    .s_magic = MESSAGE_MAGIC,
                    .s_payload_len = 0
            },
    };
    Entry request = (Entry) {
            .local_pid = dp->local_pid,
            .time = dp->time
    };

    push(&dp->requests_queue, request);
    send_multicast(dp, &msg);

    int reply_left = processes_total - 2;
    Entry head = peek(&dp->requests_queue);
    while (head.local_pid != request.local_pid
           || head.time != request.time
           || reply_left != 0) {
        memset(msg.s_payload, 0, msg.s_header.s_payload_len);
        local_id from_id = (local_id) receive_any(dp, &msg);
        move_local_time(dp, msg.s_header.s_local_time);
        switch (msg.s_header.s_type) {
            case CS_REQUEST: {
                push(&dp->requests_queue, (Entry) {
                        .local_pid = from_id,
                        .time = msg.s_header.s_local_time
                });

                msg.s_header.s_type = CS_REPLY;
                msg.s_header.s_local_time = inc_local_time(dp);
                msg.s_header.s_payload_len = 0;
                msg.s_header.s_magic = MESSAGE_MAGIC;

                send(dp, from_id, &msg);
                break;
            }
            case CS_REPLY: {
                reply_left--;
                break;
            }
            case CS_RELEASE: {
                pop(&dp->requests_queue);
                break;
            }
            case DONE: {
                dp->done_left--;
                break;
            }
            default:
                break;
        }
        head = peek(&dp->requests_queue);
    }
    return 0;
}

int release_cs_l(dist_process *dp) {
    pop(&dp->requests_queue);
    Message msg = {
            .s_header = {
                    .s_type = CS_RELEASE,
                    .s_local_time = inc_local_time(dp),
                    .s_magic = MESSAGE_MAGIC,
                    .s_payload_len = 0
            },
    };
    send_multicast(dp, &msg);
    return 0;
}

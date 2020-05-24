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

    send_multicast(dp, &msg);

    int reply_left = processes_total - 2;
    while (reply_left != 0) {
        memset(msg.s_payload, 0, msg.s_header.s_payload_len);
        local_id from_id = (local_id) receive_any(dp, &msg);
        move_local_time(dp, msg.s_header.s_local_time);
        switch (msg.s_header.s_type) {
            case CS_REQUEST: {
                Entry incom = (Entry) {
                        .local_pid = from_id,
                        .time = msg.s_header.s_local_time
                };
                if (cmp(&request, &incom) < 0) {
                    push(&dp->requests_queue, incom);
                } else {
                    msg.s_header.s_type = CS_REPLY;
                    msg.s_header.s_local_time = inc_local_time(dp);
                    msg.s_header.s_payload_len = 0;
                    msg.s_header.s_magic = MESSAGE_MAGIC;

                    send(dp, from_id, &msg);
                }

                break;
            }
            case CS_REPLY: {
                reply_left--;
                break;
            }
            case DONE: {
                dp->done_left--;
                break;
            }
            default:
                break;
        }
    }
    return 0;
}

int release_cs_l(dist_process *dp) {
    while (dp->requests_queue.size != 0) {
        Entry e = pop(&dp->requests_queue);
        timestamp_t t = inc_local_time(dp);
        Message msg = {
                .s_header = {
                        .s_type = CS_REPLY,
                        .s_local_time = t,
                        .s_magic = MESSAGE_MAGIC,
                        .s_payload_len = 0
                },
        };
        send(dp, e.local_pid, &msg);
    }
    return 0;
}

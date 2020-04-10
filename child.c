#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "distributed.h"
#include "pa2345.h"
#include "logger.h"

static void receive_all(dist_process *dp, local_id curr, Message *msg) {
    for (local_id j = 1; j < processes_total; ++j) {
        memset(msg->s_payload, 0, msg->s_header.s_payload_len);
        if (j != curr) {
            receive(dp, j, msg);
        }
    }
}
static void fill_history(BalanceHistory *history, timestamp_t curr_time, balance_t new_balance) {
    balance_t prev_balance = history->s_history[history->s_history_len - 1].s_balance;
    timestamp_t prev_time = (timestamp_t) (history->s_history[history->s_history_len - 1].s_time);
    history->s_history_len += (curr_time - prev_time);
    for (timestamp_t i = (timestamp_t) (prev_time + 1); i < curr_time; ++i) {
        history->s_history[i] = (BalanceState) {
                .s_time = i,
                .s_balance = prev_balance
        };
    }
    history->s_history[curr_time] = (BalanceState) {
            .s_time = curr_time,
            .s_balance = new_balance
    };

}

int run_child(dist_process dp) {
    /* handle child process */

    dp.pid = getpid();
    dp.balance_history->s_history[0] = (BalanceState) {
            .s_time = 0,
            .s_balance = dp.balance
    };
    dp.balance_history->s_history_len = 1;
    // 1
    Message msg = {
            .s_header = {
                    .s_type = STARTED,
                    .s_local_time = get_physical_time(),
                    .s_magic = MESSAGE_MAGIC
            },
    };

    sprintf(msg.s_payload, log_started_fmt, get_physical_time(), dp.local_pid, dp.pid, getppid(),
            dp.balance);
    msg.s_header.s_payload_len = (uint16_t) strlen(msg.s_payload);

    log_started(&dp);
    send_multicast(&dp, &msg);

    receive_all(&dp, dp.local_pid, &msg);
    log_received_all_started(&dp);

    // 2
    bool is_interrupted = false;
    size_t done_received = 0;
    while (!is_interrupted || done_received != processes_total - 2) {
        memset(msg.s_payload, 0, msg.s_header.s_payload_len);
        receive_any(&dp, &msg);
        switch (msg.s_header.s_type) {
            case STOP: {
                is_interrupted = true;
                // 3
                memset(msg.s_payload, 0, msg.s_header.s_payload_len);
                msg.s_header.s_type = DONE;
                msg.s_header.s_local_time = get_physical_time();
                sprintf(msg.s_payload, log_done_fmt, get_physical_time(), dp.local_pid, dp.balance);
                msg.s_header.s_payload_len = (uint16_t) strlen(msg.s_payload);

                log_done(&dp);
                send_multicast(&dp, &msg);

                break;
            }
            case DONE:
                done_received++;
                break;
            case TRANSFER: {
                TransferOrder *to = (TransferOrder *) msg.s_payload;
                // update history
                timestamp_t tt = get_physical_time();
                if (to->s_src == dp.local_pid) {
                    dp.balance -= to->s_amount;
                    fill_history(dp.balance_history, tt, dp.balance);
                    send(&dp, to->s_dst, &msg);
                    log_transfer_out(to);
                } else if (to->s_dst == dp.local_pid) {
                    dp.balance += to->s_amount;
                    log_transfer_in(to);
                    fill_history(dp.balance_history, tt, dp.balance);

                    msg.s_header.s_local_time = tt;
                    msg.s_header.s_type = ACK;
                    memset(msg.s_payload, 0, msg.s_header.s_payload_len);
                    msg.s_header.s_payload_len = 0;
                    send(&dp, PARENT_ID, &msg);
                }
                break;
            }

            default:
                break;

        }
    }

    log_received_all_done(&dp);
    fill_history(dp.balance_history, get_physical_time(), dp.balance);

    memset(msg.s_payload, 0, msg.s_header.s_payload_len);
    uint16_t history_length = sizeof(local_id) + sizeof(uint8_t) +
                             dp.balance_history->s_history_len * sizeof(BalanceState);
    msg.s_header.s_type = BALANCE_HISTORY;
    msg.s_header.s_local_time = get_physical_time();
    msg.s_header.s_payload_len = history_length;

    memcpy(msg.s_payload, dp.balance_history, history_length);
    send(&dp, PARENT_ID, &msg);

    fclose(event_log);
    return 0;
}

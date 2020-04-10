#include <stdio.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <memory.h>
#include <errno.h>
#include <fcntl.h>

#include "ipc.h"
#include "child.h"
#include "common.h"
#include "distributed.h"
#include "logger.h"
#include "banking.h"


int processes_total;
FILE *event_log;

static void close_pipes(dist_process dp[], local_id current) {
    for (int i = 0; i < processes_total; ++i) {
        for (int j = 0; j < processes_total; ++j) {
            if (i != j && i != current) {
                close(dp[i].pipe_rd[j]);
                close(dp[i].pipe_wr[j]);
            }
        }
    }
}

static void receive_all(dist_process *dp, local_id curr, Message *msg) {
    memset(msg->s_payload, 0, msg->s_header.s_payload_len);
    for (local_id j = 1; j < processes_total; ++j) {
        if (j != curr) {
            receive(dp, j, msg);
        }
    }
}

static void receive_all_balance_histories(dist_process *parent, AllHistory *all_history) {
    all_history->s_history_len = (uint8_t) (processes_total - 1);
    for (local_id i = 1; i < processes_total; i++) {
        Message msg;
        receive(parent, i, &msg);
        if (msg.s_header.s_type == BALANCE_HISTORY) {
            BalanceHistory *their_history = (BalanceHistory *) &msg.s_payload;
            all_history->s_history[i - 1] = *their_history;
        }
    }
}

void transfer(void *parent_data, local_id src, local_id dst,
              balance_t amount) {
    dist_process *s = parent_data;

    Message msg = {
            .s_header = {
                    .s_type = TRANSFER,
                    .s_local_time = get_physical_time(),
                    .s_magic = MESSAGE_MAGIC
            },
    };

    TransferOrder to = {
            .s_src = src,
            .s_dst = dst,
            .s_amount = amount
    };

    memcpy(msg.s_payload, &to, sizeof(TransferOrder));
    msg.s_header.s_payload_len = sizeof(TransferOrder);
    send(s, src, &msg);

    memset(msg.s_payload, 0, msg.s_header.s_payload_len);
    receive(s, dst, &msg); //ACK
}

int main(int argc, char *argv[]) {
    int opt;

    while ((opt = getopt(argc, argv, "p:")) != -1) {
        switch (opt) {
            case 'p':
                processes_total = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s [-p] [number of processes] amount...\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

//    if (processes_total < 1 || processes_total > 10) {
//        fprintf(stderr, "Number of processes should be between 1 and 10\n");
//        exit(EXIT_FAILURE);
//    }

    processes_total++;

    dist_process dp[processes_total];

    for (int i = 0; i < processes_total; i++) {
        dist_process process = (dist_process) {
                .pipe_rd = malloc(processes_total * sizeof(int)),
                .pipe_wr = malloc(processes_total * sizeof(int)),
                .balance_history = malloc(sizeof(BalanceHistory))
        };
        dp[i] = process;
    }

    int pipefd[2];
    FILE *pipe_log = fopen(pipes_log, "w");
    for (int i = 0; i < processes_total; i++) {
        for (int j = 0; j < processes_total; j++) {
            if (i == j) {
                dp[i].pipe_wr[j] = -1;
                dp[j].pipe_rd[i] = -1;
                continue;
            }
            pipe(pipefd);

            fcntl(pipefd[0], F_SETFL, fcntl(pipefd[0], F_GETFL) | O_NONBLOCK);
            fcntl(pipefd[1], F_SETFL, fcntl(pipefd[1], F_GETFL) | O_NONBLOCK);

            fprintf(pipe_log, log_pipe_opened, pipefd[0], pipefd[1]);
            dp[i].pipe_wr[j] = pipefd[1];
            dp[j].pipe_rd[i] = pipefd[0];
        }
    }
    fclose(pipe_log);

    dp[0].local_pid = PARENT_ID;
    dp[0].pid = getpid();

    event_log = fopen(events_log, "w");
    for (local_id i = 1; i < processes_total; i++) {
        dp[i].local_pid = i;
        dp[i].balance_history->s_id = i;
        dp[i].balance = (balance_t) atoi(argv[i + 2]);

        if (fork() == 0) {
            /* handle child process */
            close_pipes(dp, i);
            return run_child(dp[i]);
        }
    }

    close_pipes(dp, PARENT_ID);

    Message msg;
    receive_all(&dp[PARENT_ID], PARENT_ID, &msg);
    log_received_all_started(&dp[PARENT_ID]);

    bank_robbery(&dp[PARENT_ID], (local_id) (processes_total - 1));
    msg.s_header.s_type = STOP;
    msg.s_header.s_local_time = get_physical_time();
    msg.s_header.s_payload_len = 0;

    send_multicast(&dp[PARENT_ID], &msg);

    receive_all(&dp[PARENT_ID], PARENT_ID, &msg); // DONE
    log_received_all_done(&dp[PARENT_ID]);

    AllHistory *all_history = malloc(sizeof(AllHistory));
    receive_all_balance_histories(&dp[PARENT_ID], all_history);
    print_history(all_history);

    fclose(event_log);
    for (local_id j = 1; j < processes_total; ++j) {
        wait(NULL);
    }
    return 0;
}

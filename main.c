#include <stdio.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <memory.h>
#include <errno.h>

#include "ipc.h"
#include "common.h"
#include "distributed.h"
#include "logger.h"

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

int main(int argc, char *argv[]) {
    int opt;

    while ((opt = getopt(argc, argv, "p:")) != -1) {
        switch (opt) {
            case 'p':
                processes_total = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s [-p] [number of processes]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (processes_total < 1 || processes_total > 10) {
        fprintf(stderr, "Number of processes should be between 1 and 10\n");
        exit(EXIT_FAILURE);
    }

    processes_total++;

    dist_process dp[processes_total];

    for (int i = 0; i < processes_total; i++) {
        dist_process process = {
                .pipe_rd = malloc(processes_total * sizeof(process.pipe_rd)),
                .pipe_wr = malloc(processes_total * sizeof(process.pipe_wr)),
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
        if (fork() == 0) {
            /* handle child process */

            dp[i].pid = getpid();
            close_pipes(dp, dp[i].local_pid);

            // 1
            Message msg = {
                    .s_header = {
                            .s_type = STARTED,
                            .s_magic = MESSAGE_MAGIC
                    },
            };

            sprintf(msg.s_payload, log_started_fmt, dp[i].local_pid, dp[i].pid, getppid());
            msg.s_header.s_payload_len = (uint16_t) strlen(msg.s_payload);

            log_started(dp[i].local_pid);
            send_multicast(&dp[i], &msg);

            receive_all(&dp[i], i, &msg);
            log_received_all_started(dp[i].local_pid);

            // 2

            // 3
            memset(msg.s_payload, 0, msg.s_header.s_payload_len);
            msg.s_header.s_type = DONE;
            sprintf(msg.s_payload, log_done_fmt, dp[i].local_pid);
            msg.s_header.s_payload_len = (uint16_t) strlen(msg.s_payload);

            log_done(dp[i].local_pid);
            send_multicast(&dp[i], &msg);

            receive_all(&dp[i], i, &msg);
            log_received_all_done(dp[i].local_pid);

            fclose(event_log);
            return 0;
        }
    }

    close_pipes(dp, PARENT_ID);

    Message res_msg;
    receive_all(&dp[PARENT_ID], PARENT_ID, &res_msg);
    log_received_all_started(PARENT_ID);

    receive_all(&dp[PARENT_ID], PARENT_ID, &res_msg);
    log_received_all_done(PARENT_ID);

    fclose(event_log);
    for (local_id j = 1; j < processes_total; ++j) {
        wait(NULL);
    }
    return 0;
}

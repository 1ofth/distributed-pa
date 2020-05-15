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


int processes_total;
bool mutexl;
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

int main(int argc, char *argv[]) {
    int opt;
    static struct option long_options[] =
            {
                    {"mutexl", no_argument, NULL, 'm'},
                    {NULL, 0,               NULL, 0}
            };

    while ((opt = getopt_long(argc, argv, "p:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'm':
                mutexl = true;
                break;
            case 'p':
                processes_total = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s -p number of processes [--mutexl]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    processes_total++;

    dist_process dp[processes_total];

    for (int i = 0; i < processes_total; i++) {
        dist_process process = (dist_process) {
                .time = 0,
                .done_left = processes_total - 2
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

    dp[PARENT_ID].local_pid = PARENT_ID;
    dp[PARENT_ID].pid = getpid();

    event_log = fopen(events_log, "w");
    for (local_id i = 1; i < processes_total; i++) {
        dp[i].local_pid = i;
        dp[i].requests_queue = (MinPQ) {
                .size = 0
        };

        if (fork() == 0) {
            /* handle child process */
            close_pipes(dp, i);
            return run_child(dp[i]);
        }
    }

    close_pipes(dp, PARENT_ID);

    receive_all(&dp[PARENT_ID], PARENT_ID);
    log_received_all_started(&dp[PARENT_ID]);

    dp[PARENT_ID].done_left = processes_total - 1;
    Message msg;
    while (dp[PARENT_ID].done_left != 0) {
        receive_any(&dp[PARENT_ID], &msg);
        move_local_time(&dp[PARENT_ID], msg.s_header.s_local_time);
        switch (msg.s_header.s_type) {
            case DONE:
                dp[PARENT_ID].done_left--;
                break;
            default:
                break;

        }
    }
    log_received_all_done(&dp[PARENT_ID]);

    fclose(event_log);
    for (local_id j = 1; j < processes_total; ++j) {
        wait(NULL);
    }
    return 0;
}

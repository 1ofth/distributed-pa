#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include "distributed.h"

int send(void *self, local_id dst, const Message *msg) {
    dist_process *s = self;

    if ((write(s->pipe_wr[dst], msg, sizeof(MessageHeader) + msg->s_header.s_payload_len)) < 0) {
        perror("write");
        return errno;
    }
    return 0;
}

int send_multicast(void *self, const Message *msg) {
    dist_process *p = self;
    for (local_id i = 0; i < processes_total; ++i) {
        if (i != p->local_pid) {
            int res = send(self, i, msg);
            if (res > 0) {
                return res;
            }
        }
    }
    return 0;
}

int receive(void *self, local_id from, Message *msg) {
    dist_process *s = self;

    while (1) {
        if (read(s->pipe_rd[from], &msg->s_header, sizeof(MessageHeader)) > 0) {
            while (1) {
                if (read(s->pipe_rd[from], &msg->s_payload, msg->s_header.s_payload_len) >= 0) {
                    return 0;
                }
            }
        }
    }
}


int receive_any(void * self, Message * msg) {
    dist_process *s = self;

    while (1) {
        for (int i = 0; i < processes_total; ++i) {
            if (i == s->local_pid)
                continue;
            if (read(s->pipe_rd[i], &msg->s_header, sizeof(MessageHeader)) > 0) {
                if (read(s->pipe_rd[i], &msg->s_payload, msg->s_header.s_payload_len) >= 0) {
                    return 0;
                }
            }
        }
    }
}


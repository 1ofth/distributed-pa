// Min priority queue based on sorted array

#include <stdio.h>
#include "MinPQ.h"

static int cmp(Entry *left, Entry *right) {
    if (left->time > right->time)
        return 1;
    else if (left->time < right->time)
        return -1;
    else {
        if (left->local_pid > right->local_pid)
            return 1;
        else if (left->local_pid < right->local_pid)
            return -1;
        else
            return 0;
    }

}

void push(MinPQ *pq, Entry entry) {

    int l = 0, r = pq->size - 1, mid;
    while (l <= r) {
        mid = l + (r - l) / 2;

        if (cmp(&pq->elements[mid], &entry) > 0) {
            r = mid - 1;
        } else if (cmp(&pq->elements[mid], &entry) < 0) {
            l = mid + 1;
        } else {

        }
    }
    for (int i = pq->size; i > l; i--) {
        pq->elements[i] = pq->elements[i - 1];
    }
    pq->size++;
    pq->elements[l] = entry;

}

Entry pop(MinPQ *pq) {
    Entry ans = pq->elements[0];
    for (int i = 1; i < pq->size; ++i) {
        pq->elements[i - 1] = pq->elements[i];
    }
    pq->size--;
    return ans;
}

Entry peek(MinPQ *pq) {
    return pq->elements[0];
}

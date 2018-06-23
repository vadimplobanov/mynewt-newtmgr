#ifndef __RENDEZVOUS_H__
#define __RENDEZVOUS_H__

#include <pthread.h>
#include "ddsmgr.h"

struct rendezvous {
    pthread_mutex_t mutex;
    pthread_cond_t condvar;
    int enabled;
    const void *srcwait;
    void *dstwait;
    void (*convert)(void *dst, const void *src);
};

void rendezvous_initialize(struct rendezvous *rendezvous,
                           void (*convert)(void *dst, const void *src));
int rendezvous_consume(struct rendezvous *rendezvous,
                       void *dstdata,
                       const struct abs_timeout *abstimo);
int rendezvous_produce(struct rendezvous *rendezvous,
                       const void *srcdata);
void rendezvous_listen(struct rendezvous *rendezvous);
void rendezvous_reject(struct rendezvous *rendezvous);

#endif

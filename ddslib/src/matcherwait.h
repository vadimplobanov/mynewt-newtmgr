#ifndef __MATCHERWAIT_H__
#define __MATCHERWAIT_H__

#include <pthread.h>
#include "ddsmgr.h"

enum matchertype {
    MATCHERTYPE_PING = 0,
    MATCHERTYPE_PONG = 1,
    MATCHERTYPE_MAX  = 2,
};

struct matcherwait {
    pthread_mutex_t mutex;
    pthread_cond_t condvar;
    unsigned int remain;
};

void matcherwait_initialize(struct matcherwait *matcherwait);
int matcherwait_wait(struct matcherwait *matcherwait,
                     const struct abs_timeout *abstimo);
void matcherwait_wake(struct matcherwait *matcherwait,
                      enum matchertype waketype);

#endif

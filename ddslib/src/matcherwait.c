#include "matcherwait.h"

void matcherwait_initialize(struct matcherwait *matcherwait)
{
    pthread_mutex_init(&matcherwait->mutex, NULL);
    pthread_cond_init(&matcherwait->condvar, NULL);
    matcherwait->remain = (1 << MATCHERTYPE_MAX) - 1;
}

int matcherwait_wait(struct matcherwait *matcherwait,
                     const struct abs_timeout *abstimo)
{
    struct timespec timeout;
    int result;

    timeout.tv_sec = abstimo->seconds;
    timeout.tv_nsec = abstimo->nseconds;
    result = 0;

    pthread_mutex_lock(&matcherwait->mutex);
    while (matcherwait->remain)
    {
        if (pthread_cond_timedwait(&matcherwait->condvar,
                                   &matcherwait->mutex,
                                   &timeout) != 0)
        {
            result = 1;
            break;
        }
    }
    pthread_mutex_unlock(&matcherwait->mutex);

    return result;
}

void matcherwait_wake(struct matcherwait *matcherwait,
                      enum matchertype waketype)
{
    unsigned int clmask, remain;

    clmask = ~(1 << waketype);

    pthread_mutex_lock(&matcherwait->mutex);
    matcherwait->remain &= clmask;
    remain = matcherwait->remain;
    pthread_mutex_unlock(&matcherwait->mutex);

    if (!remain)
    {
        pthread_cond_signal(&matcherwait->condvar);
    }
}

#include "rendezvous.h"

void rendezvous_initialize(struct rendezvous *rendezvous,
                           void (*convert)(void *dst, const void *src))
{
    pthread_mutex_init(&rendezvous->mutex, NULL);
    pthread_cond_init(&rendezvous->condvar, NULL);
    rendezvous->enabled = 0;
    rendezvous->srcwait = NULL;
    rendezvous->dstwait = NULL;
    rendezvous->convert = convert;
}

int rendezvous_consume(struct rendezvous *rendezvous,
                       void *dstdata,
                       const struct abs_timeout *abstimo)
{
    struct timespec timeout;
    int result, wakeup;

    timeout.tv_sec = abstimo->seconds;
    timeout.tv_nsec = abstimo->nseconds;
    result = 0;
    wakeup = 0;

    pthread_mutex_lock(&rendezvous->mutex);
    if (!rendezvous->enabled || rendezvous->dstwait)
    {
        result = 1;
        goto unlock;
    }
    if (!rendezvous->srcwait)
    {
        rendezvous->dstwait = dstdata;
        do
        {
            if (pthread_cond_timedwait(&rendezvous->condvar,
                                       &rendezvous->mutex,
                                       &timeout) != 0)
            {
                rendezvous->dstwait = NULL;
                result = 1;
                goto unlock;
            }
        }
        while (rendezvous->dstwait);
    }
    else
    {
        rendezvous->convert(dstdata,
                            rendezvous->srcwait);
        rendezvous->srcwait = NULL;
        wakeup = 1;
    }
unlock:
    pthread_mutex_unlock(&rendezvous->mutex);

    if (wakeup)
    {
        pthread_cond_signal(&rendezvous->condvar);
    }

    return result;
}

int rendezvous_produce(struct rendezvous *rendezvous,
                       const void *srcdata)
{
    int result, wakeup;

    result = 0;
    wakeup = 0;

    pthread_mutex_lock(&rendezvous->mutex);
    if (!rendezvous->enabled || rendezvous->srcwait)
    {
        result = 1;
        goto unlock;
    }
    if (!rendezvous->dstwait)
    {
        rendezvous->srcwait = srcdata;
        do
        {
            if (pthread_cond_wait(&rendezvous->condvar,
                                  &rendezvous->mutex) != 0)
            {
                rendezvous->srcwait = NULL;
                result = 1;
                goto unlock;
            }
        } while (rendezvous->srcwait);
    }
    else
    {
        rendezvous->convert(rendezvous->dstwait,
                            srcdata);
        rendezvous->dstwait = NULL;
        wakeup = 1;
    }
unlock:
    pthread_mutex_unlock(&rendezvous->mutex);

    if (wakeup)
    {
        pthread_cond_signal(&rendezvous->condvar);
    }

    return result;
}

void rendezvous_listen(struct rendezvous *rendezvous)
{
    pthread_mutex_lock(&rendezvous->mutex);
    rendezvous->enabled = 1;
    pthread_mutex_unlock(&rendezvous->mutex);
}

void rendezvous_reject(struct rendezvous *rendezvous)
{
    int wakeup;

    wakeup = 0;
    pthread_mutex_lock(&rendezvous->mutex);
    if (rendezvous->srcwait)
    {
        rendezvous->srcwait = NULL;
        wakeup = 1;
    }
    if (rendezvous->dstwait)
    {
        rendezvous->dstwait = NULL;
        wakeup = 1;
    }
    rendezvous->enabled = 0;
    pthread_mutex_unlock(&rendezvous->mutex);

    if (wakeup)
    {
        pthread_cond_signal(&rendezvous->condvar);
    }
}

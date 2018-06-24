#ifndef __DDSMGR_H__
#define __DDSMGR_H__

struct abs_timeout {
    unsigned long seconds;
    long nseconds;
};

struct packet_ping {
    int request_id;
};

struct packet_pong {
    int request_id;
    char device_name[16];
};

int ddsmgr_initialize(const struct abs_timeout *timo);

void ddsmgr_ping_send(const struct packet_ping *ping);

int ddsmgr_pong_recv(const struct abs_timeout *timo,
                     struct packet_pong *pong);
void ddsmgr_pong_listen(void);
void ddsmgr_pong_reject(void);

#endif

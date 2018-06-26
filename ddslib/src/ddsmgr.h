#ifndef __DDSMGR_H__
#define __DDSMGR_H__

struct abs_timeout {
    unsigned long seconds;
    long nseconds;
};

struct packet_mcmd {
    int request_id;
    const char *device_name;
    const char *cmd_data;
    int cmd_size;
};

struct packet_mrsp {
    int request_id;
    int rsp_size;
    char rsp_data[1024];
};

struct packet_ping {
    int request_id;
};

struct packet_pong {
    int request_id;
    char device_name[16];
};

int ddsmgr_initialize(const struct abs_timeout *timo);

void ddsmgr_mcmd_send(const struct packet_mcmd *mcmd);

int ddsmgr_mrsp_recv(const struct abs_timeout *timo,
                     struct packet_mrsp *mrsp);
void ddsmgr_mrsp_listen(void);
void ddsmgr_mrsp_reject(void);

void ddsmgr_ping_send(const struct packet_ping *ping);

int ddsmgr_pong_recv(const struct abs_timeout *timo,
                     struct packet_pong *pong);
void ddsmgr_pong_listen(void);
void ddsmgr_pong_reject(void);

#endif

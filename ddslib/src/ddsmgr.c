#include <netinet/in.h>
#include <net/if.h>
#include <assert.h>
#include <ifaddrs.h>
#include <stdarg.h>

#include "../lib2/dds.h"
#include "ddsmgr.h"
#include "matcherwait.h"
#include "rendezvous.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

struct ip4ifdesc {
    uint32_t address;
    uint32_t netmask;
};

static int valid_ipv4_ifaddr(const struct ifaddrs *ifaddr)
{
    return ifaddr->ifa_name != NULL &&
           ifaddr->ifa_addr != NULL &&
           ifaddr->ifa_netmask != NULL &&
           ifaddr->ifa_addr->sa_family == AF_INET &&
           ifaddr->ifa_flags & IFF_UP;
}

static void copy_ipv4_intf(const struct ifaddrs *ifaddr,
                           struct ip4ifdesc *ifdesc)
{
    uint32_t address, netmask;

    address = ((const struct sockaddr_in *)ifaddr->ifa_addr)->sin_addr.s_addr;
    netmask = ((const struct sockaddr_in *)ifaddr->ifa_netmask)->sin_addr.s_addr;
    address = ntohl(address);
    netmask = ntohl(netmask);
    ifdesc->address = address;
    ifdesc->netmask = netmask;
}

static const char *external_intf_prefixes[] =
{
    "en",
    "wlo",
    "",
};

static void find_ipv4_intfs(struct ip4ifdesc *ifdesc)
{
    struct ifaddrs *ifaddrs, *ifaddr;
    uint32_t i;

    getifaddrs(&ifaddrs);

    ifdesc->address = 0;
    ifdesc->netmask = 0;

    for (i = 0; i < sizeof(external_intf_prefixes) /
                    sizeof(*external_intf_prefixes); i++)
    {
        const char *prefixptr;
        size_t prefixlen;

        prefixptr = external_intf_prefixes[i];
        prefixlen = strlen(prefixptr);

        for (ifaddr = ifaddrs; ifaddr != NULL; ifaddr = ifaddr->ifa_next)
        {
            if (valid_ipv4_ifaddr(ifaddr) &&
                strncmp(ifaddr->ifa_name, prefixptr, prefixlen) == 0)
            {
                copy_ipv4_intf(ifaddr, ifdesc);
                goto done;
            }
        }
    }

done:

    freeifaddrs(ifaddrs);
}

static struct matcherwait init_matchwait;
static struct rendezvous mrsp_rendezvous;
static struct rendezvous pong_rendezvous;

static void convert_packet_mrsp(void *dst, void *src)
{
    struct packet_mrsp *dstmrsp;
    PacketMRsp *srcmrsp;

    dstmrsp = dst;
    srcmrsp = src;

    dstmrsp->request_id = srcmrsp->request_id;
    dstmrsp->rsp_size = MIN(DDS_CharSeq_get_length(&srcmrsp->rsp_data),
                            sizeof(dstmrsp->rsp_data));
    DDS_CharSeq_to_array(&srcmrsp->rsp_data,
                         (DDS_Char *)dstmrsp->rsp_data,
                         dstmrsp->rsp_size);
}

static void convert_packet_pong(void *dst, void *src)
{
    struct packet_pong *dstpong;
    PacketPong *srcpong;

    dstpong = dst;
    srcpong = src;

    dstpong->request_id = srcpong->request_id;
    memcpy(dstpong->device_name, srcpong->device_name,
           sizeof(dstpong->device_name));
}

static int print_error(const char *fmt, ...)
{
    va_list arg;
    int res;

    va_start(arg, fmt);
    res = vfprintf(stderr, fmt, arg);
    va_end(arg);

    return res;
}

static void mrsp_received(PacketMRsp *mrsp)
{
    rendezvous_produce(&mrsp_rendezvous, mrsp);
}

static void mrsp_submatched(void)
{
    matcherwait_wake(&init_matchwait, MATCHERTYPE_MRSP);
}

static void mcmd_pubmatched(void)
{
    matcherwait_wake(&init_matchwait, MATCHERTYPE_MCMD);
}

static void pong_received(PacketPong *pong)
{
    rendezvous_produce(&pong_rendezvous, pong);
}

static void pong_submatched(void)
{
    matcherwait_wake(&init_matchwait, MATCHERTYPE_PONG);
}

static void ping_pubmatched(void)
{
    matcherwait_wake(&init_matchwait, MATCHERTYPE_PING);
}

int ddsmgr_initialize(const struct abs_timeout *timo)
{
    struct ip4ifdesc ifdesc;

    find_ipv4_intfs(&ifdesc);

    matcherwait_initialize(&init_matchwait);
    rendezvous_initialize(&mrsp_rendezvous, convert_packet_mrsp);
    rendezvous_initialize(&pong_rendezvous, convert_packet_pong);

    if (dds_create(print_error, ifdesc.address, ifdesc.netmask))
    {
        fprintf(stderr, "dds_create failed\n");
        return 1;
    }

    if (dds_mrsp_subscriber(mrsp_submatched, mrsp_received))
    {
        fprintf(stderr, "dds_mrsp_subscriber failed\n");
    }

    if (dds_pong_subscriber(pong_submatched, pong_received))
    {
        fprintf(stderr, "dds_pong_subscriber failed\n");
        return 1;
    }

    if (dds_mcmd_publisher(mcmd_pubmatched))
    {
        fprintf(stderr, "dds_mcmd_publisher failed\n");
        return 1;
    }

    if (dds_ping_publisher(ping_pubmatched))
    {
        fprintf(stderr, "dds_ping_publisher failed\n");
        return 1;
    }

    if (dds_enable())
    {
        fprintf(stderr, "dds_enable failed\n");
        return 1;
    }

    if (matcherwait_wait(&init_matchwait, timo))
    {
        fprintf(stderr, "failed to match dds client\n");
        return 1;
    }

    return 0;
}

void ddsmgr_mcmd_send(const struct packet_mcmd *mcmd)
{
    dds_mcmd_publish(mcmd->request_id,
                     mcmd->device_name,
                     mcmd->cmd_data,
                     mcmd->cmd_size);
}

int ddsmgr_mrsp_recv(const struct abs_timeout *timo,
                     struct packet_mrsp *mrsp)
{
    return rendezvous_consume(&mrsp_rendezvous, mrsp, timo);
}

void ddsmgr_mrsp_listen(void)
{
    rendezvous_listen(&mrsp_rendezvous);
}

void ddsmgr_mrsp_reject(void)
{
    rendezvous_reject(&mrsp_rendezvous);
}

void ddsmgr_ping_send(const struct packet_ping *ping)
{
    dds_ping_publish(ping->request_id);
}

int ddsmgr_pong_recv(const struct abs_timeout *timo,
                     struct packet_pong *pong)
{
    return rendezvous_consume(&pong_rendezvous, pong, timo);
}

void ddsmgr_pong_listen(void)
{
    rendezvous_listen(&pong_rendezvous);
}

void ddsmgr_pong_reject(void)
{
    rendezvous_reject(&pong_rendezvous);
}

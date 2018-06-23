#include <netinet/in.h>
#include <assert.h>
#include <ifaddrs.h>
#include <stdarg.h>

#include "../lib2/dds.h"
#include "ddsmgr.h"
#include "rendezvous.h"

static int valid_ipv4_ifaddr(const struct ifaddrs *ifaddr)
{
    return ifaddr->ifa_name != NULL &&
           ifaddr->ifa_addr != NULL &&
           ifaddr->ifa_netmask != NULL &&
           ifaddr->ifa_addr->sa_family == AF_INET &&
           ifaddr->ifa_flags & IFF_UP;
}

static void copy_ipv4_intf(const struct ifaddrs *ifaddr,
                           struct ipv4_intf *intf)
{
    uint32_t address, netmask;

    address = ((const struct sockaddr_in *)ifaddr->ifa_addr)->sin_addr.s_addr;
    netmask = ((const struct sockaddr_in *)ifaddr->ifa_netmask)->sin_addr.s_addr;
    address = ntohl(address);
    netmask = ntohl(netmask);

    strncpy(intf->ifname, ifaddr->ifa_name, sizeof(intf->ifname) - 1);
    intf->ifname[sizeof(intf->ifname) - 1] = '\0';
    intf->address = address;
    intf->netmask = netmask;
}

static const char *external_intf_prefixes[] =
{
    "en",
    "vir",
    "",
};

static void find_ipv4_intfs(struct ipv4_intfs *intfs)
{
    struct ifaddrs *ifaddrs, *ifaddr;
    uint32_t i;

    getifaddrs(&ifaddrs);

    intfs->nintfs = 0;

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
                copy_ipv4_intf(ifaddr, intfs->intfs + intfs->nintfs);
                intfs->nintfs++;
                i = sizeof(external_intf_prefixes) /
                    sizeof(*external_intf_prefixes);
                break;
            }
        }
    }

    for (ifaddr = ifaddrs; ifaddr != NULL; ifaddr = ifaddr->ifa_next)
    {
        if (valid_ipv4_ifaddr(ifaddr) &&
            strcmp(ifaddr->ifa_name, "lo") == 0)
        {
            copy_ipv4_intf(ifaddr, intfs->intfs + intfs->nintfs);
            intfs->nintfs++;
            break;
        }
    }

    freeifaddrs(ifaddrs);

    assert(intfs->nintfs != 0);
}

static struct rendezvous pong_rendezvous;

static void convert_packet_pong(void *dst,
                                const void *src)
{
    struct packet_pong *dstpong;
    const PacketPong *srcpong;

    dstpong = dst;
    srcpong = src;

    dstpong->request_id = srcpong->request_id;
    memcpy(dstpong->device_name, srcpong->device_name,
           sizeof(dstpong->device_name));
}

static void print_error(const char *fmt, ...)
{
    va_list arg;

    va_start(arg, fmt);
    vfprintf(stderr, fmt, arg);
    va_end(arg);
}

static void pong_received(const PacketPong *pong)
{
    rendezvous_produce(&pong_rendezvous, pong);
}

int ddsmgr_initialize(void)
{
    struct ipv4_intfs intfs;

    find_ipv4_intfs(&intfs);

    rendezvous_initialize(&pong_rendezvous, convert_packet_pong);

    if (dds_create(print_error, &intfs))
    {
        fprintf(stderr, "dds_create failed\n");
        return 1;
    }

    if (dds_pong_subscriber(pong_received))
    {
        fprintf(stderr, "dds_pong_subscriber failed\n");
        return 1;
    }

    if (dds_ping_publisher())
    {
        fprintf(stderr, "dds_ping_publisher failed\n");
        return 1;
    }

    if (dds_enable())
    {
        fprintf(stderr, "dds_enable failed\n");
        return 1;
    }

    return 0;
}

void ddsmgr_ping_send(const struct packet_ping *ping)
{
    dds_ping_publish(ping->request_id);
}

int ddsmgr_pong_recv(struct packet_pong *pong,
                     const struct abs_timeout *timo)
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

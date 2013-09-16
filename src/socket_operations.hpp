#pragma once

#include <string>
#include <system_error>
#include <cstring>
#include <signal.h>
#include <fcntl.h>
#include <linux/if_ether.h>
#include <linux/filter.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>

struct socket_operations
{
    static int interface_index(int sock, std::string &if_name)
    {
        struct ifreq ifr;

        strncpy(ifr.ifr_name, if_name.c_str(), IFNAMSIZ);

        if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0)
            throw std::system_error(errno, std::system_category(),
                                    "unable to read interface index");

        return ifr.ifr_ifindex;
    }

    static void interface_address(int sock, std::string if_name, uint8_t *out)
    {
        struct ifreq ifr;

        strncpy(ifr.ifr_name, if_name.c_str(), IFNAMSIZ);

        if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0)
            throw std::system_error(errno, std::system_category(),
                                    "unable to read interface address");

        memcpy(out, ifr.ifr_hwaddr.sa_data, ETH_ALEN);
    }

    static void interface_address(int sock, std::string if_name, uint32_t *out)
    {
        struct ifreq ifr;

        ifr.ifr_addr.sa_family = AF_INET;
        strncpy(ifr.ifr_name, if_name.c_str(), IFNAMSIZ);

        if (ioctl(sock, SIOCGIFADDR, &ifr) < 0)
            throw std::system_error(errno, std::system_category(),
                                    "unable to read interface address");

        *out = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;
        *out = htonl(*out);
    }

    static void interface_promisc_on(int sock, std::string &if_name)
    {
        struct ifreq ifr;

        strncpy(ifr.ifr_name, if_name.c_str(), IFNAMSIZ);

        if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0)
            throw std::system_error(errno, std::system_category(),
                                    "unable to read interface flags");

        ifr.ifr_flags |= IFF_PROMISC;

        if (ioctl(sock, SIOCSIFFLAGS, &ifr) < 0)
            throw std::system_error(errno, std::system_category(),
                                    "unable to set interface promisc");
    }

    static void interface_promisc_off(int sock, std::string &if_name)
    {
        struct ifreq ifr;

        strncpy(ifr.ifr_name, if_name.c_str(), IFNAMSIZ);

        if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0)
            throw std::system_error(errno, std::system_category(),
                                    "unable to read interface flags");

        ifr.ifr_flags &= ~IFF_PROMISC;

        if (ioctl(sock, SIOCSIFFLAGS, &ifr) < 0)
            throw std::system_error(errno, std::system_category(),
                                    "unabel to unset interface promisc");
    }

    static void set_non_blocking(int sock)
    {
        int flags = fcntl(sock, F_GETFL, 0);

        if (flags < 0)
            throw std::system_error(errno, std::system_category(),
                                    "unable to receive socket flags");

        flags |= O_NONBLOCK;

        if (fcntl(sock, F_SETFL, flags) < 0)
            throw std::system_error(errno, std::system_category(),
                                    "unable to set socket flags");
    }

    static void socket_filter_outbound(int sock)
    {
        /* filter generated with:
         * # tcpdump -dd inbound
         */
        struct sock_filter bpf[] = {
            { 0x28, 0, 0, 0xfffff004 },
            { 0x15, 0, 1, 0x00000004 },
            { 0x6, 0, 0, 0x00000000 },
            { 0x6, 0, 0, 0x0000ffff },
        };

        struct sock_fprog filter = {
            .len = sizeof(bpf)/sizeof(struct sock_filter),
            .filter = bpf,
        };

        if (setsockopt(sock, SOL_SOCKET, SO_ATTACH_FILTER, &filter,
                       sizeof(filter)) < 0)
            throw std::system_error(errno,std::system_category(),
                                    "unable to attach packet filter");
    }

    static void install_signals(void (*handler)(int))
    {
        struct sigaction act;

        memset(&act, 0, sizeof(act));

        act.sa_handler = handler;

        if (sigaction(SIGTERM, &act, 0))
            throw std::system_error(errno, std::system_category(),
                                    "unable to install signal");

        if (sigaction(SIGINT, &act, 0))
            throw std::system_error(errno, std::system_category(),
                                    "unable to install signal");
    }
};

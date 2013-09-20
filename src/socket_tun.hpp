#pragma once

#include <string>
#include <fcntl.h>
#include <string.h>
#include <system_error>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <linux/if_ether.h>

#include "arguments.hpp"

template<class super>
class socket_tun : public super
{
    typedef typename super::handle_cb handle_cb;

    int create(std::string if_name)
    {
        struct ifreq ifr = {0};
        int fd;

        std::cout << if_name << std::endl;

        if ((fd = open("/dev/net/tun", O_RDWR)) < 0)
            throw std::system_error(errno, std::system_category(),
                                    "unable to open /dev/net/tun");

        ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
        strncpy(ifr.ifr_name, if_name.c_str(), IFNAMSIZ);

        if (ioctl(fd, TUNSETIFF, &ifr) < 0)
            throw std::system_error(errno, std::system_category(),
                                    "unable to create tun interface");
        if_name = ifr.ifr_name;
        return fd;
    }

  protected:
    int tun_add(std::string if_name, handle_cb recv_cb, handle_cb send_cb = NULL)
    {
        int fd = create(if_name);
        return super::file_add(fd, recv_cb, send_cb);
    }

  public:
    socket_tun(const struct arguments &args)
        : super(args)
    {}
};

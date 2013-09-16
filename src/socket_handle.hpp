#pragma once

#include <memory>

#include "base_handle.hpp"
#include "socket_operations.hpp"

template<class hdr>
class socket_handle
    : public base_handle,
      public socket_operations,
      public hdr
{
    void socket_open()
    {
        int if_index;

        m_fd = socket(hdr::domain(), hdr::family(), hdr::protocol());
        if (m_fd < 0)
            throw std::system_error(errno, std::system_category(),
                                    "unable to open socket");

        if_index = interface_index(m_fd, m_name);
        interface_promisc_on(m_fd, m_name);
        set_non_blocking(m_fd);
        //socket_filter_outbound(m_fd);
        interface_address(m_fd, m_name, hdr::if_address());
        hdr::sockaddr_prepare(if_index);

        if (bind(m_fd, hdr::sockaddr(), hdr::sockaddr_len()) < 0)
            throw std::system_error(errno, std::system_category(),
                                    "unable to bind to socket");
    }

  public:
    typedef std::shared_ptr<socket_handle<hdr>> pointer;

    socket_handle(std::string if_name, cb recv_cb, cb send_cb)
        : base_handle(if_name, recv_cb, send_cb)
    {
        socket_open();
    }

    ~socket_handle()
    {
        interface_promisc_off(m_fd, m_name);
        close(m_fd);
    }
};


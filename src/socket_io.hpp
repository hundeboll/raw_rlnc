#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <cstring>
#include <ctime>
#include <cerrno>
#include <system_error>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>

#include "arguments.hpp"
#include "socket_operations.hpp"
#include "socket_handle.hpp"
#include "file_ref.hpp"

static volatile bool do_return = false;

static void signal_handler(int sig)
{
    do_return = true;
}

template<class super>
class socket_io : public super
{
    typedef std::function<void ()> timer_cb;
    typedef socket_handle<typename super::packet_hdr> sock_h;
    typedef typename sock_h::pointer sock_pointer;
    typedef file_ref::pointer file_pointer;
    typedef typename base_handle::cb handle_cb;
    typedef typename base_handle::pointer handle_pointer;

    std::vector<handle_pointer> m_fds;
    std::vector<timer_cb> m_timers;
    fd_set m_read_fds, m_write_fds;
    struct timeval m_timeout;

    sock_pointer sock(int fd) const
    {
        return std::static_pointer_cast<sock_h>(m_fds.at(fd));
    }

    file_pointer file(int fd) const
    {
        return std::static_pointer_cast<file_ref>(m_fds.at(fd));
    }

    void fd_add(handle_pointer &p)
    {
        if (p->read())
            FD_SET(p->fd(), &m_read_fds);

        if (p->write())
            FD_SET(p->fd(), &m_write_fds);

        if (p->fd() >= m_fds.size())
            m_fds.resize(p->fd() + 1);

        m_fds[p->fd()] = p;
    }

    void handle()
    {
        fd_set read_fds, write_fds;
        int res, nfds = m_fds.back()->fd() + 1;
        struct timeval timeout;

        while (!do_return) {
            read_fds = m_read_fds;
            write_fds = m_write_fds;
            timeout = m_timeout;

            res = select(nfds, &read_fds, &write_fds, NULL, &timeout);

            if (res < 0 && errno != EINTR) {
                throw std::system_error(errno, std::system_category(),
                                        "select");
            } else if (do_return) {
                return;
            } else if (res == 0) {
                for (auto i : m_timers)
                    i();
                continue;
            }

            for (int i = 0; i < nfds; ++i) {
                if (FD_ISSET(i, &m_read_fds) && FD_ISSET(i, &read_fds))
                    m_fds[i]->handle_read();

                if (FD_ISSET(i, &m_write_fds) && FD_ISSET(i, &write_fds))
                    m_fds[i]->handle_write();
            }
        }
    }

  protected:
    enum file_mode : int {
        mode_rdonly = O_RDONLY,
        mode_wronly = O_WRONLY,
        mode_rdwr   = O_RDWR,
    };

    void timer_add(timer_cb cb)
    {
        m_timers.push_back(cb);
    }

    int socket_add(std::string if_name, handle_cb recv_cb, handle_cb send_cb = NULL)
    {
        handle_pointer s(new sock_h(if_name, recv_cb, send_cb));

        fd_add(s);

        return s->fd();
    }

    int file_add(std::string path, handle_cb read_cb, handle_cb write_cb)
    {
        handle_pointer f(new file_ref(path, read_cb, write_cb));

        fd_add(f);

        return f->fd();
    }

    int file_add(int fd, handle_cb read_cb, handle_cb write_cb)
    {
        handle_pointer f(new file_ref(fd, read_cb, write_cb));

        fd_add(f);

        return f->fd();
    }

    int file_add(std::string path, file_mode mode)
    {
        handle_pointer f(new file_ref(path, mode));

        fd_add(f);

        return f->fd();
    }

    int file_add(int fd)
    {
        handle_pointer f(new file_ref(fd, NULL, NULL));

        fd_add(f);

        return f->fd();
    }

    ssize_t socket_recv(int fd, uint8_t *buf, size_t len)
    {
        unsigned int addr_len = sock(fd)->sockaddr_len();

        return recvfrom(fd, buf, len, 0,
                        sock(fd)->sockaddr_recv(),
                        &addr_len);
    }

    ssize_t socket_send(int fd, uint8_t *buf, size_t len)
    {
        return sendto(fd, buf, len, 0,
                      sock(fd)->sockaddr(),
                      sock(fd)->sockaddr_len());
    }

    ssize_t file_read(int fd, uint8_t *buf, size_t len)
    {
        return read(fd, buf, len);
    }

    ssize_t file_write(int fd, uint8_t *buf, size_t len)
    {
        return write(fd, buf, len);
    }

    void fd_enable_read(int fd)
    {
        FD_SET(fd, &m_read_fds);
    }

    void fd_enable_write(int fd)
    {
        FD_SET(fd, &m_write_fds);
    }

    void fd_disable_read(int fd)
    {
        FD_CLR(fd, &m_read_fds);
    }

    void fd_disable_write(int fd)
    {
        FD_CLR(fd, &m_write_fds);
    }

    void start()
    {
        handle();
    }

    void stop()
    {
        do_return = true;
    }

    uint8_t *packet_hdr_add(int fd, uint8_t *pkt) const
    {
        if (super::packet_hdr_do_broadcast())
            return sock(fd)->hdr_add_broadcast(pkt);
        else
            return sock(fd)->hdr_add(pkt, super::packet_hdr_dest());
    }

    uint8_t *packet_hdr_add(int fd, uint8_t *pkt, const uint8_t *dst) const
    {
        return sock(fd)->hdr_add(pkt, dst);
    }

    uint8_t *packet_hdr_add_reply(int fd, uint8_t *pkt) const
    {
        return sock(fd)->hdr_add_reply(pkt);
    }

    uint8_t *packet_hdr_add_broadcast(int fd, uint8_t *pkt) const
    {
        return sock(fd)->hdr_add_broadcast(pkt);
    }

  public:
    socket_io(const struct arguments &args)
        : super(args),
          m_timeout({0, args.timeout*1000})
    {
        FD_ZERO(&m_read_fds);
        FD_ZERO(&m_write_fds);
        socket_operations::install_signals(&signal_handler);
    }
};

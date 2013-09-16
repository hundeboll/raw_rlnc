#pragma once

#include <memory>
#include <stdexcept>
#include <system_error>
#include <sys/stat.h>
#include <fcntl.h>

#include "base_handle.hpp"
#include "socket_operations.hpp"

class file_ref : public base_handle
{
    void file_open(std::string path, int flags)
    {
        int permissions = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

        flags |= flags & O_WRONLY ? O_TRUNC : 0;

        m_fd = open(path.c_str(), flags | O_NONBLOCK | O_CREAT, permissions);

        if (m_fd < 0)
            throw std::system_error(errno, std::system_category(),
                                    "unable to open file " + path);
    }

  public:
    typedef std::shared_ptr<file_ref> pointer;

    file_ref(int fd, cb read_cb, cb write_cb)
        : base_handle(fd, read_cb, write_cb)
    {
        socket_operations::set_non_blocking(fd);
    }

    file_ref(std::string path, int mode)
        : base_handle(path, NULL, NULL)
    {
        file_open(path, mode);
    }

    file_ref(std::string path, cb read_cb, cb write_cb)
        : base_handle(path, read_cb, write_cb)
    {
        if (read_cb && write_cb)
            file_open(path, O_RDWR);
        else if (read_cb && !write_cb)
            file_open(path, O_RDONLY);
        else if (!read_cb && write_cb)
            file_open(path, O_WRONLY);
        else
            throw std::runtime_error("read and/or write mode required");
    }
};

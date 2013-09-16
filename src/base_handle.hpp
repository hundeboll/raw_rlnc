#pragma once

#include <memory>
#include <functional>

class base_handle
{
  public:
    typedef std::function<void(int)> cb;
    typedef std::shared_ptr<base_handle> pointer;

  private:
    cb m_read_callback = NULL, m_write_callback = NULL;

  protected:
    int m_fd;
    std::string m_name;

  public:
    base_handle(std::string name, cb read_cb, cb write_cb)
        : m_name(name),
          m_read_callback(read_cb),
          m_write_callback(write_cb)
    {}

    base_handle(int fd, cb read_cb, cb write_cb)
        : m_fd(fd),
          m_read_callback(read_cb),
          m_write_callback(write_cb)
    {}

    void handle_read() const
    {
        if (m_read_callback)
            m_read_callback(m_fd);
    }

    void handle_write() const
    {
        if (m_write_callback)
            m_write_callback(m_fd);
    }

    const int fd() const
    {
        return m_fd;
    }

    bool read() const
    {
        return m_read_callback != NULL;
    }

    bool write() const
    {
        return m_write_callback != NULL;
    }
};

#pragma once

#include <chrono>
#include <thread>

template<class super>
class socket_rate : public super
{
    typedef std::chrono::high_resolution_clock timer;
    typedef timer::time_point timestamp;
    typedef std::chrono::microseconds usecs;
    typedef std::chrono::milliseconds msecs;
    typedef std::chrono::duration<msecs> duration;

    std::chrono::microseconds m_interval;
    timestamp m_timestamp_last, m_start, m_end;
    size_t m_bytes = 0;

    void wait()
    {
        usecs diff;

        if (m_timestamp_last == timestamp()) {
            std::this_thread::sleep_for(m_interval);
            return;
        }

        diff = std::chrono::duration_cast<usecs>(m_timestamp_last - timer::now());

        if (diff >= m_interval)
            return;

        std::this_thread::sleep_for(m_interval - diff);

    }

  protected:
    ssize_t socket_send(int sock, uint8_t *buf, size_t len)
    {
        ssize_t ret = super::socket_send(sock, buf, len);

        m_bytes += ret - super::header_len();

        if (m_start == timestamp())
            m_start = timer::now();

        m_end = timer::now();

        if (!m_interval.count())
            return ret;

        wait();
        m_timestamp_last = timer::now();

        return ret;
    }

  public:
    socket_rate(const struct arguments &args)
        : super(args),
          m_interval(0)
    {
        size_t microseconds;

        if (args.rate) {
            microseconds = 1000000/(1024*args.rate/args.symbol_size/8);
            m_interval = std::chrono::microseconds(microseconds);
        }
    }

    ~socket_rate()
    {
        msecs diff;
        size_t bitrate;

        diff = std::chrono::duration_cast<msecs>(m_end - m_start);

        if (diff.count())
            bitrate = 8000*m_bytes/diff.count()/1024;
        else
            bitrate = 0;

        std::cout << "milliseconds: " << diff.count() << std::endl;
        std::cout << "kbps: " << bitrate << std::endl;
    }
};

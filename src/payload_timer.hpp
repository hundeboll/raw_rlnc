#pragma once

#include <iostream>
#include <chrono>
#include "arguments.hpp"

template<class super>
class payload_timer : public super
{
    typedef std::chrono::high_resolution_clock timer;
    typedef timer::time_point timestamp;
    typedef std::chrono::milliseconds resolution;
    typedef std::chrono::duration<resolution> duration;

    timestamp m_start, m_end;

  protected:
    size_t payload_data_read(uint8_t *data)
    {
        if (m_start == timestamp())
            m_start = timer::now();

        return super::payload_data_read(data);
    }

    bool payload_data_is_complete()
    {
        m_end = timer::now();

        return super::payload_data_is_complete();
    }

  public:
    payload_timer(const struct arguments &args) : super(args)
    {}

    ~payload_timer()
    {
        resolution diff;
        size_t bitrate;

        diff = std::chrono::duration_cast<resolution>(m_end - m_start);

        bitrate = 8000*super::payload_data_len()/diff.count()/1024;
        std::cout << "milliseconds: " << diff.count() << std::endl;
        std::cout << "kbps: " << bitrate << std::endl;
    }
};

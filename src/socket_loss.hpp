#pragma once

#include <iostream>
#include <functional>
#include <random>
#include <chrono>
#include "arguments.hpp"

class socket_loss_base
{
    std::default_random_engine m_generator;
    std::bernoulli_distribution m_dist_e1;
    std::bernoulli_distribution m_dist_e2;
    std::bernoulli_distribution m_dist_e3;
    std::bernoulli_distribution m_dist_e4;

  protected:
    bool e1_is_lost()
    {
        return m_dist_e1(m_generator);
    }

    bool e2_is_lost()
    {
        return m_dist_e2(m_generator);
    }

    bool e3_is_lost()
    {
        return m_dist_e3(m_generator);
    }

    bool e4_is_lost()
    {
        return m_dist_e4(m_generator);
    }

    socket_loss_base(const struct arguments &args)
        : m_generator(std::chrono::system_clock::now().time_since_epoch().count()),
          m_dist_e1(args.e1/100.0),
          m_dist_e2(args.e2/100.0),
          m_dist_e3(args.e3/100.0),
          m_dist_e4(args.e4/100.0)
    {}
};

template<class super>
class socket_loss_helper : private socket_loss_base, public super
{
    size_t m_lost_source = 0;

  protected:
    ssize_t socket_recv(int sock, uint8_t *buf, size_t len)
    {
        ssize_t ret = super::socket_recv(sock, buf, len);

        if (ret <= 0)
            return ret;

        if (super::packet_hdr_is_from_source(buf) &&
            socket_loss_base::e1_is_lost() &&
            m_lost_source++)
            return 0;

        return ret;
    }

  public:
    socket_loss_helper(const struct arguments &args)
        : socket_loss_base(args),
          super(args)
    {}

    ~socket_loss_helper()
    {
        std::cout << "lost source: " << m_lost_source << std::endl;
    }
};

template<class super>
class socket_loss_next_hop : private socket_loss_base, public super
{
    size_t m_lost_helper = 0, m_lost_source = 0, m_lost_relay = 0;

  protected:
    ssize_t socket_recv(int sock, uint8_t *buf, size_t len)
    {
        ssize_t ret = super::socket_recv(sock, buf, len);

        if (ret <= 0)
            return ret;

        if (super::packet_hdr_is_from_helper(buf) &&
            socket_loss_base::e2_is_lost() &&
            m_lost_helper++)
            return 0;
        else if (super::packet_hdr_is_from_source(buf) &&
                 socket_loss_base::e3_is_lost() &&
                 m_lost_source++)
            return 0;
        else if (super::packet_hdr_is_from_relay(buf) &&
                 socket_loss_base::e4_is_lost() &&
                 m_lost_relay++)
            return 0;

        return ret;
    }

  public:
    socket_loss_next_hop(const struct arguments &args)
        : socket_loss_base(args),
          super(args)
    {}

    ~socket_loss_next_hop()
    {
        std::cout << "lost source: " << m_lost_source << std::endl;
        std::cout << "lost helper: " << m_lost_helper << std::endl;
        std::cout << "lost relay:  " << m_lost_relay << std::endl;
    }
};

template<class super>
class socket_loss_relay : private socket_loss_base, public super
{
  protected:
    ssize_t socket_recv(int sock, uint8_t *buf, size_t len)
    {
        ssize_t ret = super::socket_recv(sock, buf, len);

        if (ret <= 0)
            return ret;

        if (super::packet_hdr_is_from_source(buf) &&
            socket_loss_base::e3_is_lost())
            return 0;

        return ret;
    }

  public:
    socket_loss_relay(const struct arguments &args)
        : socket_loss_base(args),
          super(args)
    {}
};

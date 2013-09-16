#pragma once

#include <stdexcept>
#include <cstdint>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include "arguments.hpp"

template<class super>
class packet_hdr_ethernet : public super
{
    typedef struct ethhdr packet_header;

    uint8_t m_destination[ETH_ALEN], m_relay[ETH_ALEN] = {0};
    uint8_t m_source[ETH_ALEN] = {0}, m_helper[ETH_ALEN] = {0};
    bool m_do_broadcast;

    static packet_header *header(const uint8_t *data)
    {
        return (packet_header *)data;
    }

    static uint8_t *source(const uint8_t *data)
    {
        return (uint8_t *)header(data)->h_source;
    }

    static uint8_t *destination(const uint8_t *data)
    {
        return (uint8_t *)header(data)->h_dest;
    }

  protected:
    class packet_hdr
    {
        uint8_t m_if_address[ETH_ALEN];
        uint8_t m_destination[ETH_ALEN];
        struct sockaddr_ll m_sock_addr, m_sock_recv_addr;
        static const uint16_t m_proto = {0x4306};
        const uint8_t m_broadcast[ETH_ALEN] = {0xff,0xff,0xff,0xff,0xff,0xff};

      protected:
        void sockaddr_prepare(size_t if_index)
        {
            memset(&m_sock_addr, 0, sizeof(m_sock_addr));
            m_sock_addr.sll_family   = AF_PACKET;
            m_sock_addr.sll_protocol = htons(m_proto);
            m_sock_addr.sll_ifindex  = if_index;
            m_sock_addr.sll_halen    = ETH_ALEN;
        }

        static int domain()
        {
            return PF_PACKET;
        }

        static int family()
        {
            return SOCK_RAW;
        }

        static int protocol()
        {
            return m_proto;
        }

        uint8_t *if_address()
        {
            return m_if_address;
        }

      public:
        struct sockaddr *sockaddr() const
        {
            return (struct sockaddr *)&m_sock_addr;
        }

        static size_t sockaddr_len()
        {
            return sizeof(struct sockaddr_ll);
        }

        struct sockaddr *sockaddr_recv()
        {
            return (struct sockaddr *)&m_sock_recv_addr;
        }

        uint8_t *hdr_add(uint8_t *packet, const uint8_t *dst) const
        {
            packet_header *hdr = header(packet);

            memcpy(hdr->h_dest, dst, ETH_ALEN);
            memcpy(hdr->h_source, m_if_address, ETH_ALEN);
            hdr->h_proto = htons(m_proto);

            return packet + packet_hdr_len();
        }

        uint8_t *hdr_add_reply(uint8_t *pkt) const
        {
            return hdr_add(pkt, m_sock_recv_addr.sll_addr);
        }

        uint8_t *hdr_add_broadcast(uint8_t *pkt) const
        {
            return hdr_add(pkt, m_broadcast);
        }
    };

    static size_t header_len()
    {
        return packet_hdr_len() + super::header_len();
    }

    size_t frame_len() const
    {
        return packet_hdr_len() + super::frame_len();
    }

    static size_t packet_hdr_len()
    {
        return sizeof(packet_header);
    }

    static size_t packet_hdr_address_len()
    {
        return ETH_ALEN;
    }

    bool packet_hdr_do_broadcast() const
    {
        return m_do_broadcast;
    }

    const uint8_t *packet_hdr_source() const
    {
        return m_source;
    }

    const uint8_t *packet_hdr_dest() const
    {
        return m_destination;
    }

    static uint8_t *packet_hdr_source(const uint8_t *packet)
    {
        return source(packet);
    }

    static uint8_t *packet_hdr_dest(const uint8_t *packet)
    {
        return destination(packet);
    }

    bool packet_hdr_is_from_source(const uint8_t *data) const
    {
        return memcmp(source(data), m_source, ETH_ALEN) == 0;
    }

    bool packet_hdr_is_from_helper(const uint8_t *data) const
    {
        return memcmp(source(data), m_helper, ETH_ALEN) == 0;
    }

    bool packet_hdr_is_from_dest(const uint8_t *data) const
    {
        return memcmp(source(data), m_destination, ETH_ALEN) == 0;
    }

    bool packet_hdr_is_from_relay(const uint8_t *data) const
    {
        return memcmp(source(data), m_relay, ETH_ALEN) == 0;
    }

    static void packet_hdr_parse_address(const char *buf, uint8_t *out)
    {
        size_t count = 0, j;
        char *i = (char *)buf;

        if (strlen(buf) != 6*3 - 1)
            throw std::runtime_error("invalid address length");

        for (j = 1; j < 6; ++j)
            if (buf[j*3 - 1] != ':')
                throw std::runtime_error("invalid address format");

        while (i && *i && count < 6)
            if (*i != ':')
                out[count++] = strtol(i, &i, 16);
            else
                i++;
    }

    static void packet_hdr_parse_address(const uint8_t *buf, uint8_t *out)
    {
        packet_hdr_parse_address((const char *)buf, out);
    }

    static void packet_hdr_print_address(const uint8_t *addr)
    {
        printf("%02hhx", addr[0]);
        for (size_t i = 1; i < ETH_ALEN; ++i)
            printf(":%02hhx", addr[i]);
    }

    static void packet_hdr_print_source(const uint8_t *h)
    {
        packet_hdr_print_address(packet_hdr_source(h));
    }

  public:
    packet_hdr_ethernet(const struct arguments &args)
        : super(args),
          m_do_broadcast(args.broadcast)
    {
        if (args.destination)
            packet_hdr_parse_address(args.destination, m_destination);

        if (args.source)
            packet_hdr_parse_address(args.source, m_source);

        if (args.helper)
            packet_hdr_parse_address(args.helper, m_helper);

        if (args.relay)
            packet_hdr_parse_address(args.relay, m_relay);
    }
};

#pragma once

#include <cstdint>
#include <arpa/inet.h>
#include "arguments.hpp"

template<class super>
class payload_hdr_rlnc : public super
{
    enum payload_type : uint8_t {
        type_none,
        type_raw,
        type_enc,
        type_rec,
        type_hlp,
        type_ack,
        type_done,
    };

    typedef uint16_t sequence_type;
    typedef uint8_t block_id_type;

    struct payload_hdr {
        sequence_type sequence = 0;
        payload_type  type     = type_none;
        block_id_type block_id = 0;
    } __attribute__((packed));

    sequence_type m_sequence = 1;
    block_id_type m_block_id = 0;

    static struct payload_hdr *hdr(uint8_t *data)
    {
        return (struct payload_hdr *)data;
    }

    static payload_type type(uint8_t *data)
    {
        return hdr(data)->type;
    }

    static sequence_type sequence(uint8_t *data)
    {
        return ntohs(hdr(data)->sequence);
    }

    static block_id_type block_id(uint8_t *data)
    {
        return hdr(data)->block_id;
    }

  protected:
    static size_t header_len()
    {
        return payload_hdr_len() + super::header_len();
    }

    size_t frame_len() const
    {
        return payload_hdr_len() + super::frame_len();
    }

    static size_t payload_hdr_len()
    {
        return sizeof(struct payload_hdr);
    }

    static bool payload_hdr_is_raw(uint8_t *data)
    {
        return type(data) == type_raw;
    }

    static bool payload_hdr_is_enc(uint8_t *data)
    {
        return type(data) == type_enc;
    }

    static bool payload_hdr_is_rec(uint8_t *data)
    {
        return type(data) == type_rec;
    }

    static bool payload_hdr_is_hlp(uint8_t *data)
    {
        return type(data) == type_hlp;
    }

    static bool payload_hdr_is_ack(uint8_t *data)
    {
        return type(data) == type_ack;
    }

    static bool payload_hdr_is_done(uint8_t *data)
    {
        return type(data) == type_done;
    }

    uint8_t *payload_hdr_add_raw(uint8_t *data)
    {
        struct payload_hdr *h = hdr(data);

        h->sequence = htons(m_sequence++);
        h->type = type_raw;
        h->block_id = m_block_id;

        return data + payload_hdr_len();
    }

    uint8_t *payload_hdr_add_enc(uint8_t *data)
    {
        struct payload_hdr *h = hdr(data);

        h->sequence = htons(m_sequence++);
        h->type = type_enc;
        h->block_id = m_block_id;

        return data + payload_hdr_len();
    }

    uint8_t *payload_hdr_add_rec(uint8_t *data)
    {
        struct payload_hdr *h = hdr(data);

        h->sequence = htons(m_sequence++);
        h->type = type_rec;
        h->block_id = m_block_id;

        return data + payload_hdr_len();
    }

    uint8_t *payload_hdr_add_hlp(uint8_t *data)
    {
        struct payload_hdr *h = hdr(data);

        h->sequence = htons(m_sequence++);
        h->type = type_hlp;
        h->block_id = m_block_id;

        return data + payload_hdr_len();
    }

    uint8_t *payload_hdr_add_ack(uint8_t *data, block_id_type b)
    {
        struct payload_hdr *h = hdr(data);

        h->sequence = 0;
        h->type = type_ack;
        h->block_id = b;

        return data + payload_hdr_len();
    }

    uint8_t *payload_hdr_add_ack(uint8_t *data)
    {
        return payload_hdr_add_ack(data, m_block_id);
    }

    uint8_t *payload_hdr_add_done(uint8_t *data)
    {
        struct payload_hdr *h = hdr(data);

        h->sequence = 0;
        h->type = type_done;
        h->block_id = 0;

        return data + payload_hdr_len();
    }

    size_t payload_hdr_sequence(uint8_t *data)
    {
        return sequence(data);
    }

    size_t payload_hdr_sequence()
    {
        return m_sequence;
    }

    bool payload_hdr_sequence_equal(uint8_t *data)
    {
        return m_sequence == sequence(data);
    }

    size_t payload_hdr_block_id(uint8_t *data)
    {
        return block_id(data);
    }

    size_t payload_hdr_block_id()
    {
        return m_block_id;
    }

    bool payload_hdr_block_id_equal(uint8_t *data)
    {
        return m_block_id == block_id(data);
    }

    void payload_data_reset()
    {
        m_block_id++;
        m_sequence = 0;
        super::payload_data_reset();
    }

  public:
    payload_hdr_rlnc(const struct arguments &args) : super(args)
    {}
};

#pragma once

#include <vector>
#include <cstdint>
#include <cstring>
#include "codes.hpp"
#include "arguments.hpp"

template<class Coder, class super>
class payload_data_rlnc_base : public super
{
  protected:
    typename Coder::factory m_factory;
    typename Coder::pointer m_coder;

    size_t frame_len() const
    {
        return m_coder->payload_size() + super::frame_len();
    }

    size_t payload_data_len()
    {
        return m_coder->block_size();
    }

    size_t payload_data_symbols()
    {
        return m_coder->symbols();
    }

    size_t payload_data_received() const
    {
        return m_coder->rank();
    }

    virtual void payload_data_add(uint8_t *data, size_t len)
    {
    }

    virtual size_t payload_data_get(uint8_t *data)
    {
        return 0;
    }

    virtual size_t payload_data_read(uint8_t *data)
    {
        return 0;
    }

    virtual uint8_t *payload_data_write(uint8_t *data)
    {
        return NULL;
    }

    virtual void payload_data_reset()
    {
        m_coder->initialize(m_factory);
    }

  public:
    payload_data_rlnc_base(const struct arguments &args)
        : m_factory(args.symbols, args.symbol_size),
          m_coder(m_factory.build()),
          super(args)
    {
    }
};

template<class Encoder, class super>
class payload_data_rlnc_encoder : public payload_data_rlnc_base<Encoder, super>
{
    typedef payload_data_rlnc_base<Encoder, super> Base;
    size_t m_symbol_idx = 0;

  protected:
    uint8_t *payload_data_write(uint8_t *data)
    {
        Base::m_coder->encode(data);

        return data + Base::m_coder->payload_size();
    }

    void payload_data_add(uint8_t *data, size_t len)
    {
        sak::const_storage symbol(data, len);
        Base::m_coder->set_symbol(m_symbol_idx++, symbol);
    }

    void payload_data_reset()
    {
        m_symbol_idx = 0;
        Base::payload_data_reset();
    }

    bool payload_data_is_full()
    {
        return Base::m_coder->rank() == Base::m_coder->symbols();
    }

  public:
    payload_data_rlnc_encoder(const struct arguments &args)
        : Base(args)
    {
        Base::m_coder->seed(time(0));
    }
};

template<class Decoder, class super>
class payload_data_rlnc_decoder : public payload_data_rlnc_base<Decoder, super>
{
    typedef payload_data_rlnc_base<Decoder, super> Base;

    size_t m_non_innovative = 0;
    size_t m_decoded = 0;

  protected:
    bool payload_data_is_complete() const
    {
        return Base::m_coder->is_complete();
    }

    bool payload_data_is_partial_complete() const
    {
        return Base::m_coder->is_complete() ||
               (Base::m_coder->is_partial_complete() &&
               Base::m_coder->symbol_pivot(m_decoded));
    }

    bool payload_data_is_decoded() const
    {
        return m_decoded == Base::m_coder->symbols();
    }

    size_t payload_data_get(uint8_t *data)
    {
        size_t symbol_size = Base::m_coder->symbol_size();

        memcpy(data, Base::m_coder->symbol(m_decoded++), symbol_size);

        return symbol_size;
    }

    size_t payload_data_read(uint8_t *data)
    {
        size_t rank = Base::m_coder->rank();

        Base::m_coder->decode(data);

        if (Base::m_coder->rank() == rank)
            m_non_innovative++;

        return Base::m_factory.symbol_size();
    }

    uint8_t *payload_data_write(uint8_t *data)
    {
        Base::m_coder->recode(data);

        return data + Base::m_coder->payload_size();
    }

    void payload_data_reset()
    {
        m_decoded = 0;
        Base::payload_data_reset();
    }

  public:
    payload_data_rlnc_decoder(const struct arguments &args)
        : Base(args)
    {}

    ~payload_data_rlnc_decoder()
    {
        std::cout << "non-innovative: " << m_non_innovative << std::endl;
    }
};

#pragma once

template<class super>
class payload_data_raw : public super
{
    size_t m_symbol_size;

  protected:
    size_t frame_len() const
    {
        return m_symbol_size + super::frame_len();
    }

    uint8_t *payload_data_set(uint8_t *pkt)
    {
        return pkt;
    }

    uint8_t payload_data_get(uint8_t *pkt)
    {
    }

    size_t payload_data_len() const
    {
        return m_symbol_size;
    }

  public:
    payload_data_raw(const struct arguments &args)
        : super(args),
          m_symbol_size(args.symbol_size)
    {}
};

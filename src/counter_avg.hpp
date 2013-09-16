#pragma once

template<class super>
class counter_avg : public super
{
    size_t m_tx = 0;
    size_t m_tx_raw = 0;
    size_t m_tx_enc = 0;
    size_t m_tx_rec = 0;
    size_t m_tx_hlp = 0;
    size_t m_tx_dec = 0;
    size_t m_tx_ack = 0;
    size_t m_rx = 0;
    size_t m_rx_raw = 0;
    size_t m_rx_enc = 0;
    size_t m_rx_rec = 0;
    size_t m_rx_hlp = 0;
    size_t m_rx_dec = 0;
    size_t m_rx_ack = 0;
    size_t m_blocks = 0;
    size_t m_last_enc = 0;
    size_t m_last_rec = 0;
    size_t m_last_hlp = 0;
    size_t m_seq_enc = 0;
    size_t m_seq_rec = 0;
    size_t m_seq_hlp = 0;

  protected:
    void payload_data_reset()
    {
        if (m_seq_enc)
            m_last_enc += m_seq_enc;

        if (m_seq_rec)
            m_last_rec += m_seq_rec;

        if (m_seq_hlp)
            m_last_hlp += m_seq_hlp;

        m_blocks++;
        super::payload_data_reset();
    }

    ssize_t socket_recv(int sock, uint8_t *data, size_t len)
    {
        uint8_t *payload_hdr = data + super::packet_hdr_len();
        int ret = super::socket_recv(sock, data, len);

        if (ret <= 0)
            return ret;

        if (super::payload_hdr_is_raw(payload_hdr)) {
            m_rx_raw++;
        } else if (super::payload_hdr_is_enc(payload_hdr)) {
            m_rx_enc++;
            m_seq_enc = super::payload_hdr_sequence(payload_hdr);
        } else if (super::payload_hdr_is_rec(payload_hdr)) {
            m_rx_rec++;
            m_seq_rec = super::payload_hdr_sequence(payload_hdr);
        } else if (super::payload_hdr_is_hlp(payload_hdr)) {
            m_rx_hlp++;
            m_seq_hlp = super::payload_hdr_sequence(payload_hdr);
        } else if (super::payload_hdr_is_ack(payload_hdr)) {
            m_rx_ack++;
        }

        m_rx++;
        return ret;
    }

    ssize_t socket_send(int sock, uint8_t *data, size_t len)
    {
        uint8_t *payload_hdr = data + super::packet_hdr_len();
        int ret = super::socket_send(sock, data, len);

        if (ret <= 0)
            return ret;

        if (super::payload_hdr_is_raw(payload_hdr))
            m_tx_raw++;
        else if (super::payload_hdr_is_enc(payload_hdr))
            m_tx_enc++;
        else if (super::payload_hdr_is_rec(payload_hdr))
            m_tx_rec++;
        else if (super::payload_hdr_is_ack(payload_hdr))
            m_tx_ack++;
        else if (super::payload_hdr_is_hlp(payload_hdr))
            m_tx_hlp++;

        m_tx++;
        return ret;
    }

  public:
    counter_avg(const struct arguments &args)
        : super(args)
    {}

    ~counter_avg()
    {
        print_counters();
    }

    void print_counters()
    {

        size_t redundant = m_rx_enc + m_rx_rec + m_rx_hlp;

        if (redundant && m_tx_raw)
            redundant -= m_tx_raw;
        else if (redundant && m_tx_hlp)
            redundant -= m_tx_hlp;
        else
            redundant = 0;

        std::cout << std::endl;
        std::cout << "tx:        " << m_tx       << std::endl;
        std::cout << "tx ack:    " << m_tx_ack   << std::endl;
        std::cout << "tx raw:    " << m_tx_raw   << std::endl;
        std::cout << "tx enc:    " << m_tx_enc   << std::endl;
        std::cout << "tx rec:    " << m_tx_rec   << std::endl;
        std::cout << "tx hlp:    " << m_tx_hlp   << std::endl;
        std::cout << "rx:        " << m_rx       << std::endl;
        std::cout << "rx ack:    " << m_rx_ack   << std::endl;
        std::cout << "rx raw:    " << m_rx_raw   << std::endl;
        std::cout << "rx enc:    " << m_rx_enc   << std::endl;
        std::cout << "rx rec:    " << m_rx_rec   << std::endl;
        std::cout << "rx hlp:    " << m_rx_hlp   << std::endl;
        std::cout << "blocks:    " << m_blocks   << std::endl;
        std::cout << "redundant: " << redundant  << std::endl;
        std::cout << "last enc:  " << m_last_enc << std::endl;
        std::cout << "last rec:  " << m_last_rec << std::endl;
        std::cout << "last hlp:  " << m_last_hlp << std::endl;
    }
};

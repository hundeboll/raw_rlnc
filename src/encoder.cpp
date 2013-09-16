#include <iostream>

#include "arguments.hpp"
#include "counter_avg.hpp"
#include "socket_io.hpp"
#include "packet_hdr_ethernet.hpp"
#include "payload_hdr_rlnc.hpp"
#include "payload_length.hpp"
#include "payload_data_rlnc.hpp"
#include "final_rlnc.hpp"
#include "codes.hpp"
#include "budgets.hpp"

typedef counter_avg<
        socket_io<
        packet_hdr_ethernet<
        payload_hdr_rlnc<
        payload_length<
        payload_data_rlnc_encoder<kodo::encoder,
        final_coder
        >>>>>> encoder_stack;

class encoder : public encoder_stack
{
    std::vector<uint8_t> m_frame;
    uint8_t *m_packet_hdr;
    uint8_t *m_payload_hdr;
    uint8_t *m_payload_data;
    int m_sock_out, m_sock_in;
    double m_packets = 0;
    double m_credits, m_budget_max, m_budget = 0;
    size_t m_current_seq = 0;

    void send_done()
    {
        for (size_t i = 0; i < 10; ++i) {
            packet_hdr_add(m_sock_out, m_packet_hdr);
            payload_hdr_add_done(m_payload_hdr);
            socket_send(m_sock_out, m_packet_hdr, header_len());
        }
    }

    void recv_raw(int sock)
    {
        size_t sequence;
        int len;

        while ((len = socket_recv(sock, &m_frame[0], m_frame.size())) > 0) {
            if (payload_hdr_is_done(m_payload_hdr)) {
                send_done();
                stop();
            }
            sequence = payload_hdr_sequence(m_payload_hdr);

            if (sequence == m_current_seq)
                continue;

            payload_data_add(m_payload_data, len - header_len());
            m_budget += m_credits;

            packet_hdr_add_reply(sock, m_packet_hdr);
            payload_hdr_add_ack(m_payload_hdr);
            socket_send(sock, &m_frame[0], header_len());
            m_current_seq = sequence;

            if (payload_data_is_full()) {
                fd_disable_read(m_sock_in);
                break;
            }
        }

        fd_enable_write(m_sock_out);
    }

    void send_budget(int sock)
    {
        if (payload_data_is_full())
            send_budget_max();
        else
            send_credits();
    }

    void send_credits()
    {
        while (m_budget >= 1) {
            send_packet();
            m_budget--;
        }

        fd_disable_write(m_sock_out);
    }

    void send_budget_max()
    {
        send_packet();

        if (m_packets >= m_budget_max)
            fd_disable_write(m_sock_out);
    }

    void send_packet()
    {
        packet_hdr_add(m_sock_out, m_packet_hdr);
        payload_hdr_add_enc(m_payload_hdr);
        payload_data_write(m_payload_data);
        socket_send(m_sock_out, &m_frame[0], m_frame.size());
        m_packets++;
    }

    void recv_packet(int sock)
    {
        while (socket_recv(sock, &m_frame[0], header_len()) > 0) {
            if (!payload_hdr_is_ack(m_payload_hdr))
                continue;

            if (!payload_hdr_block_id_equal(m_payload_hdr))
                continue;

            if (packet_hdr_is_from_helper(m_packet_hdr))
                continue;

            payload_data_reset();
            m_packets = 0;
            m_budget = 0;
            fd_disable_write(m_sock_out);
            fd_enable_read(m_sock_in);
        }
    }

    void timer()
    {
        if (!payload_data_is_full())
            return;

        m_budget += m_credits;
        send_credits();
    }

  public:
    encoder(struct arguments &args)
        : encoder_stack(args),
          m_frame(frame_len()),
          m_packet_hdr(&m_frame[0]),
          m_payload_hdr(&m_frame[packet_hdr_len()]),
          m_payload_data(&m_frame[header_len()]),
          m_credits(budgets::source_credits(args.symbols, args.e1/100.0,
                                            args.e2/100.0, args.e3/100.0)),
          m_budget_max(budgets::source_budget(args.symbols, args.e1/100.0,
                                              args.e2/100.0, args.e3/100.0))
    {
        auto rp = std::bind(&encoder::recv_packet, this, std::placeholders::_1);
        auto sb = std::bind(&encoder::send_budget, this, std::placeholders::_1);
        auto rr = std::bind(&encoder::recv_raw, this, std::placeholders::_1);
        auto t  = std::bind(&encoder::timer, this);

        m_sock_out = socket_add(args.interface, rp, sb);
        m_sock_in = socket_add(args.raw, rr);
        fd_disable_write(m_sock_out);
        timer_add(t);

        std::cout << "credits: " << m_credits << std::endl;
        std::cout << "budget:  " << m_budget_max << std::endl;

        start();
    }
};

int main(int argc, char **argv)
{
    if (arguments_parse(argc, argv) < 0)
        return EXIT_FAILURE;

    if (arguments.help) {
        arguments_usage(argv[0]);
        return EXIT_SUCCESS;
    }

    encoder enc(arguments);

    return EXIT_SUCCESS;
}

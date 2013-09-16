#include <iostream>
#include <functional>

#include "arguments.hpp"
#include "counter_avg.hpp"
#include "socket_loss.hpp"
#include "socket_io.hpp"
#include "packet_hdr_ethernet.hpp"
#include "payload_hdr_rlnc.hpp"
#include "payload_length.hpp"
#include "payload_data_raw.hpp"
#include "final_rlnc.hpp"
#include "arguments.hpp"

typedef counter_avg<
        socket_loss_relay<
        socket_io<
        packet_hdr_ethernet<
        payload_hdr_rlnc<
        payload_length<
        payload_data_raw<
        final_coder
        >>>>>>> relay_stack;

class relay : public relay_stack
{
    std::vector<uint8_t> m_frame;
    uint8_t *m_packet_hdr;
    uint8_t *m_payload_hdr;
    uint8_t *m_payload_data;
    int m_sock_raw, m_sock_pkt, m_sock_out = 0;
    ssize_t m_len_out = 0;
    size_t m_current_seq = 0;
    bool m_pending = false;

    void handle(int sock_in)
    {
        size_t sequence = payload_hdr_sequence(m_payload_hdr);

        if (m_len_out == 0)
            return;

        if (payload_hdr_is_done(m_payload_hdr) &&
            packet_hdr_is_from_source(m_packet_hdr)) {
            packet_hdr_add(m_sock_out, m_packet_hdr);
            std::cout << "done" << std::endl;
            for (size_t i = 0; i < 10; ++i)
                socket_send(m_sock_out, &m_frame[0], m_len_out);
            stop();
            return;
        }

        /*
        if (payload_hdr_is_raw(m_payload_hdr) &&
            sequence == m_current_seq)
            return;
        */

        if (payload_hdr_is_raw(m_payload_hdr) &&
            packet_hdr_is_from_source(m_packet_hdr)) {
            packet_hdr_add(m_sock_out, m_packet_hdr);
            socket_send(m_sock_out, &m_frame[0], m_len_out);
            m_current_seq = sequence;
            m_pending = true;
            return;
        }

        if (payload_hdr_is_ack(m_payload_hdr) &&
            packet_hdr_is_from_dest(m_packet_hdr) &&
            m_pending) {
            packet_hdr_add(m_sock_out, m_packet_hdr, packet_hdr_source());
            socket_send(m_sock_out, &m_frame[0], m_len_out);
            m_pending = false;
            return;
        }
    }

    void recv_raw(int sock)
    {
        ssize_t ret;

        if ((ret = socket_recv(m_sock_raw, &m_frame[0], m_frame.size())) < 0)
            return;

        m_sock_out = m_sock_pkt;
        m_len_out = ret;
        handle(sock);
    }

    void recv_pkt(int sock)
    {
        ssize_t ret;

        if ((ret = socket_recv(m_sock_pkt, &m_frame[0], m_frame.size())) < 0)
            return;

        m_sock_out = m_sock_raw;
        m_len_out = ret;
        handle(sock);
    }

    void timer()
    {
        if (!m_len_out)
            return;

        if (payload_hdr_is_raw(m_payload_hdr)) {
            packet_hdr_add(m_sock_out, m_packet_hdr);
            std::cout << "resend raw to ";
            packet_hdr_print_address(packet_hdr_dest(m_packet_hdr));
            std::cout << std::endl;
            socket_send(m_sock_out, &m_frame[0], m_len_out);
        }
    }

  public:
    relay(const struct arguments &args)
        : relay_stack(args),
          m_frame(frame_len()),
          m_packet_hdr(&m_frame[0]),
          m_payload_hdr(&m_frame[packet_hdr_len()]),
          m_payload_data(&m_frame[header_len()])
    {
        auto rr = std::bind(&relay::recv_raw, this, std::placeholders::_1);
        auto rp = std::bind(&relay::recv_pkt, this, std::placeholders::_1);
        auto t  = std::bind(&relay::timer, this);

        if (strncmp(args.interface, args.raw, IFNAMSIZ) == 0) {
            m_sock_raw = socket_add(args.interface, rr);
            m_sock_pkt = m_sock_raw;
        } else {
            m_sock_raw = socket_add(args.raw, rr);
            m_sock_pkt = socket_add(args.interface, rp);
        }

        //timer_add(t);

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

    relay relay(arguments);

    return EXIT_SUCCESS;
}

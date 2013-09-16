#include <iostream>

#include "arguments.hpp"
#include "counter_avg.hpp"
#include "socket_loss.hpp"
#include "socket_io.hpp"
#include "packet_hdr_ethernet.hpp"
#include "payload_hdr_rlnc.hpp"
#include "payload_length.hpp"
#include "payload_data_rlnc.hpp"
#include "final_rlnc.hpp"
#include "codes.hpp"
#include "budgets.hpp"

typedef counter_avg<
        socket_loss_next_hop<
        socket_io<
        packet_hdr_ethernet<
        payload_hdr_rlnc<
        payload_length<
        payload_data_rlnc_decoder<kodo::recoder,
        final_coder
        >>>>>>> recoder_stack;

class recoder : public recoder_stack, public budgets
{
    std::vector<uint8_t> m_frame;
    uint8_t *m_packet_hdr;
    uint8_t *m_payload_hdr;
    uint8_t *m_payload_data;
    double m_threshold;
    double m_budget_max;
    double m_credits;
    double m_budget = 0;
    size_t m_packets = 0;
    int m_sock;

    void send_done()
    {
        packet_hdr_add(m_sock, m_packet_hdr);
        payload_hdr_add_done(m_payload_hdr);
        socket_send(m_sock, m_packet_hdr, header_len());
    }

    void send_ack()
    {
        packet_hdr_add(m_sock, m_packet_hdr, packet_hdr_source());
        payload_hdr_add_ack(m_payload_hdr);
        socket_send(m_sock, &m_frame[0], header_len());
    }

    void send_recoded()
    {
        packet_hdr_add(m_sock, m_packet_hdr);
        payload_hdr_add_rec(m_payload_hdr);
        payload_data_write(m_payload_data);
        socket_send(m_sock, &m_frame[0], m_frame.size());
        m_packets++;
    }

    void send_budget(int sock)
    {
        if (payload_data_is_complete())
            send_budget_max();
        else
            send_credits();
    }

    void send_credits()
    {
        while (m_budget >= 1) {
            send_recoded();
            m_budget--;
        }

        fd_disable_write(m_sock);
    }

    void send_budget_max()
    {
        send_recoded();

        if (m_packets >= m_budget_max)
            fd_disable_write(m_sock);
    }

    void recv_packet(int sock)
    {
        ssize_t res;

        while ((res = socket_recv(sock, &m_frame[0], m_frame.size())) >= 0) {
            if (res == 0)
                continue;

            if (payload_hdr_is_done(m_payload_hdr)) {
                send_done();
                stop();
                return;
            }

            if (!payload_hdr_block_id_equal(m_payload_hdr))
                continue;

            if (payload_hdr_is_ack(m_payload_hdr) &&
                packet_hdr_is_from_dest(m_packet_hdr)) {
                send_ack();
                m_packets = 0;
                m_budget = 0;
                payload_data_reset();
                continue;
            }

            if (payload_data_is_complete())
                continue;

            if (payload_hdr_is_enc(m_payload_hdr) ||
                payload_hdr_is_hlp(m_payload_hdr)) {
                payload_data_read(m_payload_data);
                m_budget += m_credits;
                fd_enable_write(m_sock);
            }
        }
    }

    void timer()
    {
        if (!payload_data_is_complete())
            return;

        m_budget += m_credits;
        send_credits();
    }

  public:
    recoder(const struct arguments &args)
        : recoder_stack(args),
          m_frame(frame_len()),
          m_packet_hdr(&m_frame[0]),
          m_payload_hdr(&m_frame[packet_hdr_len()]),
          m_payload_data(&m_frame[header_len()]),
          m_budget_max(relay_budget(args.symbols, args.e1/100.0, args.e2/100.0,
                                    args.e3/100.0, args.e4/100.0)),
          m_credits(relay_credits(args.symbols, args.e1/100.0,
                                  args.e2/100.0, args.e3/100.0))
    {
        auto rp = std::bind(&recoder::recv_packet, this, std::placeholders::_1);
        auto sp = std::bind(&recoder::send_budget, this, std::placeholders::_1);
        auto t = std::bind(&recoder::timer, this);

        m_sock = socket_add(args.interface, rp, sp);
        fd_disable_write(m_sock);
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

    recoder rec(arguments);

    return EXIT_SUCCESS;
}

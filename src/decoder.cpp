#include <iostream>

#include "arguments.hpp"
#include "counter_avg.hpp"
#include "socket_loss.hpp"
#include "socket_io.hpp"
#include "packet_hdr_ethernet.hpp"
#include "payload_hdr_rlnc.hpp"
#include "payload_timer.hpp"
#include "payload_length.hpp"
#include "payload_data_rlnc.hpp"
#include "final_rlnc.hpp"
#include "codes.hpp"

typedef counter_avg<
        socket_loss_next_hop<
        socket_io<
        packet_hdr_ethernet<
        payload_hdr_rlnc<
        payload_length<
        payload_data_rlnc_decoder<kodo::decoder,
        final_coder
        >>>>>>> decoder_stack;

class decoder : public decoder_stack
{
    std::vector<uint8_t> m_frame;
    uint8_t *m_packet_hdr;
    uint8_t *m_payload_hdr;
    uint8_t *m_payload_data;
    int m_sock_out, m_sock_in;
    bool m_pending = false;
    size_t m_len;

    void send_done()
    {
        packet_hdr_add(m_sock_out, m_packet_hdr);
        payload_hdr_add_done(m_payload_hdr);
        socket_send(m_sock_out, m_packet_hdr, header_len());
    }

    void send_ack(size_t block_id)
    {
        packet_hdr_add_broadcast(m_sock_in, m_packet_hdr);
        payload_hdr_add_ack(m_payload_hdr, block_id);
        socket_send(m_sock_in, &m_frame[0], header_len());
    }


    void send_ack()
    {
        send_ack(payload_hdr_block_id());
    }

    void send_raw()
    {
        if (m_pending)
            return;

        packet_hdr_add(m_sock_out, m_packet_hdr);
        payload_hdr_add_raw(m_payload_hdr);
        m_len = payload_data_get(m_payload_data);

        socket_send(m_sock_out, m_packet_hdr, header_len() + m_len);
        m_pending = true;
    }

    void recv_raw(int sock)
    {
        int ret;

        while ((ret = socket_recv(sock, &m_frame[0], m_frame.size())) > 0) {
            if (!payload_hdr_is_ack(m_payload_hdr))
                continue;

            if (!packet_hdr_is_from_dest(m_packet_hdr))
                continue;

            m_pending = false;

            if (payload_data_is_decoded()) {
                send_ack();
                payload_data_reset();
                continue;
            }

            if (payload_data_is_partial_complete())
                send_raw();
        }
    }

    void recv_packet(int sock)
    {
        int ret;

        while ((ret = socket_recv(sock, &m_frame[0], m_frame.size())) >= 0) {
            if (ret == 0)
                continue;

            if (payload_hdr_is_done(m_payload_hdr)) {
                send_done();
                stop();
                return;
            }

            if (!payload_hdr_block_id_equal(m_payload_hdr)) {
                send_ack(payload_hdr_block_id(m_payload_hdr));
                continue;
            }

            if (payload_hdr_is_rec(m_payload_hdr))
                payload_data_read(m_payload_data);

            if (payload_hdr_is_enc(m_payload_hdr))
                payload_data_read(m_payload_data);

            if (payload_hdr_is_hlp(m_payload_hdr))
                payload_data_read(m_payload_data);

            if (payload_data_is_partial_complete())
                send_raw();
        }
    }

    void timer()
    {
        if (!m_pending)
            return;

        socket_send(m_sock_out, m_packet_hdr, header_len() + m_len);
    }


  public:
    decoder(struct arguments &args)
        : decoder_stack(args),
          m_frame(frame_len()),
          m_packet_hdr(&m_frame[0]),
          m_payload_hdr(&m_frame[packet_hdr_len()]),
          m_payload_data(&m_frame[header_len()])
    {
        auto rp = std::bind(&decoder::recv_packet, this, std::placeholders::_1);
        auto rr = std::bind(&decoder::recv_raw, this, std::placeholders::_1);

        m_sock_in = socket_add(args.interface, rp);
        m_sock_out = socket_add(args.raw, rr);
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

    decoder dec(arguments);

    return EXIT_SUCCESS;
}

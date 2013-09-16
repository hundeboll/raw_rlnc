#include <iostream>
#include <functional>
#include <vector>
#include <chrono>
#include <thread>

#include "arguments.hpp"
#include "counter_avg.hpp"
#include "socket_io.hpp"
#include "packet_hdr_ethernet.hpp"
#include "payload_hdr_rlnc.hpp"
#include "payload_length.hpp"
#include "payload_data_raw.hpp"
#include "final_rlnc.hpp"

typedef counter_avg<
        socket_io<
        packet_hdr_ethernet<
        payload_hdr_rlnc<
        payload_length<
        payload_data_raw<
        final_coder
        >>>>>> source_stack;

class source : public source_stack
{

    std::vector<uint8_t> m_frame;
    uint8_t *m_packet_hdr;
    uint8_t *m_payload_hdr;
    uint8_t *m_payload_data;
    int m_sock, m_fd;
    ssize_t m_len;

    void send_done()
    {
        packet_hdr_add(m_sock, m_packet_hdr);
        payload_hdr_add_done(m_payload_hdr);
        socket_send(m_sock, m_packet_hdr, header_len());
    }

    void read_file(int fd)
    {
        m_len = file_read(fd, m_payload_data, payload_data_len());

        if (m_len == 0) {
            stop();
            return;
        }

        if (m_len < 0)
            return;

        packet_hdr_add(m_sock, m_packet_hdr);
        payload_hdr_add_raw(m_payload_hdr);
        socket_send(m_sock, m_packet_hdr, header_len() + m_len);
        fd_disable_read(m_fd);
    }

    void recv_packet(int sock)
    {
        int ret;

        while ((ret = socket_recv(sock, &m_frame[0], m_frame.size())) >= 0) {
            if (ret == 0)
                continue;

            if (!packet_hdr_is_from_dest(m_packet_hdr))
                continue;

            if (!payload_hdr_is_ack(m_payload_hdr))
                continue;

            fd_enable_read(m_fd);
        }
    }

    void timer()
    {
        if (!payload_hdr_is_raw(m_payload_hdr))
            return;

        socket_send(m_sock, m_packet_hdr, header_len() + m_len);
    }

  public:
    source(const struct arguments &args)
        : source_stack(args),
          m_frame(frame_len()),
          m_packet_hdr(&m_frame[0]),
          m_payload_hdr(&m_frame[packet_hdr_len()]),
          m_payload_data(&m_frame[header_len()])
    {
        char stdin_str[] = "-";
        auto rf = std::bind(&source::read_file, this, std::placeholders::_1);
        auto rp = std::bind(&source::recv_packet, this, std::placeholders::_1);
        auto t  = std::bind(&source::timer, this);

        if (strncmp(args.input, stdin_str, sizeof(stdin_str)) == 0)
            m_fd = file_add(STDIN_FILENO, rf, NULL);
        else
            m_fd = file_add(args.input, rf, NULL);

        m_sock = socket_add(args.interface, rp);
        timer_add(t);

        start();
        send_done();
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

    if (!arguments.input) {
        std::cerr << "no input file supplied - please call with -f|--input" << std::endl;
        arguments_usage(argv[0]);
        return EXIT_FAILURE;
    }

    source source(arguments);

    return EXIT_SUCCESS;
}

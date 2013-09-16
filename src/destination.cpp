#include <iostream>
#include <functional>

#include "arguments.hpp"
#include "socket_io.hpp"
#include "packet_hdr_ethernet.hpp"
#include "payload_hdr_rlnc.hpp"
#include "payload_length.hpp"
#include "payload_data_raw.hpp"
#include "final_rlnc.hpp"
#include "arguments.hpp"

typedef socket_io<
        packet_hdr_ethernet<
        payload_hdr_rlnc<
        payload_length<
        payload_data_raw<
        final_coder
        >>>>> destination_stack;

class destination : public destination_stack
{
    std::vector<uint8_t> m_frame;
    uint8_t *m_packet_hdr;
    uint8_t *m_payload_hdr;
    uint8_t *m_payload_data;
    int m_fd;
    size_t m_packets = 0, m_current_seq = 0;
    bool m_verbose;

    void send_ack(int sock)
    {
        packet_hdr_add_reply(sock, m_packet_hdr);
        payload_hdr_add_ack(m_payload_hdr);
        socket_send(sock, &m_frame[0], header_len());
    }

    void read_packet(int sock)
    {
        size_t sequence;
        int len;

        while ((len = socket_recv(sock, &m_frame[0], m_frame.size())) > 0) {
            if (!packet_hdr_is_from_source(m_packet_hdr))
                continue;

            if (payload_hdr_is_done(m_payload_hdr))
                stop();

            if (!payload_hdr_is_raw(m_payload_hdr))
                continue;

            sequence = payload_hdr_sequence(m_payload_hdr);

            if (sequence == m_current_seq) {
                send_ack(sock);
                continue;
            }

            if (!m_packets++ && m_verbose) {
                std::cout << "connected: ";
                packet_hdr_print_address(m_packet_hdr);
                std::cout << std::endl;
            }

            file_write(m_fd, m_payload_data, len - header_len());
            send_ack(sock);
            m_current_seq = sequence;
        }
    }

  public:
    destination(const struct arguments &args)
        : destination_stack(args),
          m_frame(frame_len()),
          m_packet_hdr(&m_frame[0]),
          m_payload_hdr(&m_frame[packet_hdr_len()]),
          m_payload_data(&m_frame[header_len()])
    {
        char stdout_str[] = "-";
        auto r = std::bind(&destination::read_packet, this, std::placeholders::_1);

        socket_add(args.interface, r);

        if (strncmp(args.output, stdout_str, sizeof(stdout_str)) == 0) {
            m_verbose = false;
            m_fd = file_add(STDOUT_FILENO);
        } else {
            m_fd = file_add(args.output, mode_wronly);
            m_verbose = true;
        }

        start();
    }

    ~destination()
    {
        if (m_verbose)
            std::cout << "packets: " << m_packets << std::endl;
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

    if (!arguments.output) {
        std::cerr << "no output file supplied - please call with -o|--output" << std::endl;
        arguments_usage(argv[0]);
        return EXIT_FAILURE;
    }

    destination dest(arguments);

    return EXIT_SUCCESS;
}

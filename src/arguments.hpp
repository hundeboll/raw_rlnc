#pragma once

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <net/if.h>
#include <getopt.h>

struct arguments
{
    char interface[IFNAMSIZ] = "lo"; // -i --interface
    char raw[IFNAMSIZ] = "eth0";     // -j --raw
    char *destination = NULL;        // -d --destination
    char *source = NULL;             // -s --source
    char *helper = NULL;             // -h --helper
    char *relay = NULL;              // -r --relay
    char *input = NULL;              // -f --input
    char *output = NULL;             // -o --output
    size_t symbols = 100;            // -g --symbols
    size_t symbol_size = 1400;       // -k --symbol_size
    size_t max_tx = 500;             // -m --max_tx
    size_t e1 = 30;                  // -1 --e1
    size_t e2 = 30;                  // -2 --e2
    size_t e3 = 50;                  // -3 --e3
    size_t e4 = 50;                  // -4 --e4
    ssize_t timeout = 100;           // -t --timeout
    int broadcast = 0;               // -b --broadcast
    int help = 0;                    //    --help
} arguments;

static struct option options[] = {
    {"interface",   required_argument, NULL, 'i'},
    {"raw",         required_argument, NULL, 'j'},
    {"destination", required_argument, NULL, 'd'},
    {"source",      required_argument, NULL, 's'},
    {"helper",      required_argument, NULL, 'h'},
    {"relay",       required_argument, NULL, 'r'},
    {"input",       required_argument, NULL, 'f'},
    {"output",      required_argument, NULL, 'o'},
    {"symbols",     required_argument, NULL, 'g'},
    {"symbol_size", required_argument, NULL, 'k'},
    {"max_tx",      required_argument, NULL, 'm'},
    {"e1",          required_argument, NULL, '1'},
    {"e2",          required_argument, NULL, '2'},
    {"e3",          required_argument, NULL, '3'},
    {"e4",          required_argument, NULL, '4'},
    {"timeout",     required_argument, NULL, 't'},
    {"broadcast",   no_argument,       &arguments.broadcast, 1},
    {"help",        no_argument,       &arguments.help, 1},
    {0}
};

#define OPTIONS_NUM sizeof(options)/sizeof(options[0]) - 1

static const char *options_desc[OPTIONS_NUM] = {
    "interface to use for encoded packets",
    "interface to use for raw packets",
    "address of next hop",
    "address of previous hop",
    "address of previous helper",
    "address of previous relay",
    "file to read data from (- for stdin)",
    "file to write data to (- for stdout)",
    "number of symbols/packets per generations",
    "number of bytes per symbol/packet",
    "maximum number of packets to send",
    "percentage of packets to drop from source to helper",
    "percentage of packets to drop from helper to dest",
    "percentage of packets to drop from source to dest",
    "percentage of packets to drop from relay to dest",
    "milliseconds between timer callbacks",
    "send encoded packets using broadcast",
    "show this help",
};

static inline void arguments_usage(char *name)
{
    struct option *opt;
    const char *desc;
    size_t i;

    printf("Usage: %s [options]\n", name);
    printf("\n");
    printf("Available options:\n");
    for (i = 0; i < OPTIONS_NUM; ++i) {
        opt = options + i;
        desc = options_desc[i];

        if (!opt->flag)
            printf("  -%c,", (char)opt->val);
        else
            printf("     ");

        printf(" --%-20s %s\n", opt->name, desc);
    }
}

static inline int arguments_parse(int argc, char **argv)
{
    char c, opt_string[] = "i:j:d:s:h:f:o:g:k:r:m:1:2:3:4:t:";

    while ((c = getopt_long(argc, argv, opt_string, options, NULL)) != -1) {
        switch (c) {
            case 'i':
                strncpy(arguments.interface, optarg, IFNAMSIZ);
                break;
            case 'j':
                strncpy(arguments.raw, optarg, IFNAMSIZ);
                break;
            case 'd':
                arguments.destination = optarg;
                break;
            case 's':
                arguments.source = optarg;
                break;
            case 'h':
                arguments.helper = optarg;
                break;
            case 'r':
                arguments.relay = optarg;
                break;
            case 'f':
                arguments.input = optarg;
                break;
            case 'o':
                arguments.output = optarg;
                break;
            case 'g':
                arguments.symbols = atoi(optarg);
                break;
            case 'k':
                arguments.symbol_size = atoi(optarg);
                break;
            case 'm':
                arguments.max_tx = atoi(optarg);
                break;
            case '1':
                arguments.e1 = atoi(optarg);
                break;
            case '2':
                arguments.e2 = atoi(optarg);
                break;
            case '3':
                arguments.e3 = atoi(optarg);
                break;
            case '4':
                arguments.e4 = atoi(optarg);
                break;
            case 't':
                arguments.timeout = atoi(optarg);
                break;
            case '?':
                arguments_usage(argv[0]);
                return -1;
        }
    }

    for (size_t index = optind; index < argc; index++)
        printf ("Non-option argument %s\n", argv[index]);

    return 0;
}

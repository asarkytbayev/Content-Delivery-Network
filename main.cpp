/**
 * CS5700 Fall 2019
 * Project 5: Roll Your Own CDN
 * @author Azamat Sarkytbayev
 * NU ID: 001873077
 */
#include <cstdio>
#include <cstdlib>
#include <cstdbool>
#include "dnsserver.h"

int socket_fd;

int main(int argc, char *argv[]) {

    const char *port;
    const char *url;
    if (argc != 5) {
        fprintf(stderr, "usage: ./dnsserver -p <port> -n <name>\n");
        return EXIT_FAILURE;
    }
    port = argv[2]; // "cs5700cdn.example.com";
    url = argv[4]; // 40006

    socket_fd = setup_server(port);
    
    while (true) {
        process_query();
    }
    
    return EXIT_SUCCESS;
}

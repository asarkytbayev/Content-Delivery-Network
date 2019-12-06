/**
 * CS5700 Fall 2019
 * Project 5: Roll Your Own CDN
 * @author Azamat Sarkytbayev
 * NU ID: 001873077
 */

#include "dnsserver.h"

#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <sstream>
#include <fstream>
#include <cmath>
#include <limits>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

float ec2nodes_coords[NUMBER_EC2_NODES][2U] = {
    {39.0438, -77.4874}, 
    {45.5235f, -122.676f}, 
    {37.5665f, 126.978f}, 
    {43.6532f, -79.3832f}, 
    {50.1109f, 8.68213f} 
};

unsigned char ec2nodes_ip[NUMBER_EC2_NODES][4] = {
    {3U, 92U, 59U, 2U},
    {54U, 213U, 74U, 38U},
    {13U, 209U, 67U, 180U},
    {35U, 182U, 244U, 184U},
    {54U, 93U, 225U, 118U}
};

char ec2nodes_ipname[NUMBER_EC2_NODES][200] = {
    {"ec2-3-92-59-2.compute-1.amazonaws.com	North virginia: 39.0438, -77.4874. ip: \"3.92.59.2\""},
    {"ec2-54-213-74-38.us-west-2.compute.amazonaws.com	Oregon 45.5235, -122.676. ip: \"54.213.74.38\""},
    {"ec2-13-209-67-180.ap-northeast-2.compute.amazonaws.com	Seoul 37.5665, 126.978. ip: \"13.209.67.180\""},
    {"ec2-35-182-244-184.ca-central-1.compute.amazonaws.com	Canada 43.6532, -79.3832. ip: \"35.182.244.184\""}, 
    {"ec2-54-93-225-118.eu-central-1.compute.amazonaws.com	Frankfurt 50.1109, 8.68213. ip: \"54.93.225.118\""}   
};

unordered_map<string, string> ip_ec2node;

static void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// NOTE: reference: https://www.geeksforgeeks.org/haversine-formula-to-find-distance-between-two-points-on-a-sphere/
// This maths is a bit tricky
static float haversine(float lat1, float lon1, float lat2, float lon2)  {

    lat1 = lat1 / 180.0f * M_PI;
    lon1 = lon1 / 180.0f * M_PI;
    lat2 = lat2 / 180.0f * M_PI;
    lon2 = lon2 / 180.0f * M_PI;

    float central_angle = powf(sinf((lat2-lat1)/2), 2) + cosf(lat1)*cosf(lat2)*powf(sinf((lon2-lon1)/2),2);

    // distance = 2*r*inv_haversine(central_angle)
    float distance = 2.0f*EARTH_RADIUS_KM*asinf(sqrtf(central_angle));

    return distance;
} 

int setup_server(const char *port) {
    int socket_fd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int bytes_received;
    socklen_t addr_len;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(EXIT_FAILURE);
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (bind(socket_fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(socket_fd);
            perror("server: bind");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind socket\n");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(servinfo);

    return socket_fd;
}

void map2ec2node(unsigned char *answer_ip, const char *client_ip) {
    unsigned char ip_parts[4] = {};

    string ip24 = string(client_ip); // 24
    size_t last_dot = ip24.find_last_of(".");
    ip24 = ip24.substr(0, last_dot);

    if (ip_ec2node.count(ip24) == 1) { // if mapping exists
        
        string ec2node_ip = ip_ec2node.at(ip24);
        istringstream strm(ec2node_ip);
        string ip_part;
        unsigned char index = 0U;
        while (getline(strm, ip_part, '.')) {
            ip_parts[index++] = stoi(ip_part);
        }
        printf("mapping exists: directing client to %s\n", ec2node_ip.c_str());
    } else { // if mapping doesn't exist - query the api / if out of queries, return default
        // to query the api, call the python script
        // it saves lat, lng and ip to a file
        string command = "python ip_lookup.py " + string(client_ip);
        system(command.c_str());

        // read the file
        ifstream ip_file;
        ip_file.open("lat_lng.txt");
        // check if out of queries
        string line;
        getline(ip_file, line);
        // TODO - run out of queries
        if (line == "ERROR") {
            // - return virginia server ip by default
            printf("exceeded the quota\n");
            ip_parts[0] = 3U;
            ip_parts[1] = 92U;
            ip_parts[2] = 59U;
            ip_parts[3] = 2U;        
        } else { // query successful
            // read latitude
            float lat = atof(line.c_str());
            // read longitude
            getline(ip_file, line);
            float lng = atof(line.c_str());

            printf("%s: (lat, lng): (%.1f,%.1f)\n", client_ip, lat, lng);

            // use haversine formula to calculate the distance between 2 lat-lng points
            float min_distance = numeric_limits<float>::max();
            int index = 0;
            for (int i = 0; i < 5; ++i) {
                float current_distance = haversine(lat, lng, ec2nodes_coords[i][0], ec2nodes_coords[i][1]);
                printf("distance: %f\n", current_distance);
                if (min_distance > current_distance) {
                    min_distance = current_distance;
                    index = i;
                }
            }

            printf("new entry: %s\n", ec2nodes_ipname[index]);

            ip_parts[0] = ec2nodes_ip[index][0];
            ip_parts[1] = ec2nodes_ip[index][1];
            ip_parts[2] = ec2nodes_ip[index][2];
            ip_parts[3] = ec2nodes_ip[index][3];

            // update the map
            ip_ec2node[ip24] = to_string(ec2nodes_ip[index][0]) + "." + to_string(ec2nodes_ip[index][1]) + "." + to_string(ec2nodes_ip[index][2]) + "." + to_string(ec2nodes_ip[index][3]);
        }
    }

    printf("**************send out: %u.%u.%u.%d*************\n", ip_parts[0], ip_parts[1], ip_parts[2], ip_parts[3]);

    *(answer_ip) = ip_parts[0];
    *(answer_ip+1) = ip_parts[1];
    *(answer_ip+2) = ip_parts[2];
    *(answer_ip+3) = ip_parts[3];
}

size_t construct_response(unsigned char *buffer_in, unsigned char *buffer_out, char *client_ip) {
    memset(buffer_out, 0, MAX_DNS_SIZE);
    unsigned char *qname = (unsigned char *)&buffer_in[sizeof(DnsHeader)];
    memcpy(buffer_out, buffer_in, sizeof(DnsHeader) + strlen((char*)qname) + 1 + sizeof(DnsQuestion));

    // set the header
    DnsHeader *dns_header = (DnsHeader *)buffer_out;
    dns_header->qr = 1;
    dns_header->opcode = 0;
    dns_header->aa = 1;
    dns_header->tc = 0;
    dns_header->rd = 0;
    dns_header->ra = 0;
    dns_header->z = 0;
    dns_header->rcode = 0; // NXDOMAIN or 0 - no problem TODO
    dns_header->qdcount = htons(1);
    dns_header->ancount = htons(1);
    dns_header->nscount = htons(0);
    dns_header->arcount = htons(0);

    // set the question
    qname = (unsigned char *)&buffer_out[sizeof(DnsHeader)];
    DnsQuestion *dns_question = (DnsQuestion *)&buffer_out[sizeof(DnsHeader) + strlen((char*)qname) + 1];
    dns_question->qclass = htons(1);
    dns_question->qtype = htons(1);

    // set the response - compress the name
    unsigned char *name = (unsigned char *)&buffer_out[sizeof(DnsHeader) + strlen((char*)qname) + 1U + sizeof(DnsQuestion)];
    *(name) = 0xc0;
    *(name+1) = 0x0c;
    // set response fields
    DnsAnswer *dns_answer = (DnsAnswer *)&buffer_out[sizeof(DnsHeader) + strlen((char*)qname) + 1U + sizeof(DnsQuestion) + 2U];
    dns_answer->class_ = htons(1U);
    dns_answer->type = htons(1U);
    dns_answer->ttl = htonl(0x255);
    dns_answer->rdlength = htons(4U);
    // map to the best ec2 node
    unsigned char *answer_ip = (unsigned char *)&buffer_out[sizeof(DnsHeader) + strlen((char*)qname) + 1U + sizeof(DnsQuestion) + 2U + sizeof(DnsAnswer)];
    map2ec2node(answer_ip, client_ip);
    size_t message_len = sizeof(DnsHeader) + strlen((char*)qname) + 1U + sizeof(DnsQuestion) + 2U + sizeof(DnsAnswer) + 4U;
    
    return message_len;
}


void process_query(void) {

    fprintf(stderr, "\n\n\nserver: waiting to recvfrom...\n");

    char client_ip[INET_ADDRSTRLEN];

    unsigned char buffer_in[MAX_DNS_SIZE];
    memset(buffer_in, 0, MAX_DNS_SIZE);

    struct sockaddr_storage client_addr;
    socklen_t addr_len = sizeof(client_addr);

    ssize_t bytes_received;
    if ((bytes_received = recvfrom(socket_fd, buffer_in, 512-1 , 0, (struct sockaddr *)&client_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }

    fprintf(stderr, "server: got packet from %s\n",
            inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr), client_ip, sizeof(client_ip)));

    fprintf(stderr, "server: packet is %ld bytes long\n", bytes_received);
    // buf[bytes_received] = '\0';

    unsigned char buffer_out[MAX_DNS_SIZE];
    size_t message_len = construct_response(buffer_in, buffer_out, client_ip);


    // size_t message_len = sizeof(DnsHeader) + strlen((char*)qname) + 1U + sizeof(DnsQuestion);
    
    // size_t message_len = bytes_received;
    ssize_t bytes_sent;
    addr_len = sizeof(client_addr);
    if ((bytes_sent = sendto(socket_fd, buffer_out, message_len, 0, (struct sockaddr *)(&client_addr), addr_len)) == -1) {
        perror("server: sendto");
        exit(1);
    }

    printf("server: sent %ld bytes to %s\n\n\n", bytes_sent, client_ip);
}




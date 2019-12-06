/**
 * CS5700 Fall 2019
 * Project 5: Roll Your Own CDN
 * @author Azamat Sarkytbayev
 * NU ID: 001873077
 */

#ifndef DNS_SERVER
#define DNS_SERVER

#include <cstddef>
#include <unordered_map>
#include <string>

#include <arpa/inet.h>

using namespace std;

#define MAX_DNS_SIZE 512U
#define NUMBER_EC2_NODES 5U
#define EARTH_RADIUS_KM 6371.0f

extern int socket_fd;
// lat & lng & ip
// Origin: ec2-54-158-114-217.compute-1.amazonaws.com	North virginia 39.0438, -77.4874. ip: 54.158.114.217
// ec2-3-92-59-2.compute-1.amazonaws.com	North virginia: 39.0438, -77.4874. ip: "3.92.59.2"
// ec2-54-213-74-38.us-west-2.compute.amazonaws.com	Oregon 45.5235, -122.676. ip: "54.213.74.38"
// ec2-13-209-67-180.ap-northeast-2.compute.amazonaws.com	Seoul 37.5665, 126.978. ip: "13.209.67.180"
// ec2-35-182-244-184.ca-central-1.compute.amazonaws.com	Canada 43.6532, -79.3832. ip: "35.182.244.184" 
// ec2-54-93-225-118.eu-central-1.compute.amazonaws.com	Frankfurt 50.1109, 8.68213. ip: "54.93.225.118"
extern float ec2nodes_coords[NUMBER_EC2_NODES][2U];

extern unsigned char ec2nodes_ip[NUMBER_EC2_NODES][4];

extern char ec2nodes_ipname[NUMBER_EC2_NODES][200];

extern unordered_map<string, string> ip_ec2node;


struct DnsHeader {
    unsigned short id; // match up replies to queries

    unsigned char rd : 1; // 1 - recursion desired
    unsigned char tc : 1; // 1 - truncated -> exit => set to 0
    unsigned char aa : 1; // 1 - authoritative, 0 - otherwise
    unsigned char opcode : 4; // set to 0 - standard send_question
    unsigned char qr : 1; // 0 - send_question, 1 -response
    
    unsigned char rcode : 4; // 0 - good, 1 - format, 2 server failure, 3 - name error, 4 - not implemented, 5 - refused
    unsigned char z : 3; // set to 0
    // unsigned char cd : 1; // 
    // unsigned char ad : 1;
    // unsigned char z : 1;
    unsigned char ra : 1; // 1 - recursion available 
    
    unsigned short qdcount;
    unsigned short ancount;
    unsigned short nscount;
    unsigned short arcount;
};

typedef struct DnsHeader DnsHeader;

struct DnsQuestion {
    // char qname[128] = {};
    unsigned short qtype; // a records - host addresses
    unsigned short qclass; // internet address
};

typedef struct DnsQuestion DnsQuestion;

#pragma pack(push, 1)
struct DnsAnswer {
    // char name[128] = {};
    unsigned short type; // 0x0001 A record 0x0005 CNAME
    unsigned short class_; // 0x0001 internet address
    unsigned int ttl; // time to live
    unsigned short rdlength; // length of rdata
    // char rdata[128]; // if type 0x0001 - ip address 4 octets, 0x0005 CNAME - name of the alias
};
#pragma pack(pop)

typedef struct DnsAnswer DnsAnswer;

int setup_server(const char *port);

void map2ec2node(unsigned char *answer_ip, const char *client_ip);

size_t construct_response(unsigned char *buffer_in, unsigned char *buffer_out, const char *client_ip);

void process_query(void);

#endif /* DNS_SERVER */
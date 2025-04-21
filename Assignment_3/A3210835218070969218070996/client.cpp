#include <iostream>
#include <cstring>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>

// Configuration constants
#define SERVER_PORT 12345
#define MACHINE_IP "127.0.0.1"
#define CLIENT_PORT 54321
#define BUFFER_SIZE 65536
#define TIMEOUT 5
#define MAX_RECEIVE_ATTEMPTS 5


// Prints TCP flags for debugging purposes
void print_tcp_flags(struct tcphdr *tcp) {
    std::cout << "TCP Flags: SYN=" << tcp->syn
              << ", ACK=" << tcp->ack
              << ", FIN=" << tcp->fin
              << ", RST=" << tcp->rst
              << ", PSH=" << tcp->psh
              << ", SEQ=" << ntohl(tcp->seq) << std::endl;
}

// Constructs and sends a raw TCP packet
void send_packet(int sock, struct sockaddr_in *server_addr, uint32_t seq, uint32_t ack_seq, uint8_t syn, uint8_t ack) {
    char packet[sizeof(struct iphdr) + sizeof(struct tcphdr)];
    memset(packet, 0, sizeof(packet));

    struct iphdr *ip = (struct iphdr *)packet;
    struct tcphdr *tcp = (struct tcphdr *)(packet + sizeof(struct iphdr));

    // Populate IP header
    ip->ihl = 5;
    ip->version = 4;
    ip->tos = 0;
    ip->tot_len = htons(sizeof(packet));
    ip->id = htons(CLIENT_PORT);
    ip->frag_off = 0;
    ip->ttl = 64;
    ip->protocol = IPPROTO_TCP;
    ip->saddr = inet_addr(MACHINE_IP);
    ip->daddr = server_addr->sin_addr.s_addr;

    // Populate TCP header
    tcp->source = htons(CLIENT_PORT);
    tcp->dest = htons(SERVER_PORT);
    tcp->seq = htonl(seq);
    tcp->ack_seq = htonl(ack_seq);
    tcp->doff = 5;
    tcp->syn = syn;
    tcp->ack = ack;
    tcp->window = htons(8192);
    tcp->check = 0;
    // Send the packet
    if (sendto(sock, packet, sizeof(packet), 0, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
        perror("sendto() failed");
    } else {
        std::cout << "Sent packet with SYN=" << (int)syn << ", ACK=" << (int)ack << std::endl;
    }
}


int main() {
    std::cout << "Client started" << std::endl;

    // Create raw socket
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Enable custom IP header
    int one = 1;
    if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0) {
        perror("setsockopt() failed");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(MACHINE_IP);

    // Send SYN packet
    std::cout << "Sending SYN to server..." << std::endl;
    send_packet(sock, &server_addr, 200, 0, 1, 0);

    // Set socket timeout
    struct timeval timeout;
    timeout.tv_sec = TIMEOUT;
    timeout.tv_usec = 0;
    
    char buffer[BUFFER_SIZE];
    int attempts = 0;
    
    while (attempts < MAX_RECEIVE_ATTEMPTS) {
        if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) < 0) {
            std::cerr << "Error setting SO_RCVTIMEO: " << strerror(errno) << std::endl;
            close(sock);
            return -1;
        }
        // Wait for SYN-ACK packet
        int data_size = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&server_addr, &addr_len);
        if (data_size < 0) {
            std::cerr << "Timeout. Didn't receiving SYN-ACK. Resending SYN. Attempt:" << ++attempts << "/5" << std::endl;
            send_packet(sock, &server_addr, 200, 0, 1, 0);
            continue;
        }
        struct iphdr *ip = (struct iphdr *)buffer;
        struct tcphdr *tcp = (struct tcphdr *)(buffer + (ip->ihl * 4));

        // Process packets for the correct destination port
        if (ntohs(tcp->dest) != CLIENT_PORT) continue;

        print_tcp_flags(tcp);

        // Check for SYN-ACK
        if (tcp->syn == 1 && tcp->ack == 1 && ntohl(tcp->seq) == 400) {
            std::cout << "Received SYN-ACK from server" << std::endl;
            // Send ACK packet
            std::cout << "Sending ACK to server..." << std::endl;
            send_packet(sock, &server_addr, 600, ntohl(tcp->seq) + 1, 0, 1);
            std::cout << "ACK sent" << std::endl;
            break;
        } else {
            std::cerr << "Unexpected packet received" << std::endl;
            break;
        }
    }std::cout << "Server seems to be unreachable or shut down." << std::endl;
    // Close socket
    close(sock);
    std::cout << "Client exiting" << std::endl;
    return 0;
}
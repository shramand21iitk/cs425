#include <iostream>
#include <cstring>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>

// Define server and client settings
#define SERVER_PORT 12345   // Server is listening on port 12345
#define MACHINE_IP "127.0.0.1"  // Localhost IP address
#define CLIENT_PORT 54321   // Arbitrary client port for outgoing packets
#define BUFFER_SIZE 65536   // Buffer size for receiving packets

// Function to print TCP flags for debugging
void print_tcp_flags(struct tcphdr *tcp) {
    std::cout << "[+] TCP Flags: "
              << " SYN: " << tcp->syn
              << " ACK: " << tcp->ack
              << " FIN: " << tcp->fin
              << " RST: " << tcp->rst
              << " PSH: " << tcp->psh
              << " SEQ: " << ntohl(tcp->seq) << std::endl;
}

// Function to construct and send a raw TCP packet
void send_packet(int sock, struct sockaddr_in *server_addr, uint32_t seq, uint32_t ack_seq, uint8_t syn, uint8_t ack) {
    // Allocate memory for the packet (IP header + TCP header)
    char packet[sizeof(struct iphdr) + sizeof(struct tcphdr)];
    memset(packet, 0, sizeof(packet));  // Clear the packet memory

    // Pointers to the IP and TCP headers within the packet
    struct iphdr *ip = (struct iphdr *)packet;
    struct tcphdr *tcp = (struct tcphdr *)(packet + sizeof(struct iphdr));

    // Fill IP header
    ip->ihl = 5;  // IP header length (5 words = 20 bytes)
    ip->version = 4;  // IPv4
    ip->tos = 0;  // Type of service
    ip->tot_len = htons(sizeof(packet));  // Total length of the packet
    ip->id = htons(CLIENT_PORT);  // Identification field
    ip->frag_off = 0;  // Fragment offset
    ip->ttl = 64;  // Time to live
    ip->protocol = IPPROTO_TCP;  // Protocol (TCP)
    ip->saddr = inet_addr(MACHINE_IP);  // Source IP address
    ip->daddr = server_addr->sin_addr.s_addr;  // Destination IP address

    // Fill TCP header
    tcp->source = htons(CLIENT_PORT);  // Source port
    tcp->dest = htons(SERVER_PORT);  // Destination port
    tcp->seq = htonl(seq);  // Sequence number
    tcp->ack_seq = htonl(ack_seq);  // Acknowledgment number
    tcp->doff = 5;  // Data offset (5 words = 20 bytes)
    tcp->syn = syn;  // SYN flag
    tcp->ack = ack;  // ACK flag
    tcp->window = htons(8192);  // Window size
    tcp->check = 0;  // Checksum (kernel will compute it)

    // Send the packet
    if (sendto(sock, packet, sizeof(packet), 0, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
        perror("sendto() failed");
    } else {
        std::cout << "[+] Sent packet with SYN=" << (int)syn << ", ACK=" << (int)ack << std::endl;
    }
}

int main() {
    std::cout << "[+] Client started" << std::endl;

    // Create a raw socket
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Enable IP header inclusion
    int one = 1;
    if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0) {
        perror("setsockopt() failed");
        exit(EXIT_FAILURE);
    }

    // Define the server address
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(MACHINE_IP);

    // Step 1: Send SYN packet to the server
    std::cout << "[+] Sending SYN to server..." << std::endl;
    send_packet(sock, &server_addr, 200, 0, 1, 0);

    // Buffer to receive packets
    char buffer[BUFFER_SIZE];

    while (true) {
        // Step 2: Wait for SYN-ACK packet from the server
        int data_size = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&server_addr, &addr_len);
        if (data_size < 0) {
            perror("Packet reception failed");
            continue;
        }

        // Parse the received packet
        struct iphdr *ip = (struct iphdr *)buffer;
        struct tcphdr *tcp = (struct tcphdr *)(buffer + (ip->ihl * 4));

        // Only process packets for the correct destination port
        if (ntohs(tcp->dest) != CLIENT_PORT) continue;

        // Print TCP flags for debugging
        print_tcp_flags(tcp);

        // Check if the packet is a SYN-ACK
        if (tcp->syn == 1 && tcp->ack == 1 && ntohl(tcp->seq) == 400) {
            std::cout << "[+] Received SYN-ACK from server" << std::endl;

            // Step 3: Send ACK packet to the server
            std::cout << "[+] Sending ACK to server..." << std::endl;
            send_packet(sock, &server_addr, 600, ntohl(tcp->seq) + 1, 0, 1);
            std::cout << "[+] Handshake complete, ACK sent" << std::endl;
            break;
        }
    }

    // Close the socket
    close(sock);

    std::cout << "[+] Client exiting" << std::endl;
    return 0;
}
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>

// Define server and client settings (adjust these as needed)
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 12345   // Server is listening on port 12345
#define CLIENT_IP "127.0.0.1"
#define CLIENT_PORT 54321   // Arbitrary client port for our outgoing packets
#define BUFFER_SIZE 65536

void print_tcp_flags(struct tcphdr *tcp) {
    std::cout << "[+] TCP Flags: "
              << " SYN: " << tcp->syn
              << " ACK: " << tcp->ack
              << " FIN: " << tcp->fin
              << " RST: " << tcp->rst
              << " PSH: " << tcp->psh
              << " SEQ: " << ntohl(tcp->seq) << std::endl;
}


void send_packet(int sock, struct sockaddr_in *server_addr, uint32_t seq, uint32_t ack_seq, uint8_t syn, uint8_t ack) {
   
    char packet[sizeof(struct iphdr) + sizeof(struct tcphdr)];
    memset(packet, 0, sizeof(packet));

    struct iphdr *ip = (struct iphdr *)packet;
    struct tcphdr *tcp = (struct tcphdr *)(packet + sizeof(struct iphdr));

     // Fill IP header
     ip->ihl = 5;
     ip->version = 4;
     ip->tos = 0;
     ip->tot_len = htons(sizeof(packet));
     ip->id = htons(CLIENT_PORT);
     ip->frag_off = 0;
     ip->ttl = 64;
     ip->protocol = IPPROTO_TCP;
     ip->saddr = inet_addr(CLIENT_IP);
     ip->daddr = server_addr->sin_addr.s_addr; 

     // Fill TCP header
     tcp->source = htons(CLIENT_PORT);
     tcp->dest = htons(SERVER_PORT);
     tcp->seq = htonl(seq);
     tcp->ack_seq = htonl(ack_seq); 
     tcp->doff = 5;
     tcp->syn = syn;
     tcp->ack = ack;
     tcp->window = htons(8192);
     tcp->check = 0;  // Kernel will compute the checksum
 
     // Send packet
     if (sendto(sock, packet, sizeof(packet), 0, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
         perror("sendto() failed");
     } else {
         std::cout << "[+] Sent SYN "<< std::endl;
     }

    }



int main(){
    std::cout << "Client started" << std::endl;
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
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    uint32_t client_syn_seq = 200;
    send_packet(sock, &server_addr, client_syn_seq, 400, 1, 0);

    char buffer[65536];
    while (true) {
        int data_size = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&server_addr, &addr_len);
        if (data_size < 0) {
            perror("Packet reception failed");
            continue;
        }

        struct iphdr *ip = (struct iphdr *)buffer;
        struct tcphdr *tcp = (struct tcphdr *)(buffer + (ip->ihl * 4));

        // Only process packets for the correct destination port
        if (ntohs(tcp->dest) != CLIENT_PORT) continue;

        print_tcp_flags(tcp);

        if (tcp->syn == 1 && tcp->ack == 1 && ntohl(tcp->seq) == 400) {
            std::cout << "[+] Received SYN_ACK from " << inet_ntoa(server_addr.sin_addr) << std::endl;
            send_packet(sock, &server_addr, 600, ntohl(tcp->seq) + 1, 0, 1);
            std::cout << "[+] Sent ACK" << std::endl;
            break;
        }
    }

    close(sock);

//create raw socket, and set socket options after which we will create a sockaddr_in structure to hold the server address

// send SYN packet to the server
 //receive SYN-ACK packet from server
    return 0;
}
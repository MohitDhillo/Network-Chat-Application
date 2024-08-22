#include <iostream>
#include <winsock2.h>
#include <cstring>
#include <string>
#include <vector>
#include <iomanip>
#include <sstream>
#include <map>
#include <string>
using namespace std;
// DNS header structure
struct DNSHeader
{
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
};

// DNS question structure
struct DNSQuestion
{
    uint16_t qtype;
    uint16_t qclass;
};

// DNS answer structure for Type A (IPv4) records
struct DNSAnswerA
{
    uint16_t type;
    uint16_t _class;
    uint32_t ttl;
    uint16_t rdlength;
    uint32_t address;
};
map<string, vector<string>> cache;

int main()
{
    char domain[256];
    std::cout << "Enter a domain name: ";
    std::cin >> domain;
    if (cache.count(domain) == 1)
    {
        cout << "from cache response:\n";
        for (auto h : cache[domain])
        {
            cout << h << endl;
        }
    }
    else
    {

        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        {
            std::cerr << "WSAStartup failed." << std::endl;
            return 1;
        }

        // Create a UDP socket
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock == -1)
        {
            perror("socket");
            WSACleanup();
            return 1;
        }

        // Define the DNS server (Google's public DNS)
        struct sockaddr_in dns_server;
        dns_server.sin_family = AF_INET;
        dns_server.sin_port = htons(53);
        dns_server.sin_addr.s_addr = inet_addr("8.8.8.8");

        // Prepare the DNS query
        struct DNSHeader header;
        header.id = htons(1234);
        header.flags = htons(0x0100); // Standard query with recursion
        header.qdcount = htons(1);    // One question in the question section
        header.ancount = 0;
        header.nscount = 0;
        header.arcount = 0;

        struct DNSQuestion question;
        question.qtype = htons(1);  // Type A (IPv4 address)
        question.qclass = htons(1); // IN class

        // Construct the DNS query packet
        char packet[512];
        int offset = 0;

        memcpy(packet + offset, &header, sizeof(struct DNSHeader));
        offset += sizeof(struct DNSHeader);

        // Encode the domain name
        char *token = strtok(domain, ".");
        while (token != nullptr)
        {
            int len = strlen(token);
            packet[offset++] = len;
            strncpy(packet + offset, token, len);
            offset += len;
            token = strtok(nullptr, ".");
        }
        packet[offset++] = 0; // Null-terminated label

        memcpy(packet + offset, &question, sizeof(struct DNSQuestion));
        offset += sizeof(struct DNSQuestion);

        // Send the DNS query to the server
        if (sendto(sock, packet, offset, 0, (struct sockaddr *)&dns_server, sizeof(dns_server)) == -1)
        {
            perror("sendto");
            closesocket(sock);
            WSACleanup();
            return 1;
        }

        // Receive the DNS response
        char response[512];
        int response_len = recv(sock, response, sizeof(response), 0);
        if (response_len == -1)
        {
            perror("recv");
            closesocket(sock);
            WSACleanup();
            return 1;
        }

        // Parse the DNS header to get ancount
        struct DNSHeader *dnsHeader = reinterpret_cast<struct DNSHeader *>(response);
        uint16_t ancount = ntohs(dnsHeader->ancount);

        // Print the number of answers (ancount)
        std::cout << "Number of Answers: " << ancount << std::endl;

        // Parse the DNS response from the end to find the IP address(es)
        cache[domain] = {};
        offset = response_len - 4; // Start from the end of the response
        for (int j = 0; j < ancount; ++j)
        {
            if (offset >= 0)
            {
                std::stringstream ipStream;
                for (int i = 0; i < 4; ++i)
                {
                    ipStream << static_cast<int>(static_cast<unsigned char>(response[offset + i]));
                    if (i != 3)
                    {
                        ipStream << ".";
                    }
                }

                std::string ipAddress = ipStream.str();
                cache[domain].push_back(ipAddress);
                std::cout << "IPv4 Address " << j + 1 << ": " << ipAddress << std::endl;

                offset -= 16; // Move to the previous IPv4 address
            }
            else
            {
                std::cerr << "Invalid response length." << std::endl;
                break;
            }
        }

        closesocket(sock);
        WSACleanup();
    }
    return 0;
}
/**
 *@file DNS.hpp
 * @author Muhd Syamim (Syazam33@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-04-06
 *
 * @copyright Copyright (c) 2024
 *
 */

#ifndef DNS
#define DNS

#include <cassert>
#include <cerrno>
#include <cstring>

#include <lwip/ip_addr.h>
#include <lwip/udp.h>

#define PORT_DNS_SERVER 53

#define MAX_DNS_MSG_SIZE 300

typedef struct DNS_HEADER_T_ {
    uint16_t id;
    uint16_t flags;
    uint16_t question_count;
    uint16_t answer_record_count;
    uint16_t authority_record_count;
    uint16_t additional_record_count;
} DNS_HEADER_T;

class DNS_SERVER {
public:
    int SocketNewdatagram(void* cb_data, udp_recv_fn cb_udp_recv);
    int SocketBind(uint32_t ip, uint16_t port);
    int SocketSendTo(const void* buf, size_t len, const ip_addr_t* dest, uint16_t port);

    void SocketFree();

    static void Process(void* arg, struct udp_pcb* upcb, struct pbuf* p, const ip_addr_t* src_addr, u16_t src_port);

    DNS_SERVER(ip_addr_t* ip);
    ~DNS_SERVER();

private:
    ip_addr_t ipAddress;
    struct udp_pcb* udp;
};

#endif /* DNS */

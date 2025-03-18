/**
 *@file DNS.cpp
 * @author Muhd Syamim (Syazam33@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-03-31
 *
 * @copyright Copyright (c) 2024
 *
 */

#ifdef DEBUG_DNS
#define DEBUG_WRITE printf
#else
#define DEBUG_WRITE //
#endif

#define ERROR_WRITE printf

#include <lwipopts.h>
#include <DNS.hpp>

DNS_SERVER::DNS_SERVER(ip_addr_t* ip) {
    if (SocketNewdatagram(this, Process) != ERR_OK) {
        ERROR_WRITE("DNS: Failed to Start\n");
        return;
    }

    if (SocketBind(0, PORT_DNS_SERVER) != ERR_OK) {
        ERROR_WRITE("DNS: Failed to Bind\n");
        return;
    }

    ip_addr_copy(ipAddress, *ip);
    DEBUG_WRITE("DNS: Listening on port %d\n", PORT_DNS_SERVER);
}

DNS_SERVER::~DNS_SERVER() {
    SocketFree();
}

int DNS_SERVER::SocketNewdatagram(void* cb_data, udp_recv_fn cb_udp_recv) {
    udp = udp_new();

    if (udp == NULL) return -ENOMEM;

    udp_recv(udp, cb_udp_recv, (void*)cb_data);

    return ERR_OK;
}

int DNS_SERVER::SocketBind(uint32_t ip, uint16_t port) {
    ip_addr_t addr;
    IP4_ADDR(&addr, ip >> 24 & 0xFF, ip >> 16 & 0xFF, ip >> 8 & 0xFF, ip & 0xFF);

    err_t err = udp_bind(udp, &addr, port);
    if (err != ERR_OK) {
        ERROR_WRITE("DNS: Failed to bind to port %u: %d", port, err);
        assert(false);
    }

    return err;
}

int DNS_SERVER::SocketSendTo(const void* buf, size_t len, const ip_addr_t* dest, uint16_t port) {
    if (len > 0xFFFF) len = 0xFFFF;

    struct pbuf* p = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
    if (p == NULL) {
        ERROR_WRITE("DNS: Failed to send message out of memory\n");
        return -ENOMEM;
    }

    memcpy(p->payload, buf, len);
    err_t err = udp_sendto(udp, p, dest, port);

    pbuf_free(p);

    if (err != ERR_OK) {
        ERROR_WRITE("DNS: Failed to send message %d\n", err);
        return err;
    }

    return len;
}

void DNS_SERVER::SocketFree() {
    if (udp == NULL) return;

    udp_remove(udp);
    udp = NULL;
}


void DNS_SERVER::Process(void* arg, struct udp_pcb* upcb, struct pbuf* p, const ip_addr_t* src_addr, u16_t src_port) {
    DNS_SERVER* d = reinterpret_cast<DNS_SERVER*>(arg);
    DEBUG_WRITE("DNS Process %u\n", p->tot_len);

    uint8_t msg[MAX_DNS_MSG_SIZE];
    DNS_HEADER_T* header = reinterpret_cast<DNS_HEADER_T*>(msg);

    size_t len;

    uint16_t flags, question_count;
    const uint8_t* question_ptr_start, * question_ptr_end, * question_ptr;
    uint8_t* answer_ptr;

    len = pbuf_copy_partial(p, msg, sizeof(msg), 0);
    if (len < sizeof(header)) {
        goto ignore_request;
    }

    flags = lwip_ntohs(header->flags);
    question_count = lwip_ntohs(header->question_count);

    DEBUG_WRITE("Length %d\n", len);
    DEBUG_WRITE("DNS Flags 0x%x\n", flags);
    DEBUG_WRITE("DNS Question Count 0x%x\n", question_count);

    // flags from rfc1035
    // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    // |QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |
    // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

    if (((flags >> 15) & 0x01) != 0) {
        DEBUG_WRITE("Ignoring non-query\n");
        goto ignore_request;
    }

    if (((flags >> 11) & 0x0F) != 0) {
        DEBUG_WRITE("Ignoring non-standard query\n");
        goto ignore_request;
    }

    if (question_count < 1) {
        DEBUG_WRITE("Invalid Question Count\n");
        goto ignore_request;
    }

#pragma region Print Question
    DEBUG_WRITE("Question: ");
    question_ptr_start = msg + sizeof(header);
    question_ptr_end = msg + len;
    question_ptr = question_ptr_start;
    while (question_ptr < question_ptr_end) {
        if (*question_ptr == 0) {
            question_ptr++;
            break;
        } else {
            if (question_ptr > question_ptr_start) DEBUG_WRITE(".");

            int label_len = *question_ptr++;
            if (label_len > 63) {
                DEBUG_WRITE("Invalid label\n");
                goto ignore_request;
            }

            DEBUG_WRITE("%.*s", label_len, question_ptr);
            question_ptr += label_len;
        }
    }
    DEBUG_WRITE("\n");
#pragma endregion

    if (question_ptr - question_ptr_start > 255) {
        DEBUG_WRITE("Invalid question length\n");
        goto ignore_request;
    }

    // Skip QNAME and QTYPE
    question_ptr += 4;

#pragma region Generate Answer
    answer_ptr = msg + (question_ptr - msg);
    *answer_ptr++ = 0xC0;                   // Pointer
    *answer_ptr++ = question_ptr - msg;     // Pointer to Question

    *answer_ptr++ = 0;
    *answer_ptr++ = 1;                      // Host Address

    *answer_ptr++ = 0;
    *answer_ptr++ = 1;                      // Internet Class

    *answer_ptr++ = 0;
    *answer_ptr++ = 0;
    *answer_ptr++ = 0;
    *answer_ptr++ = 60;                     // TTL 60s

    *answer_ptr++ = 0;
    *answer_ptr++ = 4;                      // Length
    memcpy(answer_ptr, &d->ipAddress, 4);   // Use our address
    answer_ptr += 4;

    header->flags = lwip_htons(
        0x1 << 15 | // QR = Response
        0x1 << 10 | // AA = Authoritive
        0x1 << 7    // RA = Authenticated
    );
    header->question_count = lwip_htons(1);
    header->answer_record_count = lwip_htons(1);
    header->authority_record_count = 0;
    header->additional_record_count = 0;
#pragma endregion

    DEBUG_WRITE("Sending %d byte reply to %s:%d\n", answer_ptr - msg, ipaddr_ntoa(src_addr), src_port);
    d->SocketSendTo(&msg, answer_ptr - msg, src_addr, src_port);

ignore_request:
    pbuf_free(p);
}
/**
 *@file TCP.hpp
 * @author Muhd Syamim (Syazam33@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-03-31
 *
 * @copyright Copyright (c) 2024
 *
 */

#ifndef TCP
#define TCP

#define TCP_PORT 80

#include <pico/cyw43_arch.h>

#include <lwip/tcp.h>

typedef struct TCP_CONNECT_STATE_T_ {
    struct tcp_pcb* pcb;
    int sent_len;
    char header[128];
    char result[256];
    int header_len;
    int result_len;
    ip_addr_t* gw;
} TCP_CONNECT_STATE_T;

class TCP_SERVER {
public:
    static err_t Poll(void* arg, struct tcp_pcb* pcb);
    static err_t Sent(void* arg, struct tcp_pcb* pcb, u16_t len);
    static err_t Accept(void* arg, struct tcp_pcb* client_pcb, err_t err);
    static err_t Receive(void* arg, struct tcp_pcb* pcb, struct pbuf* p, err_t err);
    static err_t CloseClient(TCP_CONNECT_STATE_T* con_state, struct tcp_pcb* client_pcb, err_t close_err);

    static void Error(void* arg, err_t err);
    static int Content(const char* request, const char* params, char* result, size_t max_result_len);

    TCP_SERVER(const char* ap_name);
    ~TCP_SERVER();

public:
    struct tcp_pcb* server_pcb;
    bool complete;
    ip_addr_t gw;
    async_context* context;
};

#endif /* TCP */

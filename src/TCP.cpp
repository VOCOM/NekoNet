/**
 *@file TCP.cpp
 * @author Muhd Syamim (Syazam33@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-03-31
 *
 * @copyright Copyright (c) 2024
 *
 */

#ifdef DEBUG_TCP
#define DEBUG_WRITE printf
#else
#define DEBUG_WRITE //
#endif

#define ERROR_WRITE printf

#define POLL_TIME_S 5
#define HTTP_GET "GET"
#define HTTP_RESPONSE_HEADER "HTTP/1.1 %d OK\nContent-Length: %d\nContent-Type: text/html; charset=utf-8\nConnection: close\n\n"
#define HTTP_RESPONSE_REDIRECT "HTTP/1.1 302 Redirect\nLocation: http://%s/NekoNet\n\n"
#define HTTP_BODY "<html><body><h1>Hello from Pico W.</h1></body></html>"

#include <cassert>

#include <lwipopts.h>
#include <TCP.hpp>

err_t TCP_SERVER::Poll(void* arg, tcp_pcb* pcb) {
    TCP_CONNECT_STATE_T* connection = reinterpret_cast<TCP_CONNECT_STATE_T*>(arg);
    DEBUG_WRITE("TCP: Polling\n");
    return CloseClient(connection, pcb, ERR_OK);
}

err_t TCP_SERVER::Sent(void* arg, tcp_pcb* pcb, u16_t len) {
    TCP_CONNECT_STATE_T* connection = reinterpret_cast<TCP_CONNECT_STATE_T*>(arg);

    DEBUG_WRITE("TCP: Server Sent %u\n", len);

    connection->sent_len = len;
    if (connection->sent_len >= connection->header_len + connection->result_len) {
        DEBUG_WRITE("TCP: All Done\n");
        return CloseClient(connection, pcb, ERR_OK);
    }

    return ERR_OK;
}

err_t TCP_SERVER::Accept(void* arg, tcp_pcb* client_pcb, err_t err) {
    TCP_SERVER* state = reinterpret_cast<TCP_SERVER*>(arg);

    if (err != ERR_OK || client_pcb == nullptr) {
        DEBUG_WRITE("TCP: Failure in accept\n");
        return ERR_VAL;
    }
    DEBUG_WRITE("TCP: Client Connected\n");

    TCP_CONNECT_STATE_T* connection = reinterpret_cast<TCP_CONNECT_STATE_T*>(calloc(1, sizeof(TCP_CONNECT_STATE_T)));
    if (connection == nullptr) {
        DEBUG_WRITE("TCP: Failed to allocate connection state\n");
        return ERR_MEM;
    }
    connection->pcb = client_pcb;
    connection->gw = &state->gw;

    tcp_arg(client_pcb, connection);
    tcp_sent(client_pcb, Sent);
    tcp_recv(client_pcb, Receive);
    tcp_poll(client_pcb, Poll, POLL_TIME_S * 2);
    tcp_err(client_pcb, Error);

    return ERR_OK;
}

err_t TCP_SERVER::Receive(void* arg, tcp_pcb* pcb, pbuf* p, err_t err) {
    TCP_CONNECT_STATE_T* connection = reinterpret_cast<TCP_CONNECT_STATE_T*>(arg);
    if (p == nullptr) {
        DEBUG_WRITE("TCP: Connection Closed\n");
        return CloseClient(connection, pcb, ERR_OK);
    }
    assert(connection && connection->pcb == pcb);

    if (p->tot_len > 0) {
        DEBUG_WRITE("TCP: Receive %d err %d\n", p->tot_len, err);

#ifdef DUMP_DATA
        for (struct pbuf* q = p; q != nullptr; q = q->next) {
            DEBUG_WRITE("\nDumping data: %.*s\n", q->len, q->payload);
        }
#endif

        // Copy request into buffer
        pbuf_copy_partial(p, connection->header, p->tot_len > sizeof(connection->header) ? sizeof(connection->header) - 1 : p->tot_len, 0);

        // Handle GET request
        if (strncmp(HTTP_GET, connection->header, sizeof(HTTP_GET) - 1) == 0) {
            char* request = connection->header + sizeof(HTTP_GET);
            char* params = strchr(request, '?');
            if (params != nullptr) {
                DEBUG_WRITE("GET: %c", params);
            }

            // Generate content
            connection->result_len = Content(request, params, connection->result, sizeof(connection->result));
            DEBUG_WRITE("TCP Request: %s?%s\n", request, params);
            DEBUG_WRITE("TCP Result: %d\n", connection->result);

            // Check for buffer overflow
            if (connection->result_len > sizeof(connection->result) - 1) {
                DEBUG_WRITE("TCP: Too much result data %d\n", connection->result_len);
                return CloseClient(connection, pcb, ERR_CLSD);
            }

            //Generate webpage
            if (connection->result_len > 0) {
                connection->header_len = snprintf(connection->header, sizeof(connection->header), HTTP_RESPONSE_HEADER,
                    200, connection->result_len);
                if (connection->header_len > sizeof(connection->header) - 1) {
                    DEBUG_WRITE("TCP: Too much header data %d\n", connection->header_len);
                    return CloseClient(connection, pcb, ERR_CLSD);
                }
            } else {
                // Send redirect
                connection->header_len = snprintf(connection->header, sizeof(connection->header), HTTP_RESPONSE_REDIRECT,
                    ipaddr_ntoa(connection->gw));
                DEBUG_WRITE("TCP: Sending redirect %s", connection->header);
            }

            // Send header to client
            connection->sent_len = 0;
            err_t err = tcp_write(pcb, connection->header, connection->header_len, 0);
            if (err != ERR_OK) {
                DEBUG_WRITE("TCP: Failed to write header data %d\n", err);
                return CloseClient(connection, pcb, err);
            }

            // Send body to client
            if (connection->result_len > 0) {
                err_t err = tcp_write(pcb, connection->result, connection->result_len, 0);
                if (err != ERR_OK) {
                    DEBUG_WRITE("TCP: Failed to write result data %d\n", err);
                    return CloseClient(connection, pcb, err);
                }
            }
        }
        tcp_recved(pcb, p->tot_len);
    }
    pbuf_free(p);
    return ERR_OK;
}

err_t TCP_SERVER::CloseClient(TCP_CONNECT_STATE_T* con_state, tcp_pcb* client_pcb, err_t close_err) {
    if (client_pcb != nullptr) {
        assert(con_state != NULL && con_state->pcb == client_pcb);
        tcp_arg(client_pcb, NULL);
        tcp_poll(client_pcb, NULL, 0);
        tcp_sent(client_pcb, NULL);
        tcp_recv(client_pcb, NULL);
        tcp_err(client_pcb, NULL);

        err_t err = tcp_close(client_pcb);
        if (err != ERR_OK) {
            DEBUG_WRITE("TCP: Close failed %d, calling abort\n", err);
            tcp_abort(client_pcb);
            close_err = ERR_ABRT;
        }

        if (con_state != nullptr) {
            free(con_state);
        }
    }

    return close_err;
}

void TCP_SERVER::Error(void* arg, err_t err) {
    if (err == ERR_ABRT) return;

    DEBUG_WRITE("TCP: Client Error %d\n", err);

    TCP_CONNECT_STATE_T* con_state = reinterpret_cast<TCP_CONNECT_STATE_T*>(arg);
    CloseClient(con_state, con_state->pcb, err);
}

int TCP_SERVER::Content(const char* request, const char* params, char* result, size_t max_result_len) {
    int len = snprintf(result, max_result_len, HTTP_BODY);
    return len;
}

TCP_SERVER::TCP_SERVER(const char* ap_name) {
    DEBUG_WRITE("TCP: Starting server on port %d\n", TCP_PORT);

    struct tcp_pcb* pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (pcb == nullptr) {
        ERROR_WRITE("TCP: Failed to create pcb\n");
        assert(false);
    }

    err_t err = tcp_bind(pcb, IP_ANY_TYPE, TCP_PORT);
    if (err != ERR_OK) {
        ERROR_WRITE("TCP: Failed to bind to port %d\n", TCP_PORT);
        assert(false);
    }

    server_pcb = tcp_listen_with_backlog(pcb, 1);
    if (server_pcb == nullptr) {
        ERROR_WRITE("TCP: Failed to listen\n");
        if (pcb != nullptr) tcp_close(pcb);
        assert(false);
    }

    tcp_arg(server_pcb, this);
    tcp_accept(server_pcb, Accept);

    DEBUG_WRITE("Try connecting to '%s'\n", ap_name);
}

TCP_SERVER::~TCP_SERVER() {
    if (server_pcb == nullptr) return;

    tcp_arg(server_pcb, NULL);
    tcp_close(server_pcb);
    server_pcb = NULL;
}

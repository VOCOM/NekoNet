/**
 *@file DHCP.cpp
 * @author Muhd Syamim (Syazam33@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-03-27
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <DHCP.hpp>

DHCP_SERVER::DHCP_SERVER(ip_addr_t* ip, ip_addr_t* nm) {
    ip_addr_copy(ipAddress, *ip);
    ip_addr_copy(netmask, *nm);
    memset(lease, 0, sizeof(lease));

    if (SocketNewDatagram(Process) != 0) return;

    SocketBind(PORT_DHCP_SERVER);
}

DHCP_SERVER::~DHCP_SERVER() {
    SocketFree();
}

int DHCP_SERVER::SocketNewDatagram(udp_recv_fn cb_udp_recv) {
    // Family is AF_INET
    // Type is SOCK_DGRAM

    udp = udp_new();
    if (udp == NULL) return -ENOMEM;

    // Register Callback
    udp_recv(udp, cb_udp_recv, (void*)this);

    return 0;
}

int DHCP_SERVER::SocketBind(uint16_t port) {
    return udp_bind(udp, IP_ANY_TYPE, port);
}

void DHCP_SERVER::SocketFree() {
    if (udp == NULL) return;

    udp_remove(udp);
    udp = NULL;
}

int DHCP_SERVER::SocketSendTo(netif* nif, const void* buf, size_t len, uint32_t ip, uint16_t port) {
    // Limit buffer size to 65535 bytes [32-bit register address]
    if (len > 0xFFFF) len = 0xFFFF;

    // Allocate RAM space
    struct pbuf* p = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
    if (p == NULL) return -ENOMEM;

    memcpy(p->payload, buf, len);

    ip_addr_t dest;
    IP4_ADDR(ip_2_ip4(&dest), ip >> 24 & 0xFF, ip >> 16 & 0xFF, ip >> 8 & 0xFF, ip & 0xFF);

    // Send data through UDP
    err_t err;
    if (nif != NULL) err = udp_sendto_if(udp, p, &dest, port, nif);
    else err = udp_sendto(udp, p, &dest, port);

    // Free RAM space
    pbuf_free(p);

    if (err != ERR_OK) return err;

    return len;
}

uint8_t* DHCP_SERVER::Find(uint8_t* opt, uint8_t cmd) {
    for (int i = 0;i < 308 && opt[i] != DHCP_OPT_END;) {
        if (opt[i] == cmd) return &opt[i];
        i += 2 + opt[i + 1];
    }

    return NULL;
}

void DHCP_SERVER::Write(uint8_t** opt, uint8_t cmd, size_t n, const void* data) {
    uint8_t* o = *opt;
    *o++ = cmd;
    *o++ = n;
    memcpy(o, data, n);
    *opt = o + n;
}
void DHCP_SERVER::Write(uint8_t** opt, uint8_t cmd, uint8_t val) {
    uint8_t* o = *opt;
    *o++ = cmd;
    *o++ = 1;
    *o++ = val;
    *opt = o;
}
void DHCP_SERVER::Write(uint8_t** opt, uint8_t cmd, uint32_t val) {
    uint8_t* o = *opt;
    *o++ = cmd;
    *o++ = 4;
    *o++ = val >> 24;
    *o++ = val >> 16;
    *o++ = val >> 8;
    *o++ = val;
    *opt = o;
}

void DHCP_SERVER::Process(void* arg, udp_pcb* upcb, pbuf* p, const ip_addr_t* src_addr, u16_t src_port) {
    DHCP_SERVER* d = (DHCP_SERVER*)arg;
    (void)upcb;
    (void)src_addr;
    (void)src_port;

    // This is around 548 bytes
    Message msg;
    size_t len;
    uint8_t* opt, * msgtype;
    struct netif* nif;

#define DHCP_MIN_SIZE (240+3)
    if (p->tot_len < DHCP_MIN_SIZE) goto ignore_request;

    len = pbuf_copy_partial(p, &msg, sizeof(msg), 0);
    if (len < DHCP_MIN_SIZE) goto ignore_request;

    msg.op = DHCPOFFER;
    memcpy(&msg.yiaddr, &ip4_addr_get_u32(ip_2_ip4(&d->ipAddress)), 4);

    opt = (uint8_t*)msg.options;
    opt += 4; // Assume magic cookie: 99, 130, 83, 99

    msgtype = Find(opt, DHCP_OPT_MSG_TYPE);
    if (msgtype == NULL) goto ignore_request;

    switch (msgtype[2]) {
        case DHCPDISCOVER: {
            int yi = DHCPS_MAX_IP;

            // Look up leases
            for (int i = 0;i < DHCPS_MAX_IP;++i) {
                if (memcmp(d->lease[i].mac, msg.chaddr, MAC_LEN) == 0) {
                    // MAC match, use this IP address
                    yi = i;
                    break;
                }
                if (yi == DHCPS_MAX_IP) {
                    // Look for a free IP address
                    if (memcmp(d->lease[i].mac, "\x00\x00\x00\x00\x00\x00", MAC_LEN) == 0) yi = i; // IP available

                    uint32_t expiry = d->lease[i].expiry << 16 | 0xFFFF;
                    if ((int32_t)(expiry - cyw43_hal_ticks_ms()) < 0) {
                        // IP expired, reuse it
                        memset(d->lease[i].mac, 0, MAC_LEN);
                        yi = i;
                    }
                }
            }

            // No more IP addresses left
            if (yi == DHCPS_MAX_IP) goto ignore_request;

            // Send IP address offer
            msg.yiaddr[3] = DHCPS_BASE_IP + yi;
            Write(&opt, DHCP_OPT_MSG_TYPE, (uint8_t)DHCPOFFER);

            break;
        }
        case DHCPREQUEST: {
            uint8_t* o = Find(opt, DHCP_OPT_REQUESTED_IP);

            if (o == NULL) goto ignore_request; // Should be NACK
            if (memcmp(o + 2, &ip4_addr_get_u32(ip_2_ip4(&d->ipAddress)), 3) != 0) goto ignore_request; // Should be NACK

            uint8_t yi = o[5] - DHCPS_BASE_IP;
            if (yi >= DHCPS_MAX_IP) goto ignore_request; // Should be NACK

            if (memcmp(d->lease[yi].mac, msg.chaddr, MAC_LEN) == 0);
            // MAC match, ok to use this IP address
            else if (memcmp(d->lease[yi].mac, "\x00\x00\x00\x00\x00\x00", MAC_LEN) == 0)
                memcpy(d->lease[yi].mac, msg.chaddr, MAC_LEN);
            else
                goto ignore_request;

            d->lease[yi].expiry = (cyw43_hal_ticks_ms() + DEFAULT_LEASE_TIME_S * 1000) >> 16;
            msg.yiaddr[3] = DHCPS_BASE_IP + yi;
            Write(&opt, DHCP_OPT_MSG_TYPE, (uint8_t)DHCPACK);

            break;
        }
        default:
            goto ignore_request;
    }

    Write(&opt, DHCP_OPT_SERVER_ID, 4, &ip4_addr_get_u32(ip_2_ip4(&d->ipAddress)));
    Write(&opt, DHCP_OPT_SUBNET_MASK, 4, &ip4_addr_get_u32(ip_2_ip4(&d->netmask)));
    Write(&opt, DHCP_OPT_ROUTER, 4, &ip4_addr_get_u32(ip_2_ip4(&d->ipAddress)));
    Write(&opt, DHCP_OPT_DNS, 4, &ip4_addr_get_u32(ip_2_ip4(&d->ipAddress)));

    Write(&opt, DHCP_OPT_IP_LEASE_TIME, (uint32_t)DEFAULT_LEASE_TIME_S);
    *opt++ = DHCP_OPT_END;

    nif = ip_current_input_netif();

    d->SocketSendTo(nif, &msg, opt - (uint8_t*)&msg, 0xFFFFFFFF, PORT_DHCP_CLIENT);

ignore_request:
    pbuf_free(p);
}

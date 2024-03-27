/**
 *@file NekoNet.cpp
 * @author Muhd Syamim (Syazam33@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-03-26
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <NekoNet.h>
#include <DHCP.hpp>

using namespace std;

static const char* SSID = "NekoNet";
static const char* PASS = "12345678";
static bool ledOn = false;

typedef struct TCP_SERVER {
  struct tcp_pcb* server_pcb;
  bool complete;
  ip_addr_t gw;
  async_context* context;
} TCP_SERVER;

int main() {
  stdio_init_all();

  TCP_SERVER* state = (TCP_SERVER*)calloc(1, sizeof(TCP_SERVER));
  if (state == NULL) return 1;

  if (cyw43_arch_init_with_country(CYW43_COUNTRY_SINGAPORE)) return 1;

  cyw43_arch_enable_ap_mode(SSID, PASS, CYW43_AUTH_WPA2_AES_PSK);

  ip4_addr_t netMask;
  IP4_ADDR(ip_2_ip4(&state->gw), 192, 168, 4, 1);
  IP4_ADDR(ip_2_ip4(&netMask), 255, 255, 255, 0);

  // Start DHCP server
  DHCP_SERVER dhcp_server(&state->gw, &netMask);

  while (true) {
    Heartbeat(250);
  }

  dhcp_server.~DHCP_SERVER();
  cyw43_arch_deinit();
  return 0;
}

void Heartbeat(int ms) {
  ledOn = !ledOn;
  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, ledOn);
  sleep_ms(ms);
}
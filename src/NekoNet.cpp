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
#include <DNS.hpp>
#include <TCP.hpp>

using namespace std;

static const char* SSID = "NekoNet";
static const char* PASS = "12345678";
static bool ledOn = false;

int main() {
  stdio_init_all();

  sleep_ms(1000);

  if (cyw43_arch_init_with_country(CYW43_COUNTRY_SINGAPORE)) return 1;
  cyw43_arch_enable_ap_mode(SSID, PASS, CYW43_AUTH_WPA2_AES_PSK);

  TCP_SERVER tcp_server(SSID);

  ip4_addr_t netMask;
  IP4_ADDR(ip_2_ip4(&tcp_server.gw), 192, 168, 4, 1);
  IP4_ADDR(ip_2_ip4(&netMask), 255, 255, 255, 0);

  DHCP_SERVER dhcp_server(&tcp_server.gw, &netMask);

  DNS_SERVER dns_server(&tcp_server.gw);

  tcp_server.complete = false;
  while (tcp_server.complete == false) {
    Heartbeat(250);
  }
  tcp_server.~TCP_SERVER();
  dns_server.~DNS_SERVER();
  dhcp_server.~DHCP_SERVER();
  cyw43_arch_deinit();
  return 0;
}

void Heartbeat(int ms) {
  ledOn = !ledOn;
  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, ledOn);
  sleep_ms(ms);
}
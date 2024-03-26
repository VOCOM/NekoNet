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

#include <pico/cyw43_arch.h>
#include <pico/stdlib.h>

using namespace std;

static const char* SSID = "NekoNet";
static const char* PASS = "12345678";
static bool ledOn = false;

void Heartbeat(int ms);

int main() {
  stdio_init_all();

  if (cyw43_arch_init_with_country(CYW43_COUNTRY_SINGAPORE)) {
    printf("Wi-Fi init failed");
    return -1;
  }

  cyw43_arch_enable_ap_mode(SSID, PASS, CYW43_AUTH_WPA2_AES_PSK);

  while (true) {
    Heartbeat(250);
  }
}

void Heartbeat(int ms) {
  ledOn = !ledOn;
  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, ledOn);
  sleep_ms(ms);
}
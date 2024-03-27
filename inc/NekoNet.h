// NekoNet.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <iostream>
#include <lwipopts.h>

#include <pico/cyw43_arch.h>
#include <pico/stdlib.h>

// For Intellisense
#include <../../pico-sdk/src/boards/include/boards/pico_w.h>

// TODO: Reference additional headers your program requires here.

void Heartbeat(int ms);
project(src)

# Add source to this project's executable.
add_executable(NekoNet
  NekoNet.cpp
  DHCP.cpp
)

if(CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET NekoNet PROPERTY CXX_STANDARD 20)
endif()

include_directories(${CMAKE_SOURCE_DIR}/inc)

target_link_libraries(NekoNet
  pico_stdlib
  pico_cyw43_arch_lwip_threadsafe_background
)

pico_enable_stdio_usb(NekoNet 1)
pico_enable_stdio_uart(NekoNet 0)

pico_add_extra_outputs(NekoNet)
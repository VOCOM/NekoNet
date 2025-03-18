project(src)

# Add source to this project's executable.
add_executable(NekoNet
  NekoNet.cpp
  DHCP.cpp
  DNS.cpp
  TCP.cpp
)

if(CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET NekoNet PROPERTY CXX_STANDARD 20)
endif()

include_directories(${CMAKE_SOURCE_DIR}/inc)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_definitions(NekoNet PUBLIC
    DEBUG_TCP

    # DEBUG_DHCP
    # DEBUG_DNS
  )
endif()

target_link_libraries(NekoNet
  pico_stdlib
  pico_cyw43_arch_lwip_threadsafe_background
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  pico_enable_stdio_usb(NekoNet 1)
  pico_enable_stdio_uart(NekoNet 0)
endif()

pico_add_extra_outputs(NekoNet)
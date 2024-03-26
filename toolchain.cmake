set(PROGRAM_FILES "C:/Program Files (x86)")
set(TOOLCHAIN "${PROGRAM_FILES}/Arm GNU Toolchain arm-none-eabi/13.2 Rel1/bin")

if(WIN32)
    set(CMAKE_C_COMPILER "${TOOLCHAIN}/arm-none-eabi-gcc-13.2.1.exe")
    set(CMAKE_CXX_COMPILER "${TOOLCHAIN}/arm-none-eabi-g++.exe")
    set(CMAKE_ASM_COMPILER "${TOOLCHAIN}/arm-none-eabi-as.exe")
    set(CMAKE_MAKE_PROGRAM "${PROGRAM_FILES}/GnuWin32/bin/make.exe")
endif()
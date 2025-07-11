cmake_minimum_required(VERSION 3.13)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(PICO_BOARD pico_w CACHE STRING "Board type")
include(pico_sdk_import.cmake)
project(Grupo_Bomba C CXX ASM)
pico_sdk_init()

include_directories(${CMAKE_SOURCE_DIR}/lib)

add_executable(${PROJECT_NAME}  
        Grupo_Bomba.c
        lib/ssd1306/ssd1306.c
        lib/botoes/botoes.c
        lib/matriz/matriz.c
        lib/potenciometro/potenciometro.c
        lib/web/web.c
        lib/buzzer/buzzer.c
        lib/rgb/rgb.c
        )

pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/lib/matriz/matriz.pio)

target_link_libraries(${PROJECT_NAME} 
        pico_stdlib 
        hardware_i2c
        hardware_adc
        hardware_pwm        
        pico_cyw43_arch_lwip_threadsafe_background
        )

target_include_directories(${PROJECT_NAME}  PRIVATE   ${CMAKE_CURRENT_LIST_DIR} 
        lib/ssd1306
        lib/botoes
        lib/matriz
        lib/potenciometro
        lib/web
        lib/buzzer
        lib/rgb
        )

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)


pico_add_extra_outputs(${PROJECT_NAME})
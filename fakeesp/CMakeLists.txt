
cmake_minimum_required(VERSION 3.13)

project("wi-se-sw" CXX)

add_compile_options(
        "-Wall" "-Wpedantic" "-Wextra" "-fno-exceptions" "-Wno-unused-parameter"
        "$<$<CONFIG:DEBUG>:-O0;-g3;-ggdb;-fno-stack-protector>"
)

add_definitions(-D'PLATFORMIO=50004')
add_definitions(-D'ESP8266')
add_definitions(-D'ARDUINO_ARCH_ESP8266')
add_definitions(-D'ARDUINO_ESP8266_ESP_WROOM_02')
add_definitions(-D'PIO_FRAMEWORK_ARDUINO_LWIP_HIGHER_BANDWIDTH')
add_definitions(-D'F_CPU=160000000L')
add_definitions(-D'__ets__')
add_definitions(-D'ICACHE_FLASH')
add_definitions(-D'ARDUINO=10805')
add_definitions(-D'ARDUINO_BOARD=\"PLATFORMIO_ESP_WROOM_02\"')
add_definitions(-D'FLASHMODE_QIO')
add_definitions(-D'LWIP_OPEN_SRC')
add_definitions(-D'NONOSDK22x_190703=1')
add_definitions(-D'VTABLES_IN_FLASH')

include_directories("/usr/lib/include")
include_directories("${CMAKE_CURRENT_LIST_DIR}/include")
include_directories("${CMAKE_CURRENT_LIST_DIR}/lib")
include_directories("${CMAKE_CURRENT_LIST_DIR}/src")
include_directories("${CMAKE_CURRENT_LIST_DIR}/ESPAsyncTCP/src")
include_directories("${CMAKE_CURRENT_LIST_DIR}/../include")
include_directories("${CMAKE_CURRENT_LIST_DIR}/../src")
include_directories("${CMAKE_CURRENT_LIST_DIR}/../.pio/libdeps/esp_wroom_02/ESP Async WebServer/src")
include_directories("${CMAKE_CURRENT_LIST_DIR}/../.pio/libdeps/esp_wroom_02/ArduinoJson/src")
include_directories("$ENV{HOME}/.platformio/packages/framework-arduinoespressif8266/cores/esp8266/")
include_directories("$ENV{HOME}/.platformio/packages/framework-arduinoespressif8266/libraries/Hash/src")


FILE(GLOB_RECURSE SRC_LIST
        "${CMAKE_CURRENT_LIST_DIR}/src/*.*"
        "${CMAKE_CURRENT_LIST_DIR}/src/*.*"
        "${CMAKE_CURRENT_LIST_DIR}/src/libb64/*.*"
        "${CMAKE_CURRENT_LIST_DIR}/lib/*.*"
        "${CMAKE_CURRENT_LIST_DIR}/include/*.*"
        "${CMAKE_CURRENT_LIST_DIR}/include/libb64/*.*"
        "${CMAKE_CURRENT_LIST_DIR}/ESPAsyncTCP/src/*.*"
        "${CMAKE_CURRENT_LIST_DIR}/../.pio/libdeps/esp_wroom_02/ESP Async WebServer/src/*.*"
        "${CMAKE_CURRENT_LIST_DIR}/../.pio/libdeps/esp_wroom_02/ArduinoJson/src/*.*"
        "${CMAKE_CURRENT_LIST_DIR}/../src/*.*"
        "${CMAKE_CURRENT_LIST_DIR}/../lib/*.*"
        "${CMAKE_CURRENT_LIST_DIR}/../include/*.*"
        )

list(APPEND SRC_LIST)

message(STATUS "SRC_LIST: ${SRC_LIST}")
message(STATUS "HOME: $ENV{HOME}")

add_executable(wi-se_fakeesp main.cpp ${SRC_LIST})

find_package(OpenSSL REQUIRED)
target_link_libraries(wi-se_fakeesp OpenSSL::SSL)
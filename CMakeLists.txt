cmake_minimum_required(VERSION 3.10)
project(touchpad-enhance VERSION 1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(INVERTX "Invert X axis" OFF)
option(INVERTY "Invert Y axis" OFF)
option(TOUCHPAD_DEVICE "Specify touchpad device file (e.g., /dev/input/event5)" "")


file(GLOB CONTROL_SOURCES "${CMAKE_SOURCE_DIR}/control/*.cpp")
add_executable(${PROJECT_NAME} main.cpp ${CONTROL_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}")
if(INVERTX)
    target_compile_definitions(${PROJECT_NAME} PRIVATE INVERTX=1)
endif()
if(INVERTY)
    target_compile_definitions(${PROJECT_NAME} PRIVATE INVERTY=1)
endif()
if(TOUCHPAD_DEVICE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE TOUCHPAD_DEVICE="${TOUCHPAD_DEVICE}")
endif()


install(TARGETS ${PROJECT_NAME} DESTINATION bin)
configure_file("${CMAKE_SOURCE_DIR}/install/touchpad-enhance.service.in"
               "${CMAKE_BINARY_DIR}/touchpad-enhance.service"
               @ONLY)
install(FILES "${CMAKE_BINARY_DIR}/touchpad-enhance.service"
        DESTINATION lib/systemd/system)
install(CODE "execute_process(COMMAND systemctl daemon-reload)")
install(CODE "execute_process(COMMAND systemctl enable touchpad-enhance.service)")
install(CODE "execute_process(COMMAND systemctl start touchpad-enhance.service)")


if(NOT TARGET uninstall)
    configure_file(
        "${CMAKE_SOURCE_DIR}/install/Uninstall.cmake.in"
        "${CMAKE_BINARY_DIR}/Uninstall.cmake"
        IMMEDIATE @ONLY)
    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/Uninstall.cmake)
endif()

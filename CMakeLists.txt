cmake_minimum_required(VERSION 3.10)
project(touchpad-enhance VERSION 1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(INVERTX "Invert X axis" OFF)
option(INVERTY "Invert Y axis" OFF)
set(TOUCHPAD_DEVICE "" CACHE STRING "Specify touchpad device file (e.g., /dev/input/event5). Empty means auto-detect.")
set(EDGE_WIDTH "0.05" CACHE STRING "Width of the edge region as a fraction of total touchpad width")
set(SCROLL_THRESHOLD "0.2" CACHE STRING "Minimal vertical movement to trigger a scroll action")
set(SCROLL_STEP "0.05" CACHE STRING "Movement step for each scroll action")


file(GLOB CONTROL_SOURCES "${CMAKE_SOURCE_DIR}/control/*.cpp")
add_executable(${PROJECT_NAME} main.cpp ${CONTROL_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}")
target_link_libraries(${PROJECT_NAME} PRIVATE pthread)
if(INVERTX)
    target_compile_definitions(${PROJECT_NAME} PRIVATE INVERTX=1)
endif()
if(INVERTY)
    target_compile_definitions(${PROJECT_NAME} PRIVATE INVERTY=1)
endif()
if(TOUCHPAD_DEVICE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE TOUCHPAD_DEVICE="${TOUCHPAD_DEVICE}")
endif()
target_compile_definitions(${PROJECT_NAME} PRIVATE
    EDGE_WIDTH=${EDGE_WIDTH}
    SCROLL_THRESHOLD=${SCROLL_THRESHOLD}
    SCROLL_STEP=${SCROLL_STEP}
)


# Control CLI: only needs the IPC client code, not the touchpad/emulator stack.
add_executable(touchpad-enhance-ctl ctl/main.cpp "${CMAKE_SOURCE_DIR}/control/ipc.cpp")
target_include_directories(touchpad-enhance-ctl PRIVATE "${CMAKE_SOURCE_DIR}")


install(TARGETS ${PROJECT_NAME} touchpad-enhance-ctl DESTINATION bin)
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

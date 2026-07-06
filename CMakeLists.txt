cmake_minimum_required(VERSION 3.25)
project(chatty LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(
    -O2 -Wall -Wextra -Wpedantic
    -Wformat -Wformat=2 -Werror=format-security
    -Wimplicit-fallthrough
    -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3
    -D_GLIBCXX_ASSERTIONS
    -fstack-clash-protection -fstack-protector-strong
)

add_link_options(
    -Wl,-z,nodlopen -Wl,-z,noexecstack
    -Wl,-z,relro -Wl,-z,now
    -Wl,--as-needed -Wl,--no-copy-dt-needed-entries
)

include(FetchContent)

set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/arthursonzogni/ftxui.git
  GIT_TAG        v7.0.0
  GIT_PROGRESS   TRUE
  GIT_SHALLOW    TRUE
  EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(ftxui)

add_executable(chatty
  src/main.cpp
  src/app.cpp
  src/uniqueid.cpp
  src/networking/packets.cpp
  src/networking/client.cpp
  src/networking/udpsocket.cpp
  src/networking/tcpsocket.cpp
  src/networking/tcpserver.cpp
  src/screens/name_input.cpp
  src/screens/waiting_for_peer.cpp
  src/screens/chat_screen.cpp
)

target_include_directories(chatty PRIVATE src)

target_link_libraries(chatty
  PRIVATE ftxui::screen
  PRIVATE ftxui::dom
  PRIVATE ftxui::component
)

# Tests

enable_testing()
add_executable(tests
  src/testing/tests.cpp
  src/networking/tcpserver.cpp
  src/networking/tcpsocket.cpp
  src/networking/packets.cpp
  src/uniqueid.cpp
)
target_include_directories(tests PRIVATE src)

# unique file descriptor
add_test(NAME test_move_file_descriptor COMMAND tests test_move_file_descriptor)
add_test(NAME test_fd_default_deleter_closes COMMAND tests test_fd_default_deleter_closes)
# tcp
add_test(NAME test_tcp_server_client COMMAND tests test_tcp_server_client)
add_test(NAME test_tcp_bidirectional COMMAND tests test_tcp_bidirectional)
add_test(NAME test_tcp_multiple_messages COMMAND tests test_tcp_multiple_messages)
add_test(NAME test_tcp_close_detection COMMAND tests test_tcp_close_detection)
add_test(NAME test_tcp_connect_bad_port COMMAND tests test_tcp_connect_bad_port)
add_test(NAME test_tcp_server_accept_result_has_peer_address COMMAND tests test_tcp_server_accept_result_has_peer_address)
# packets
add_test(NAME test_hello_packet_roundtrip COMMAND tests test_hello_packet_roundtrip)
add_test(NAME test_discover_packet_roundtrip COMMAND tests test_discover_packet_roundtrip)
add_test(NAME test_packet_username_with_spaces COMMAND tests test_packet_username_with_spaces)
add_test(NAME test_packet_empty_username COMMAND tests test_packet_empty_username)
add_test(NAME test_packet_parse_rejects_empty COMMAND tests test_packet_parse_rejects_empty)
add_test(NAME test_packet_parse_rejects_garbage COMMAND tests test_packet_parse_rejects_garbage)
add_test(NAME test_packet_parse_rejects_wrong_app COMMAND tests test_packet_parse_rejects_wrong_app)
add_test(NAME test_packet_parse_rejects_wrong_version COMMAND tests test_packet_parse_rejects_wrong_version)
add_test(NAME test_packet_parse_rejects_unknown_breadcrumb COMMAND tests test_packet_parse_rejects_unknown_breadcrumb)
add_test(NAME test_packet_parse_rejects_missing_fields COMMAND tests test_packet_parse_rejects_missing_fields)
add_test(NAME test_packet_parse_rejects_oversized COMMAND tests test_packet_parse_rejects_oversized)
add_test(NAME test_packet_port_zero COMMAND tests test_packet_port_zero)
add_test(NAME test_packet_max_port COMMAND tests test_packet_max_port)
# unique id
add_test(NAME test_unique_id_length COMMAND tests test_unique_id_length)
add_test(NAME test_unique_id_hex_chars COMMAND tests test_unique_id_hex_chars)

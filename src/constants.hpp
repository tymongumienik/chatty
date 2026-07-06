#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace Constants {

constexpr std::string_view APP_NAME = "chattyp2p";  // mustn't include spaces!

constexpr std::uint16_t PROTOCOL_VERSION = 3;

constexpr std::size_t MAX_PACKET_SIZE = 4096;  // 4KB
constexpr std::size_t MESSAGE_LENGTH_PREFIX_SIZE =
    4;  // big-endian message length prefix
constexpr std::size_t MAX_MESSAGE_SIZE = 65536;  // 64KB
constexpr int NETWORK_POLL_INTERVAL_MS = 10;
constexpr std::uint16_t DISCOVERY_PORT = 1337;
constexpr std::string_view DISCOVERY_ADDRESS = "255.255.255.255";

constexpr std::array<std::string_view, 6> ASCII_ART = {
    R"(       _           _   _         )",
    R"(   ___| |__   __ _| |_| |_ _   _ )",
    R"(  / __| '_ \ / _` | __| __| | | |)",
    R"( | (__| | | | (_| | |_| |_| |_| |)",
    R"(  \___|_| |_|\__,_|\__|\__|\__, |)",
    R"(                           |___/ )"};

}  // namespace Constants

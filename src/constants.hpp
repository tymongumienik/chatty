#pragma once

#include <array>

namespace Constants {

constexpr std::string_view APP_NAME = "chattyp2p";  // mustn't include spaces!

constexpr std::uint16_t PROTOCOL_VERSION = 1;
constexpr std::string_view PROTOCOL_VERSION_SV = "1";

constexpr std::size_t MAX_PACKET_SIZE = 4096;  // 4KB

constexpr std::array<std::string_view, 6> ASCII_ART = {
    R"(       _           _   _         )",
    R"(   ___| |__   __ _| |_| |_ _   _ )",
    R"(  / __| '_ \ / _` | __| __| | | |)",
    R"( | (__| | | | (_| | |_| |_| |_| |)",
    R"(  \___|_| |_|\__,_|\__|\__|\__, |)",
    R"(                           |___/ )"};

};  // namespace Constants

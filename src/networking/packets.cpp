#include "packets.hpp"
#include <charconv>
#include <format>
#include <memory>
#include "constants.hpp"

std::string Networking::HelloPacket::serialize() const {
  return std::format("{} {} {} {} {} {}", getBreadcrumb(), Constants::APP_NAME,
                     Constants::PROTOCOL_VERSION, tcpPort, instanceId,
                     username);
}

std::string Networking::DiscoverPacket::serialize() const {
  return std::format("{} {} {} {} {} {}", getBreadcrumb(), Constants::APP_NAME,
                     Constants::PROTOCOL_VERSION, tcpPort, instanceId,
                     username);
}

namespace {  // scoped
bool safe_to_uint16(std::string_view str, std::uint16_t& out_value) {
  if (str.empty())
    return false;

  auto [ptr, ec] =
      std::from_chars(str.data(), str.data() + str.size(), out_value);

  return (ec == std::errc{} && ptr == str.data() + str.size());
}

std::string_view next_token(std::string_view& view) {
  const auto first_non_space = view.find_first_not_of(' ');
  if (first_non_space == std::string_view::npos) {
    view = {};
    return {};
  }
  view.remove_prefix(first_non_space);

  const auto space_pos = view.find(' ');
  if (space_pos == std::string_view::npos) {
    std::string_view token = view;
    view = {};  // Mark the remaining view as empty
    return token;
  }

  std::string_view token = view.substr(0, space_pos);
  view.remove_prefix(space_pos);

  return token;
}

// Returns everything remaining in view (trimmed of leading spaces)
std::string_view rest_of(std::string_view& view) {
  const auto first_non_space = view.find_first_not_of(' ');
  if (first_non_space == std::string_view::npos) {
    view = {};
    return {};
  }
  view.remove_prefix(first_non_space);
  std::string_view result = view;
  view = {};
  return result;
}
}  // namespace

std::unique_ptr<Networking::Packet> Networking::Packet::parse(
    const std::string& rawData) {
  if (rawData.size() > Constants::MAX_PACKET_SIZE || rawData.empty()) {
    return nullptr;
  }

  std::string_view view(rawData);

  std::string_view breadcrumb = next_token(view);
  std::string_view appName = next_token(view);
  std::string_view protocolVersion = next_token(view);

  if (breadcrumb.empty() || appName != Constants::APP_NAME) {
    return nullptr;
  }

  std::uint16_t proto_version = 0;
  if (!safe_to_uint16(protocolVersion, proto_version) ||
      proto_version != Constants::PROTOCOL_VERSION) {
    return nullptr;
  }

  if (breadcrumb == "HELLO") {
    std::string_view portStr = next_token(view);
    std::string_view instanceId = next_token(view);
    std::string_view username = rest_of(view);

    if (portStr.empty() || instanceId.empty())
      return nullptr;

    std::uint16_t tcpPort = 0;
    if (!safe_to_uint16(portStr, tcpPort))
      return nullptr;

    auto packet = std::make_unique<Networking::HelloPacket>();
    packet->tcpPort = tcpPort;
    packet->instanceId = std::string(instanceId);
    packet->username = std::string(username);
    return packet;
  }

  if (breadcrumb == "DISCOVER") {
    std::string_view portStr = next_token(view);
    std::string_view instanceId = next_token(view);
    std::string_view username = rest_of(view);

    if (portStr.empty() || instanceId.empty())
      return nullptr;

    std::uint16_t tcpPort = 0;
    if (!safe_to_uint16(portStr, tcpPort))
      return nullptr;

    auto packet = std::make_unique<Networking::DiscoverPacket>();
    packet->tcpPort = tcpPort;
    packet->instanceId = std::string(instanceId);
    packet->username = std::string(username);
    return packet;
  }

  // Invalid breadcrumb
  return nullptr;
}

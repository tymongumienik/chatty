#include "packets.hpp"
#include <charconv>
#include <format>
#include <memory>
#include "constants.hpp"

std::string HelloPacket::serialize() const {
  return std::format("{} {} {} {} {}", getBreadcrumb(), Constants::APP_NAME,
                     Constants::PROTOCOL_VERSION, tcpPort, instanceId);
}

std::string DiscoverPacket::serialize() const {
  return std::format("{} {} {} {}", getBreadcrumb(), Constants::APP_NAME,
                     Constants::PROTOCOL_VERSION, instanceId);
}

namespace {  // scoped
// WIP
bool safe_to_uint16(std::string_view str, std::uint16_t& out_value) {
  if (str.empty())
    return false;

  auto [ptr, ec] =
      std::from_chars(str.data(), str.data() + str.size(), out_value);

  return (ec == std::errc{} && ptr == str.data() + str.size());
}

// WIP
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
}  // namespace

std::unique_ptr<Packet> Packet::parse(const std::string& rawData) {
  if (rawData.size() > Constants::MAX_PACKET_SIZE || rawData.empty()) {
    return nullptr;
  }

  std::string_view view(rawData);

  std::string_view breadcrumb = next_token(view);
  std::string_view appName = next_token(view);
  std::string_view protocolVersion = next_token(view);

  if (breadcrumb.empty() || appName != Constants::APP_NAME ||
      protocolVersion != Constants::PROTOCOL_VERSION_SV) {
    return nullptr;
  }

  if (breadcrumb == "HELLO") {
    std::string_view portStr = next_token(view);
    std::string_view instanceId = next_token(view);

    if (portStr.empty() || instanceId.empty() || !view.empty())
      return nullptr;

    std::uint16_t tcpPort = 0;
    if (!safe_to_uint16(portStr, tcpPort))
      return nullptr;

    auto packet = std::make_unique<HelloPacket>();
    packet->tcpPort = tcpPort;
    packet->instanceId = std::string(instanceId);
    return packet;
  }

  if (breadcrumb == "DISCOVER") {
    std::string_view instanceId = next_token(view);

    if (instanceId.empty() || !view.empty())
      return nullptr;

    auto packet = std::make_unique<DiscoverPacket>();
    packet->instanceId = std::string(instanceId);
    return packet;
  }

  // Invalid breadcrumb
  return nullptr;
}

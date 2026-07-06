#include "packets.hpp"
#include <charconv>
#include <format>
#include <memory>
#include "constants.hpp"

std::string Networking::HelloPacket::Serialize() const {
  return std::format("{} {} {} {} {} {}", GetBreadcrumb(), Constants::APP_NAME,
                     Constants::PROTOCOL_VERSION, tcp_port, instance_id,
                     username);
}

std::string Networking::DiscoverPacket::Serialize() const {
  return std::format("{} {} {} {} {} {}", GetBreadcrumb(), Constants::APP_NAME,
                     Constants::PROTOCOL_VERSION, tcp_port, instance_id,
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
    view = {};  // mark the remaining view as empty
    return token;
  }

  std::string_view token = view.substr(0, space_pos);
  view.remove_prefix(space_pos);

  return token;
}

// returns everything remaining in view (trimmed of leading spaces)
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

std::unique_ptr<Networking::Packet> Networking::Packet::Parse(
    const std::string& raw_data) {
  if (raw_data.size() > Constants::MAX_PACKET_SIZE || raw_data.empty()) {
    return nullptr;
  }

  std::string_view view(raw_data);

  std::string_view breadcrumb = next_token(view);
  std::string_view app_name = next_token(view);
  std::string_view protocol_version_str = next_token(view);

  if (breadcrumb.empty() || app_name != Constants::APP_NAME) {
    return nullptr;
  }

  std::uint16_t protocol_version = 0;
  if (!safe_to_uint16(protocol_version_str, protocol_version) ||
      protocol_version != Constants::PROTOCOL_VERSION) {
    return nullptr;
  }

  if (breadcrumb == "HELLO") {
    std::string_view port_str = next_token(view);
    std::string_view instance_id = next_token(view);
    std::string_view username = rest_of(view);

    if (port_str.empty() || instance_id.empty())
      return nullptr;

    std::uint16_t tcp_port_local = 0;
    if (!safe_to_uint16(port_str, tcp_port_local))
      return nullptr;

    auto packet = std::make_unique<Networking::HelloPacket>();
    packet->tcp_port = tcp_port_local;
    packet->instance_id = std::string(instance_id);
    packet->username = std::string(username);
    return packet;
  }

  if (breadcrumb == "DISCOVER") {
    std::string_view port_str = next_token(view);
    std::string_view instance_id = next_token(view);
    std::string_view username = rest_of(view);

    if (port_str.empty() || instance_id.empty())
      return nullptr;

    std::uint16_t tcp_port_local = 0;
    if (!safe_to_uint16(port_str, tcp_port_local))
      return nullptr;

    auto packet = std::make_unique<Networking::DiscoverPacket>();
    packet->tcp_port = tcp_port_local;
    packet->instance_id = std::string(instance_id);
    packet->username = std::string(username);
    return packet;
  }

  // Invalid breadcrumb
  return nullptr;
}

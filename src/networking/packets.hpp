#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "constants.hpp"

namespace Networking {
struct Packet {
  virtual ~Packet() = default;
  virtual std::string GetBreadcrumb() const = 0;
  virtual std::string Serialize() const = 0;

  static std::unique_ptr<Packet> Parse(const std::string& raw_data);
};

struct HelloPacket : Packet {
  std::uint16_t tcp_port{};
  std::string instance_id;
  std::string username;

  std::string GetBreadcrumb() const override { return "HELLO"; }

  std::string Serialize() const override;
};

struct DiscoverPacket : Packet {
  std::uint16_t tcp_port{};
  std::string instance_id;
  std::string username;

  std::string GetBreadcrumb() const override { return "DISCOVER"; }

  std::string Serialize() const override;
};
}  // namespace Networking

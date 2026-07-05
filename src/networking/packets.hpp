#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "constants.hpp"

namespace Networking {
struct Packet {
  virtual ~Packet() = default;
  virtual std::string getBreadcrumb() const = 0;
  virtual std::string serialize() const = 0;

  static std::unique_ptr<Packet> parse(const std::string& rawData);
};

struct HelloPacket : Packet {
  std::uint16_t tcpPort{};
  std::string instanceId;

  std::string getBreadcrumb() const override { return "HELLO"; }

  std::string serialize() const override;
};

struct DiscoverPacket : Packet {
  std::uint16_t tcpPort{};
  std::string instanceId;

  std::string getBreadcrumb() const override { return "DISCOVER"; }

  std::string serialize() const override;
};
};  // namespace Networking

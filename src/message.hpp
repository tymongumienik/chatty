#pragma once

#include <string>

struct Message {
  std::string sender;
  std::string text;
  bool mine;
};

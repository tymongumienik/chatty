#include <fcntl.h>
#include <atomic>
#include <cassert>
#include "networking/filedescriptor.hpp"

void test_move_file_descriptor() {
  int devnull = open("/dev/null", O_RDONLY);
  assert(devnull >= 0);

  std::atomic<int> deleter_calls = 0;

  {
    auto a = UniqueFileDescriptor::make(devnull, [&deleter_calls](int fd) {
      if (fd >= 0) {
        ::close(fd);
        ++deleter_calls;
      }
    });

    assert(a.get() == devnull);

    // ownership transfer
    auto b = std::move(a);

    assert(b.get() == devnull);
  }

  assert(deleter_calls ==
         1);  // because we moved a to b, only b has rights to call the deleter
}

int main() {
  test_move_file_descriptor();
}

#include <fcntl.h>
#include <atomic>
#include <cassert>
#include <chrono>
#include <string_view>
#include <thread>
#include <unordered_map>
#include "networking/filedescriptor.hpp"
#include "networking/packets.hpp"
#include "networking/tcpserver.hpp"
#include "networking/tcpsocket.hpp"
#include "uniqueid.hpp"

// ─── helpers ───────────────────────────────────────────────────────────

static Networking::RecvResult poll_recv(Networking::TcpSocket& sock,
                                        int max_tries = 100) {
  Networking::RecvResult r{Networking::RecvStatus::WouldBlock, {}};
  for (int i = 0; i < max_tries; ++i) {
    r = sock.ReceiveRaw();
    if (r.status == Networking::RecvStatus::Data)
      break;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  return r;
}

static std::optional<Networking::TcpServer::AcceptResult> poll_accept(
    Networking::TcpServer& server,
    int max_tries = 100) {
  for (int i = 0; i < max_tries; ++i) {
    auto accepted = server.Accept();
    if (accepted)
      return accepted;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  return std::nullopt;
}

// ─── file descriptor tests ────────────────────────────────────────────

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

void test_fd_default_deleter_closes() {
  int fd = open("/dev/null", O_RDONLY);
  assert(fd >= 0);

  {
    auto resource = UniqueFileDescriptor::make(fd);
  }  // should close fd

  // checking a closed fd should fail with EBADF
  [[maybe_unused]] int res = ::fcntl(fd, F_GETFD);
  assert(res < 0 && errno == EBADF);
}

// ─── tcp server/client tests ──────────────────────────────────────────

void test_tcp_server_client() {
  using namespace Networking;

  TcpServer server(0);
  std::uint16_t port = server.GetPort();
  assert(port > 0);

  std::string local_ip = "127.0.0.1";

  TcpSocket client;
  [[maybe_unused]] bool connected = client.Connect(local_ip, port);
  assert(connected);

  auto accepted = poll_accept(server);
  assert(accepted.has_value());
  assert(accepted->socket.IsValid());

  client.SendRaw("hello from client");

  auto received = poll_recv(accepted->socket);
  assert(received.status == RecvStatus::Data);
  assert(received.data == "hello from client");
}

void test_tcp_bidirectional() {
  using namespace Networking;

  TcpServer server(0);

  TcpSocket client;
  [[maybe_unused]] bool connected =
      client.Connect("127.0.0.1", server.GetPort());
  assert(connected);

  auto accepted = poll_accept(server);
  assert(accepted.has_value());

  // client → server
  client.SendRaw("ping");
  auto r1 = poll_recv(accepted->socket);
  assert(r1.status == RecvStatus::Data);
  assert(r1.data == "ping");

  // server → client
  accepted->socket.SendRaw("pong");
  auto r2 = poll_recv(client);
  assert(r2.status == RecvStatus::Data);
  assert(r2.data == "pong");
}

void test_tcp_multiple_messages() {
  using namespace Networking;

  TcpServer server(0);

  TcpSocket client;
  [[maybe_unused]] bool connected =
      client.Connect("127.0.0.1", server.GetPort());
  assert(connected);

  auto accepted = poll_accept(server);
  assert(accepted.has_value());

  // send several messages in sequence
  for (int i = 0; i < 5; ++i) {
    std::string msg = "message_" + std::to_string(i);
    client.SendRaw(msg);

    auto r = poll_recv(accepted->socket);
    assert(r.status == RecvStatus::Data);
    assert(r.data == msg);
  }
}

void test_tcp_close_detection() {
  using namespace Networking;

  TcpServer server(0);

  TcpSocket client;
  [[maybe_unused]] bool connected =
      client.Connect("127.0.0.1", server.GetPort());
  assert(connected);

  auto accepted = poll_accept(server);
  assert(accepted.has_value());

  // close the client side
  client.Close();

  // server should eventually see Closed
  RecvResult r{RecvStatus::WouldBlock, {}};
  for (int i = 0; i < 100; ++i) {
    r = accepted->socket.ReceiveRaw();
    if (r.status == RecvStatus::Closed)
      break;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  assert(r.status == RecvStatus::Closed);
}

void test_tcp_connect_bad_port() {
  using namespace Networking;

  TcpSocket client;
  // port 1 is almost certainly not listening
  [[maybe_unused]] bool connected = client.Connect("127.0.0.1", 1);
  assert(!connected);
}

void test_tcp_server_accept_result_has_peer_address() {
  using namespace Networking;

  TcpServer server(0);

  TcpSocket client;
  [[maybe_unused]] bool connected =
      client.Connect("127.0.0.1", server.GetPort());
  assert(connected);

  auto accepted = poll_accept(server);
  assert(accepted.has_value());
  assert(accepted->peer_address == "127.0.0.1");
}

// ─── packet tests ─────────────────────────────────────────────────────

void test_hello_packet_roundtrip() {
  using namespace Networking;

  HelloPacket original;
  original.tcp_port = 12345;
  original.instance_id = "abc123";
  original.username = "alice";

  std::string wire = original.Serialize();
  auto parsed = Packet::Parse(wire);
  assert(parsed != nullptr);

  [[maybe_unused]] auto* hello = dynamic_cast<HelloPacket*>(parsed.get());
  assert(hello != nullptr);
  assert(hello->tcp_port == 12345);
  assert(hello->instance_id == "abc123");
  assert(hello->username == "alice");
  assert(hello->GetBreadcrumb() == "HELLO");
}

void test_discover_packet_roundtrip() {
  using namespace Networking;

  DiscoverPacket original;
  original.tcp_port = 0;
  original.instance_id = "def456";
  original.username = "bob";

  std::string wire = original.Serialize();
  auto parsed = Packet::Parse(wire);
  assert(parsed != nullptr);

  [[maybe_unused]] auto* discover = dynamic_cast<DiscoverPacket*>(parsed.get());
  assert(discover != nullptr);
  assert(discover->tcp_port == 0);
  assert(discover->instance_id == "def456");
  assert(discover->username == "bob");
  assert(discover->GetBreadcrumb() == "DISCOVER");
}

void test_packet_username_with_spaces() {
  using namespace Networking;

  HelloPacket original;
  original.tcp_port = 8080;
  original.instance_id = "id99";
  original.username = "John Doe The Third";

  std::string wire = original.Serialize();
  auto parsed = Packet::Parse(wire);
  assert(parsed != nullptr);

  [[maybe_unused]] auto* hello = dynamic_cast<HelloPacket*>(parsed.get());
  assert(hello != nullptr);
  assert(hello->username == "John Doe The Third");
}

void test_packet_empty_username() {
  using namespace Networking;

  DiscoverPacket original;
  original.tcp_port = 5555;
  original.instance_id = "xyz";
  original.username = "";

  std::string wire = original.Serialize();
  auto parsed = Packet::Parse(wire);
  assert(parsed != nullptr);

  [[maybe_unused]] auto* discover = dynamic_cast<DiscoverPacket*>(parsed.get());
  assert(discover != nullptr);
  assert(discover->username.empty());
}

void test_packet_parse_rejects_empty() {
  auto parsed = Networking::Packet::Parse("");
  assert(parsed == nullptr);
}

void test_packet_parse_rejects_garbage() {
  auto parsed = Networking::Packet::Parse("not a real packet at all");
  assert(parsed == nullptr);
}

void test_packet_parse_rejects_wrong_app() {
  std::string bad = "HELLO wrongapp 3 1234 someid alice";
  auto parsed = Networking::Packet::Parse(bad);
  assert(parsed == nullptr);
}

void test_packet_parse_rejects_wrong_version() {
  auto bad = std::format("HELLO {} 999 1234 someid alice", Constants::APP_NAME);
  auto parsed = Networking::Packet::Parse(bad);
  assert(parsed == nullptr);
}

void test_packet_parse_rejects_unknown_breadcrumb() {
  auto bad = std::format("FOOBAR {} {} 1234 someid alice", Constants::APP_NAME,
                         Constants::PROTOCOL_VERSION);
  auto parsed = Networking::Packet::Parse(bad);
  assert(parsed == nullptr);
}

void test_packet_parse_rejects_missing_fields() {
  // just breadcrumb + app + version, no port or instance_id
  auto bad = std::format("HELLO {} {}", Constants::APP_NAME,
                         Constants::PROTOCOL_VERSION);
  auto parsed = Networking::Packet::Parse(bad);
  assert(parsed == nullptr);
}

void test_packet_parse_rejects_oversized() {
  std::string big(Constants::MAX_PACKET_SIZE + 1, 'X');
  auto parsed = Networking::Packet::Parse(big);
  assert(parsed == nullptr);
}

void test_packet_port_zero() {
  using namespace Networking;

  HelloPacket p;
  p.tcp_port = 0;
  p.instance_id = "host_me";
  p.username = "tester";

  auto parsed = Packet::Parse(p.Serialize());
  assert(parsed != nullptr);

  [[maybe_unused]] auto* hello = dynamic_cast<HelloPacket*>(parsed.get());
  assert(hello != nullptr);
  assert(hello->tcp_port == 0);
}

void test_packet_max_port() {
  using namespace Networking;

  DiscoverPacket p;
  p.tcp_port = 65535;
  p.instance_id = "edge";
  p.username = "max";

  auto parsed = Packet::Parse(p.Serialize());
  assert(parsed != nullptr);

  [[maybe_unused]] auto* disc = dynamic_cast<DiscoverPacket*>(parsed.get());
  assert(disc != nullptr);
  assert(disc->tcp_port == 65535);
}

// ─── unique id tests ──────────────────────────────────────────────────

void test_unique_id_length() {
  auto id = UniqueId::make();
  assert(id.size() == UniqueId::length);
}

void test_unique_id_hex_chars() {
  auto id = UniqueId::make();
  for (char c : id) {
    (void)c;
    assert((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'));
  }
}

// ─── main ─────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
  std::unordered_map<std::string, void (*)()> tests = {
      // unique file descriptor
      {"test_move_file_descriptor", test_move_file_descriptor},
      {"test_fd_default_deleter_closes", test_fd_default_deleter_closes},
      // tcp
      {"test_tcp_server_client", test_tcp_server_client},
      {"test_tcp_bidirectional", test_tcp_bidirectional},
      {"test_tcp_multiple_messages", test_tcp_multiple_messages},
      {"test_tcp_close_detection", test_tcp_close_detection},
      {"test_tcp_connect_bad_port", test_tcp_connect_bad_port},
      {"test_tcp_server_accept_result_has_peer_address",
       test_tcp_server_accept_result_has_peer_address},
      // packets
      {"test_hello_packet_roundtrip", test_hello_packet_roundtrip},
      {"test_discover_packet_roundtrip", test_discover_packet_roundtrip},
      {"test_packet_username_with_spaces", test_packet_username_with_spaces},
      {"test_packet_empty_username", test_packet_empty_username},
      {"test_packet_parse_rejects_empty", test_packet_parse_rejects_empty},
      {"test_packet_parse_rejects_garbage", test_packet_parse_rejects_garbage},
      {"test_packet_parse_rejects_wrong_app",
       test_packet_parse_rejects_wrong_app},
      {"test_packet_parse_rejects_wrong_version",
       test_packet_parse_rejects_wrong_version},
      {"test_packet_parse_rejects_unknown_breadcrumb",
       test_packet_parse_rejects_unknown_breadcrumb},
      {"test_packet_parse_rejects_missing_fields",
       test_packet_parse_rejects_missing_fields},
      {"test_packet_parse_rejects_oversized",
       test_packet_parse_rejects_oversized},
      {"test_packet_port_zero", test_packet_port_zero},
      {"test_packet_max_port", test_packet_max_port},
      // unique id
      {"test_unique_id_length", test_unique_id_length},
      {"test_unique_id_hex_chars", test_unique_id_hex_chars},
  };

  if (argc == 2) {
    auto it = tests.find(argv[1]);
    if (it == tests.end()) {
      return 1;
    }
    it->second();
    return 0;
  }

  int failed = 0;

  for (auto& [name, fn] : tests) {
    try {
      fn();
    } catch (...) {
      ++failed;
    }
  }

  return failed > 0;
}

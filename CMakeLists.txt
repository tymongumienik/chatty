cmake_minimum_required(VERSION 3.25)
project(chatty LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/arthursonzogni/ftxui.git
  GIT_TAG        v7.0.0
  GIT_PROGRESS   TRUE
  GIT_SHALLOW    TRUE
  EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(ftxui)

add_executable(chatty
  src/main.cpp
  src/app.cpp
  src/uniqueid.cpp
  src/networking/packets.cpp
  src/networking/client.cpp
  src/screens/name_input.cpp
  src/screens/waiting_for_peer.cpp
  src/screens/chat_screen.cpp
)

target_include_directories(chatty PRIVATE src)
include_directories(${PROJECT_SOURCE_DIR}/src)

target_link_libraries(chatty
  PRIVATE ftxui::screen
  PRIVATE ftxui::dom
  PRIVATE ftxui::component
)

# Tests

enable_testing()
add_executable(tests src/testing/tests.cpp)
add_test(NAME Tests COMMAND tests)

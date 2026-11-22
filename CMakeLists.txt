cmake_minimum_required(VERSION 3.16)
project(matching-engine
    VERSION   0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Library: the matching engine core (no I/O) ──────────────────────────────
add_library(orderbook
    src/order_book.cpp
)
target_include_directories(orderbook PUBLIC include)

# ── Executable: command-line front end ──────────────────────────────────────
add_executable(matching-engine src/main.cpp)
target_link_libraries(matching-engine PRIVATE orderbook)

# ── Testing ─────────────────────────────────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.2
)
FetchContent_MakeAvailable(Catch2)

enable_testing()
add_subdirectory(tests)

# ── Benchmarks ──────────────────────────────────────────────────────────────
add_subdirectory(benchmarks)

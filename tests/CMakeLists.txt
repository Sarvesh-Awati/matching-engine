# Tests using Catch2 v3
add_executable(orderbook_tests
    test_types.cpp
)
target_link_libraries(orderbook_tests PRIVATE orderbook Catch2::Catch2WithMain)

include(Catch)
catch_discover_tests(orderbook_tests)

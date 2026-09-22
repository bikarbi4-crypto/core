cmake_minimum_required(VERSION 3.20)
project(PlayerbotCumulativeTests LANGUAGES CXX)
enable_testing()
find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/action_contracts.h
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/extract_action_contracts.py ${CMAKE_CURRENT_BINARY_DIR}/action_contracts.h
    DEPENDS extract_action_contracts.py
        ../../src/game/PlayerBots/playerbot/strategy/Action.h
        ../../src/game/PlayerBots/playerbot/strategy/Action.cpp
        ../../src/game/PlayerBots/playerbot/strategy/Trigger.h
        ../../src/game/PlayerBots/playerbot/strategy/Trigger.cpp)
add_executable(action_lists_test action_lists_test.cpp ${CMAKE_CURRENT_BINARY_DIR}/action_contracts.h)
target_compile_features(action_lists_test PRIVATE cxx_std_17)
target_include_directories(action_lists_test PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
add_executable(context_cache_test context_cache_test.cpp)
target_compile_features(context_cache_test PRIVATE cxx_std_17)
target_include_directories(context_cache_test PRIVATE ../../src/game/PlayerBots/playerbot/strategy)
if(MSVC)
    target_compile_options(context_cache_test PRIVATE /W4 /WX /UNDEBUG)
    target_compile_options(action_lists_test PRIVATE /W4 /WX /UNDEBUG)
    option(VMANGOS_TEST_ASAN "Check the standalone context tests with AddressSanitizer" OFF)
    if(VMANGOS_TEST_ASAN)
        target_compile_options(context_cache_test PRIVATE /fsanitize=address)
        target_link_options(context_cache_test PRIVATE /INCREMENTAL:NO)
        target_compile_options(action_lists_test PRIVATE /fsanitize=address)
        target_link_options(action_lists_test PRIVATE /INCREMENTAL:NO)
    endif()
else()
    target_compile_options(context_cache_test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
    target_compile_options(action_lists_test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
endif()
add_test(NAME context_cache COMMAND context_cache_test)
add_test(NAME action_lists COMMAND action_lists_test)

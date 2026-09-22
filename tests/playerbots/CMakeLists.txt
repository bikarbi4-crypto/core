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
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/value_contracts.h
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/extract_value_contracts.py ${CMAKE_CURRENT_BINARY_DIR}/value_contracts.h
    DEPENDS extract_value_contracts.py
        ../../src/game/PlayerBots/playerbot/strategy/Value.h
        ../../src/game/PlayerBots/playerbot/strategy/values/AttackerCountValues.cpp
        ../../src/game/PlayerBots/playerbot/strategy/values/EnemyPlayerValue.cpp)
add_custom_target(value_contracts DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/value_contracts.h)
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/grind_contracts.h
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/extract_grind_contracts.py ${CMAKE_CURRENT_BINARY_DIR}/grind_contracts.h
    DEPENDS extract_grind_contracts.py
        ../../src/game/PlayerBots/playerbot/strategy/values/GrindTargetValue.h
        ../../src/game/PlayerBots/playerbot/strategy/values/GrindTargetValue.cpp)
add_custom_target(grind_contracts DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/grind_contracts.h)
foreach(target grind_target_test grind_target_benchmark)
    add_executable(${target} ${target}.cpp)
    add_dependencies(${target} grind_contracts)
    target_compile_features(${target} PRIVATE cxx_std_17)
    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX /UNDEBUG)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Werror -UNDEBUG)
    endif()
endforeach()
foreach(target value_metadata_test value_metadata_benchmark)
    add_executable(${target} ${target}.cpp)
    add_dependencies(${target} value_contracts)
    target_compile_features(${target} PRIVATE cxx_std_17)
    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX /UNDEBUG)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Werror -UNDEBUG)
    endif()
endforeach()
if(MSVC)
    target_compile_options(context_cache_test PRIVATE /W4 /WX /UNDEBUG)
    target_compile_options(action_lists_test PRIVATE /W4 /WX /UNDEBUG)
    option(VMANGOS_TEST_ASAN "Check the standalone context tests with AddressSanitizer" OFF)
    if(VMANGOS_TEST_ASAN)
        target_compile_options(grind_target_test PRIVATE /fsanitize=address)
        target_link_options(grind_target_test PRIVATE /INCREMENTAL:NO)
        target_compile_options(value_metadata_test PRIVATE /fsanitize=address)
        target_link_options(value_metadata_test PRIVATE /INCREMENTAL:NO)
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
add_test(NAME value_metadata COMMAND value_metadata_test)
add_test(NAME grind_target COMMAND grind_target_test)

#pragma once
#include <cstdint>
#include <string>

enum class Kind { boolean, u8, u32, floating, unit, list };
enum class Policy { calculated, manual, single, memory, logged };
enum class Mode { warm, refresh, qualified, missing, wrong_type };
enum class Operation { get, size, empty };
enum class Stage { full, lookup, cast };
struct Case {
    Kind kind=Kind::boolean;
    Policy policy=Policy::calculated;
    Mode mode=Mode::warm;
    Operation operation=Operation::get;
    Stage stage=Stage::full;
    unsigned listSize=8;
};
struct Variant {
    const char* name;
    void* (*create)(const Case&);
    void (*destroy)(void*);
    std::uint64_t (*run)(void*,std::uint64_t);
};
Variant v16_api();
Variant c_only_api();
Variant ab_only_api();
Variant v17_api();
#ifdef AUDIT_CANDIDATE
Variant candidate_api();
#endif

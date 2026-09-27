#pragma once
#include <cstdint>
#include <string>
#include <vector>
struct Case { unsigned kind=0, size=0, mode=0; bool memory=false; };
struct Observation { std::uint64_t result=0, copies=0, gets=0, checks=0; std::vector<std::string> trace; };
struct Api { const char* name; void* (*create)(Case); void (*destroy)(void*); std::uint64_t (*run)(void*,unsigned); Observation (*observe)(void*); void (*contracts)(); };
Api v21_api();Api v22_api();

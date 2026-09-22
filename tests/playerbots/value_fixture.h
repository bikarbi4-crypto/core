#pragma once
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

using uint8 = std::uint8_t;
using uint32 = std::uint32_t;
inline time_t fixtureTime = 10000;
inline std::uint64_t clockCalls = 0;
inline time_t FixtureTime(time_t*) { ++clockCalls; return fixtureTime; }

#ifdef VALUE_BENCHMARK
using ObjectGuid = std::uint64_t;
#else
struct ObjectGuid {
    std::uint64_t value = 0;
    inline static std::size_t copies = 0;
    ObjectGuid() = default;
    ObjectGuid(std::uint64_t id) : value(id) {}
    ObjectGuid(const ObjectGuid& other) : value(other.value) { ++copies; }
    ObjectGuid& operator=(const ObjectGuid& other) { value = other.value; ++copies; return *this; }
    bool operator==(const ObjectGuid& other) const { return value == other.value; }
};
#endif

class PlayerbotAI {};
struct AiNamedObject {
    PlayerbotAI* ai;
    std::string name;
    AiNamedObject(PlayerbotAI* owner, std::string label) : ai(owner), name(std::move(label)) {}
    std::string getName() { return name; }
};
struct UntypedValue : AiNamedObject {
    using AiNamedObject::AiNamedObject;
    virtual ~UntypedValue() = default;
    virtual void Reset() {}
    virtual std::string Format() { return "?"; }
    virtual bool Expired() { return false; }
    virtual bool Expired(uint32) { return false; }
    virtual bool Protected() { return false; }
    virtual uint32 LastChangeDelay() { return 0; }
};
struct PerformanceMonitorOperation {
    inline static std::uint64_t live = 0;
    PerformanceMonitorOperation() { ++live; }
    ~PerformanceMonitorOperation() { --live; }
};
struct TestConfig { bool perfMonEnabled = false; };
inline TestConfig sPlayerbotAIConfig;
constexpr int PERF_MON_VALUE = 0;
struct TestMonitor {
    std::uint64_t starts = 0;
    std::unique_ptr<PerformanceMonitorOperation> start(int, std::string, PlayerbotAI*) {
        ++starts;
        return std::make_unique<PerformanceMonitorOperation>();
    }
};
inline TestMonitor sPerformanceMonitor;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4100 4244 4267 4458) // Existing extracted policy/uint8 consumer declarations.
#endif
#define time FixtureTime
#include "value_contracts.h"
#undef time
#ifdef _MSC_VER
#pragma warning(pop)
#endif

template<class Base>
class ListProbe : public Base {
public:
    explicit ListProbe(int interval) : Base(nullptr, "probe", interval) {}
    std::size_t generatedSize = 0;
    unsigned calculates = 0;
    std::function<void()> onCalculate;
    std::list<ObjectGuid> Calculate() override {
        ++calculates;
        if (onCalculate) onCalculate();
        std::list<ObjectGuid> out;
        for (std::size_t i = 0; i < generatedSize; ++i) out.emplace_back(i + 1);
        return out;
    }
    time_t LastCheck() const { return this->lastCheckTime; }
};

using OldList = ListProbe<v16::ObjectGuidListCalculatedValue>;
using NewList = ListProbe<v17::ObjectGuidListCalculatedValue>;

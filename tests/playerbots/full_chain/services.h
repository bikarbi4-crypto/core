#pragma once
#include "api.h"
#include <algorithm>
#include <cctype>
#include <ctime>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <type_traits>
#include <cstdlib>
#include <vector>
using uint8=std::uint8_t;
using int8=std::int8_t;
using uint32=std::uint32_t;
using int32=std::int32_t;
using uint64=std::uint64_t;
class PlayerbotAI {};
class Player {};
class Unit { public: const char* GetName() const { return "unit"; } };
struct ObjectGuid { std::uint64_t raw=0; };
static_assert(sizeof(ObjectGuid)==8);
struct PerformanceMonitorOperation { ~PerformanceMonitorOperation(); };
struct AuditConfig { bool perfMonEnabled=false; };
struct AuditMonitor {
    std::unique_ptr<PerformanceMonitorOperation> start(int,const std::string&,PlayerbotAI*);
};
extern AuditConfig sPlayerbotAIConfig;
extern AuditMonitor sPerformanceMonitor;
constexpr int PERF_MON_VALUE=1;
extern Unit auditUnit;

template<class T> T Initial(unsigned) { return static_cast<T>(7); }
template<> inline bool Initial<bool>(unsigned) { return true; }
template<> inline Unit* Initial<Unit*>(unsigned) { return &auditUnit; }
template<> inline std::list<ObjectGuid> Initial<std::list<ObjectGuid>>(unsigned n) {
    return std::list<ObjectGuid>(n,ObjectGuid{7});
}

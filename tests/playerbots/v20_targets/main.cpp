#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <list>
#include <new>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace allocations {
thread_local bool enabled = false;
thread_local std::uint64_t count = 0, bytes = 0, releases = 0;
}
#ifdef TARGETS_COUNT_ALLOC
void* operator new(std::size_t bytes) {
    void* p = std::malloc(bytes ? bytes : 1);
    if (!p) throw std::bad_alloc();
    if (allocations::enabled) { ++allocations::count; allocations::bytes += bytes; }
    return p;
}
void operator delete(void* p) noexcept {
    if (p && allocations::enabled) ++allocations::releases;
    std::free(p);
}
void* operator new[](std::size_t n) { return ::operator new(n); }
void operator delete[](void* p) noexcept { ::operator delete(p); }
void operator delete(void* p, std::size_t) noexcept { ::operator delete(p); }
void operator delete[](void* p, std::size_t) noexcept { ::operator delete(p); }
#endif

using ObjectGuid = std::uint64_t;
using std::stoi;
constexpr int ALL_ACTIVITY = 7;
struct Config { float sightDistance = 60.0f; } sPlayerbotAIConfig;
struct State {
    bool valid, ignoreFirst, ignoreSecond, breakable, unbreakable;
};
struct Unit { ObjectGuid guid; State state; int ignoreCalls = 0; };
struct World {
    std::array<Unit, 32> units{};
    bool traceEnabled = true, activity = true, inWorld = true, teleported = false;
    bool pmoEnabled = false; // This function has no PMO-dependent branch.
    int throwAt = -1, events = 0, reenterAt = -1;
    std::vector<std::uint64_t> trace;
    std::list<ObjectGuid> one, all;
    std::function<void()> reentry;
    World() {
        for (unsigned i = 0; i < units.size(); ++i)
            units[i] = {i + 1, {bool(i & 1), bool(i & 2), bool(i & 4), bool(i & 8), bool(i & 16)}};
    }
    void event(unsigned kind, ObjectGuid guid = 0) {
        if (!traceEnabled) return;
        trace.push_back((std::uint64_t(kind) << 32) | guid);
        int ordinal = events++;
        if (ordinal == reenterAt && reentry) reentry();
        if (ordinal == throwAt) throw std::runtime_error("fixture predicate exception");
    }
    void reset() {
        trace.clear(); events = 0;
        for (auto& unit : units) unit.ignoreCalls = 0;
    }
};
struct Player {
    World* world;
    bool IsInWorld() { world->event(10); return world->inWorld; }
    bool IsBeingTeleported() { world->event(11); return world->teleported; }
};
struct AI {
    World* world;
    bool AllowActivity(int kind) { assert(kind == ALL_ACTIVITY); world->event(9); return world->activity; }
    Unit* GetUnit(ObjectGuid guid) {
        world->event(1, guid);
        if (!guid) return nullptr;
        assert(guid <= world->units.size());
        Unit* unit = &world->units[guid - 1];
        unit->ignoreCalls = 0;
        return unit;
    }
    std::list<ObjectGuid> attackers(bool one) {
        world->event(one ? 12 : 13);
        return one ? world->one : world->all;
    }
};
struct Subject {
    AI* ai;
    Player* bot;
    std::string qualifier;
    bool IsValid(Unit* target, Player* player, float range, bool ignoreCC, bool checkAttackerValid) {
        assert(player == bot && range == 60.0f && ignoreCC && !checkAttackerValid);
        ai->world->event(2, target ? target->guid : 0);
        return target && target->state.valid;
    }
    bool HasIgnoreCCRti(Unit* target, Player* player) {
        assert(player == bot); ai->world->event(3, target->guid);
        return target->ignoreCalls++ == 0 ? target->state.ignoreFirst : target->state.ignoreSecond;
    }
    bool HasBreakableCC(Unit* target, Player* player) {
        assert(player == bot); ai->world->event(4, target->guid); return target->state.breakable;
    }
    bool HasUnBreakableCC(Unit* target, Player* player) {
        assert(player == bot); ai->world->event(5, target->guid); return target->state.unbreakable;
    }
};
#define AI_VALUE(type, name) ai->attackers(false)
#define AI_VALUE2(type, name, qualifier) ai->attackers(true)
namespace v19 {
struct PossibleAttackTargetsValue : Subject {
    std::list<ObjectGuid> Calculate();
    void RemoveNonThreating(std::list<ObjectGuid>&, bool);
};
#include "v19.inc"
}
namespace v20 {
struct PossibleAttackTargetsValue : Subject {
    std::list<ObjectGuid> Calculate();
    void RemoveNonThreating(std::list<ObjectGuid>&, bool);
};
#include "v20.inc"
}
#undef AI_VALUE
#undef AI_VALUE2

template<class T> T makeSubject(AI& ai, Player& bot, std::string qualifier = {}) {
    T subject; subject.ai = &ai; subject.bot = &bot; subject.qualifier = std::move(qualifier); return subject;
}
std::uint64_t observations = 0;
std::uint64_t checksum(const std::list<ObjectGuid>& values) {
    std::uint64_t hash = 0;
    for (auto guid : values) hash = hash * 37 + guid;
    return hash ^ values.size();
}
void compare(const std::vector<ObjectGuid>& input, bool getOne, bool pmo, int throwAt = -1, int reenterAt = -1) {
    World first, second;
    first.pmoEnabled = second.pmoEnabled = pmo;
    first.throwAt = second.throwAt = throwAt;
    first.reenterAt = second.reenterAt = reenterAt;
    AI a{&first}, b{&second}; Player pa{&first}, pb{&second};
    auto x = makeSubject<v19::PossibleAttackTargetsValue>(a, pa);
    auto y = makeSubject<v20::PossibleAttackTargetsValue>(b, pb);
    std::list<ObjectGuid> old(input.begin(), input.end()), now(input.begin(), input.end());
    std::vector<ObjectGuid> nestedOld, nestedNew;
    first.reentry = [&] {
        first.reenterAt = -1;
        std::list<ObjectGuid> list{10, 18, 10, 2};
        x.RemoveNonThreating(list, false);
        nestedOld.assign(list.begin(), list.end());
    };
    second.reentry = [&] {
        second.reenterAt = -1;
        std::list<ObjectGuid> list{10, 18, 10, 2};
        y.RemoveNonThreating(list, false);
        nestedNew.assign(list.begin(), list.end());
    };
    bool oldThrew = false, newThrew = false;
    try { x.RemoveNonThreating(old, getOne); } catch (const std::runtime_error&) { oldThrew = true; }
    try { y.RemoveNonThreating(now, getOne); } catch (const std::runtime_error&) { newThrew = true; }
    assert(old == now && first.trace == second.trace && oldThrew == newThrew && nestedOld == nestedNew);
    ++observations;
}

void tests() {
    // Exhaust all valid/invalid, both RTI answers, and both CC predicate results,
    // including duplicates, absent GUID, early getOne and all-CC multi-target fallback.
    std::vector<ObjectGuid> input;
    for (unsigned length = 0; length <= 3; ++length) {
        std::uint64_t combinations = 1;
        for (unsigned i = 0; i < length; ++i) combinations *= 33;
        for (std::uint64_t number = 0; number < combinations; ++number) {
            input.clear(); auto n = number;
            for (unsigned i = 0; i < length; ++i) { input.push_back(n % 33); n /= 33; }
            for (bool getOne : {false, true}) compare(input, getOne, bool(number & 1));
        }
    }
    std::mt19937 random(0x20C19);
    for (unsigned trial = 0; trial < 12000; ++trial) {
        input.clear(); unsigned length = random() % 257;
        for (unsigned i = 0; i < length; ++i) input.push_back(random() % 33);
        compare(input, trial & 1, trial & 2);
    }
    input = {0, 10, 18, 10, 2, 18, 32, 2};
    for (bool getOne : {false, true}) for (bool pmo : {false, true}) {
        for (int at = 0; at < 55; ++at) {
            compare(input, getOne, pmo, at);
            compare(input, getOne, pmo, -1, at);
        }
    }
    // Actual Calculate body: permission/world guards, qualifier conversion, one
    // attackers attempt, full-list retry, empty and CC-only results.
    for (unsigned flags = 0; flags < 16; ++flags)
        for (const std::string qualifier : {"", "0", "1", "2", "-1", "bad", "999999999999999999"})
            for (const std::vector<ObjectGuid> one : {std::vector<ObjectGuid>{}, {0}, {10, 18, 10}, {2}, {0, 2, 10}})
                for (const std::vector<ObjectGuid> all : {std::vector<ObjectGuid>{}, {0}, {10, 18, 10}, {2, 2}, {0, 2, 10}}) {
                    World w[2]; AI ai[2]{{&w[0]}, {&w[1]}}; Player p[2]{{&w[0]}, {&w[1]}};
                    for (auto& world : w) {
                        world.activity = flags & 1; world.inWorld = flags & 2;
                        world.teleported = flags & 4; world.pmoEnabled = flags & 8;
                        world.one.assign(one.begin(), one.end()); world.all.assign(all.begin(), all.end());
                    }
                    auto old = makeSubject<v19::PossibleAttackTargetsValue>(ai[0], p[0], qualifier);
                    auto now = makeSubject<v20::PossibleAttackTargetsValue>(ai[1], p[1], qualifier);
                    std::list<ObjectGuid> a, b; int ea = 0, eb = 0;
                    try { a = old.Calculate(); } catch (const std::invalid_argument&) { ea = 1; } catch (const std::out_of_range&) { ea = 2; }
                    try { b = now.Calculate(); } catch (const std::invalid_argument&) { eb = 1; } catch (const std::out_of_range&) { eb = 2; }
                    assert(a == b && w[0].trace == w[1].trace && ea == eb); ++observations;
                }
    // Known behavior, independently of baseline parity: unbreakable CC preferred;
    // getOne fallback intentionally returns every CC target in that category.
    World world; AI ai{&world}; Player bot{&world};
    auto now = makeSubject<v20::PossibleAttackTargetsValue>(ai, bot);
    std::list<ObjectGuid> cc{10, 18, 18, 10};
    const ObjectGuid* node = &*std::next(cc.begin());
    now.RemoveNonThreating(cc, true);
    assert((cc == std::list<ObjectGuid>{18, 18}) && &cc.front() == node);
    std::list<ObjectGuid> ordinary{10, 2, 18, 2};
    node = &*std::next(ordinary.begin());
    now.RemoveNonThreating(ordinary, true);
    assert((ordinary == std::list<ObjectGuid>{2}) && &ordinary.front() == node);
    std::cout << "PASS " << observations << " paired observations; exact GUID/order/predicate traces; exception/reentry; Calculate wrapper; node identity\n";
}

struct Fixture { std::string name; std::vector<ObjectGuid> input; bool getOne; };
std::vector<Fixture> fixtures() {
    std::vector<Fixture> out;
    for (unsigned length : {0, 1, 8, 64}) for (unsigned pattern = 0; pattern < 5; ++pattern)
        for (bool one : {false, true}) {
            Fixture f{std::to_string(length) + "_" + std::to_string(pattern), {}, one};
            for (unsigned i = 0; i < length; ++i)
                f.input.push_back(pattern == 0 ? 2 : pattern == 1 ? 10 : pattern == 2 ? 18 : pattern == 3 ? 0 : std::array<ObjectGuid, 6>{0, 10, 18, 10, 2, 18}[i % 6]);
            out.push_back(std::move(f));
        }
    return out;
}
template<class T> void allocationRow(std::ostream& out, const Fixture& f, const char* label) {
    World world; world.traceEnabled = false; AI ai{&world}; Player bot{&world};
    auto value = makeSubject<T>(ai, bot);
    std::list<ObjectGuid> input(f.input.begin(), f.input.end());
    allocations::count = allocations::bytes = allocations::releases = 0;
    allocations::enabled = true;
    value.RemoveNonThreating(input, f.getOne);
    allocations::enabled = false;
    if (std::string(label) == "v20") assert(allocations::count == 2); // MSVC std::list head nodes only.
    out << f.name << ',' << f.getOne << ',' << label << ',' << allocations::count << ',' << allocations::bytes << ',' << allocations::releases << ',' << input.size() << ',' << checksum(input) << '\n';
}
volatile std::uint64_t benchmarkSink = 0;
template<class T> double batch(const Fixture& f, bool cold, unsigned count) {
    World world; world.traceEnabled = false; AI ai{&world}; Player bot{&world};
    auto value = makeSubject<T>(ai, bot);
    std::vector<std::list<ObjectGuid>> batch;
    if (!cold) {
        batch.reserve(count);
        for (unsigned i = 0; i < count; ++i) batch.emplace_back(f.input.begin(), f.input.end());
    }
    std::uint64_t sum = 0;
    auto start = std::chrono::steady_clock::now();
    for (unsigned i = 0; i < count; ++i) {
        if (cold) {
            std::list<ObjectGuid> input(f.input.begin(), f.input.end());
            value.RemoveNonThreating(input, f.getOne);
            sum += checksum(input);
        } else {
            value.RemoveNonThreating(batch[i], f.getOne);
            sum += checksum(batch[i]);
        }
    }
    auto finish = std::chrono::steady_clock::now();
    benchmarkSink ^= sum;
    return std::chrono::duration<double, std::nano>(finish - start).count() / count;
}
void benchmark(const std::string& path) {
    std::ofstream output(path);
    output << "fixture,get_one,mode,round,order,variant,iterations,ns\n";
    for (const auto& fixture : fixtures()) {
        for (bool cold : {false, true}) {
            // Calibrate the faster side to >= 6 ms. Both variants do exactly the same work.
            unsigned count = 1024;
            for (;;) {
                double old = batch<v19::PossibleAttackTargetsValue>(fixture, cold, count);
                double now = batch<v20::PossibleAttackTargetsValue>(fixture, cold, count);
                if (std::min(old, now) * count >= 6000000.0 || count >= 65536) break;
                count *= 2;
            }
            for (unsigned round = 0; round < 9; ++round) {
                double old, now;
                if (round % 2) {
                    now = batch<v20::PossibleAttackTargetsValue>(fixture, cold, count);
                    old = batch<v19::PossibleAttackTargetsValue>(fixture, cold, count);
                } else {
                    old = batch<v19::PossibleAttackTargetsValue>(fixture, cold, count);
                    now = batch<v20::PossibleAttackTargetsValue>(fixture, cold, count);
                }
                output << fixture.name << ',' << fixture.getOne << ',' << (cold ? "cold" : "warm") << ',' << round << ',' << (round % 2 ? "candidate-first" : "baseline-first") << ",v19," << count << ',' << old << '\n';
                output << fixture.name << ',' << fixture.getOne << ',' << (cold ? "cold" : "warm") << ',' << round << ',' << (round % 2 ? "candidate-first" : "baseline-first") << ",v20," << count << ',' << now << '\n';
            }
            std::cerr << "completed " << fixture.name << ',' << fixture.getOne << ',' << cold << '\n';
        }
    }
}
int main(int argc, char** argv) {
    if (argc == 3 && std::string(argv[1]) == "--benchmark") benchmark(argv[2]);
#ifdef TARGETS_COUNT_ALLOC
    else if (argc == 3 && std::string(argv[1]) == "--allocations") {
        std::ofstream output(argv[2]);
        output << "fixture,get_one,variant,allocations,bytes,releases,result_size,checksum\n";
        for (const auto& fixture : fixtures()) {
            allocationRow<v19::PossibleAttackTargetsValue>(output, fixture, "v19");
            allocationRow<v20::PossibleAttackTargetsValue>(output, fixture, "v20");
        }
    }
#endif
    else tests();
}

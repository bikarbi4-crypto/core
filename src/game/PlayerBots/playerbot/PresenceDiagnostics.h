#ifndef PLAYERBOT_PRESENCE_DIAGNOSTICS_H
#define PLAYERBOT_PRESENCE_DIAGNOSTICS_H

// Diagnostic observation only. No Map/Player pointers cross a thread boundary.
#include <array>
#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

namespace PresenceDiagnostics
{
constexpr unsigned PriorityCount = 18, ActivityCount = 10, AllActivity = 9;
constexpr unsigned MaxThreads = 64, MaxBotsPerThread = 16384, MaxMapsPerThread = 128;
extern std::atomic<std::uint64_t> activeEpoch;
inline bool Enabled() { return activeEpoch.load(std::memory_order_acquire) != 0; }

struct BotState
{
    std::uint64_t epoch = 0, lifetime = 0, revision = 0;
    std::int64_t lastOrdinaryRefresh = 0;
    unsigned priority = PriorityCount;
    bool allowed = false;
};

struct Counters
{
    std::array<std::uint64_t, PriorityCount> priorities{};
    std::array<std::uint64_t, PriorityCount * ActivityCount> allowed{}, denied{};
    std::array<std::uint64_t, PriorityCount * PriorityCount> transitions{};
    std::array<std::uint64_t, 7> activitySources{};
    std::uint64_t snapshots = 0, copiedEntries = 0, friendChecks = 0;
    std::uint64_t mapScans = 0, scannedEntries = 0;
    std::uint64_t cacheHits = 0, ordinaryRefreshes = 0, forcedRefreshes = 0;
    std::uint64_t cacheAllowed = 0, cacheDenied = 0;
    std::uint64_t timedCalls = 0, priorityNs = 0, emptyClockNs = 0;
    std::uint64_t droppedBotObservations = 0, droppedMapObservations = 0;
};

struct MapSample
{
    std::uint32_t map = 0, instance = 0, samples = 0;
    std::uint64_t observedMs = 0;
    std::uint32_t entries = 0, realByPriority = 0, realByIsBot = 0;
    float currentMs = 0, localA = -1;
    unsigned source = 0, wantedIfLocalEnabled = 0;
    // Counts of in-world real players by the exact priority predicate; no names/GUIDs.
    std::array<std::uint32_t, 32> zones{}, zonePlayers{};
    unsigned zoneCount = 0, droppedZones = 0;
    void Player(bool inWorld, bool priorityReal, bool isBotReal, std::uint32_t zone);
};

struct ScaleSample
{
    std::uint32_t map = 0, instance = 0, wanted = 0, samples = 0;
    std::uint64_t observedMs = 0;
    float current = 0, before = 0, after = 0;
};

struct Shard;
class PriorityProbe
{
public:
    PriorityProbe() { if (Enabled()) Begin(); }
    void Snapshot(std::size_t n) { if (counts) { ++counts->snapshots; counts->copiedEntries += n; } }
    void Friend() { if (counts) ++counts->friendChecks; }
    void Scan() { if (counts) ++counts->mapScans; }
    void Entry() { if (counts) ++counts->scannedEntries; }
    template<class T> T Finish(T result) { if (counts) FinishImpl(static_cast<unsigned>(result)); return result; }
private:
    void FinishImpl(unsigned result);
    void Begin();
    Shard* shard = nullptr;
    Counters* counts = nullptr;
    std::uint64_t started = 0;
};

class DecisionProbe
{
public:
    DecisionProbe() { if (Enabled()) BeginThread(); }
    explicit operator bool() const { return shard != nullptr; }
    void Begin(std::uint32_t id, BotState& state, unsigned activity, unsigned priority);
    bool Finish(bool allowed);
private:
    void BeginThread();
    Shard* shard = nullptr;
    BotState* state = nullptr;
    std::uint32_t id = 0;
    unsigned activity = 0, priority = 0;
};

void Cache(std::uint32_t id, BotState& state, bool hit, bool forced, bool allowed, std::int64_t existingTimestamp);
void Source(unsigned source);
inline float ActivityValue(float value, unsigned source)
{
    if (Enabled()) Source(source);
    return value;
}
// Called only at the existing map-owner loop, at most once per map per 5 s.
bool BeginMap(MapSample& sample, std::uint32_t map, std::uint32_t instance);
void EndMap(MapSample const& sample);
void Scale(ScaleSample sample, bool global);
void Flush(); // Owner-thread publication; only a rare copy takes a lock.

// Control functions below run on the world thread, via local console only.
std::string Start(unsigned seconds, bool timing, bool pmo);
std::string Status();
std::vector<std::string> Stop(char const* reason);
std::vector<std::string> Poll(bool pmo);
// Public for standalone collector tests and explicit stop; reports published snapshots.
std::vector<std::string> Report();
}
#endif

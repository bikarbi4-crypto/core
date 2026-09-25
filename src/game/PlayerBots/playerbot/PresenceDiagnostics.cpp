#include "PresenceDiagnostics.h"
#include <algorithm>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <unordered_map>
#include <utility>

namespace PresenceDiagnostics
{
using Clock = std::chrono::steady_clock;
static std::uint64_t Ns()
{
#ifdef PRESENCE_TEST_CLOCK
    extern std::uint64_t TestClockNs();
    return TestClockNs();
#else
    return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now().time_since_epoch()).count();
#endif
}
static std::uint64_t Ms() { return Ns() / 1000000; }
// A worker may publish after Report captured its start time but before copying
// that worker's snapshot. Such a snapshot is fresh, not an unsigned age wrap.
static std::uint64_t Age(std::uint64_t now, std::uint64_t observed) { return now >= observed ? now - observed : 0; }
static std::uint64_t Key(std::uint32_t map, std::uint32_t instance) { return (std::uint64_t(map) << 32) | instance; }
std::atomic<std::uint64_t> activeEpoch{0};
static std::atomic<std::uint64_t> lifetimeSequence{0}; // One increment per observed bot lifetime/run, never per hot call.
static std::atomic<bool> sampleTiming{false};

struct Snapshot
{
    std::uint64_t epoch = 0, publishedMs = 0, publicationNs = 0, publications = 0;
    Counters counts;
    std::unordered_map<std::uint32_t, BotState> bots;
    std::map<std::uint64_t, MapSample> maps;
    std::map<std::uint64_t, ScaleSample> scales;
    ScaleSample global;
};
struct Published { std::mutex mutex; Snapshot data; };
struct Registry
{
    std::mutex mutex;
    std::vector<std::shared_ptr<Published>> shards;
    std::uint64_t epoch = 0, started = 0, deadline = 0, nextReport = 0;
    unsigned droppedThreads = 0;
};
static Registry& Store() { static Registry registry; return registry; }
struct Shard
{
    Snapshot data;
    std::shared_ptr<Published> published;
    unsigned callsSinceCheck = 0;
    std::uint64_t priorityCalls = 0;
    bool timing = false;
};
// MSVC eagerly constructs namespace-scope TLS objects on thread startup. Empty
// map/list sentinels would allocate even while diagnostics is OFF. Defer the
// shard itself until the first enabled observation, and keep its address stable.
static thread_local std::unique_ptr<Shard> local;

static Shard* Current()
{
    auto const epoch = activeEpoch.load(std::memory_order_acquire);
    if (!epoch) return nullptr;
    if (!local) local = std::make_unique<Shard>();
    auto& shard = *local;
    if (shard.data.epoch != epoch)
    {
        shard = Shard{};
        shard.data.epoch = epoch;
        shard.timing = sampleTiming.load(std::memory_order_relaxed);
        auto& registry = Store();
        std::lock_guard<std::mutex> lock(registry.mutex);
        if (registry.epoch != epoch) return nullptr;
        if (registry.shards.size() == MaxThreads) { ++registry.droppedThreads; return nullptr; }
        shard.published = std::make_shared<Published>();
        registry.shards.push_back(shard.published);
    }
    return shard.published ? &shard : nullptr;
}
static void Publish(Shard& shard, bool force)
{
    auto const now = Ms();
    if (!force && now < shard.data.publishedMs + 5000) return;
    auto const start = Ns();
    shard.data.publishedMs = now;
    ++shard.data.publications;
    {
        std::lock_guard<std::mutex> lock(shard.published->mutex);
        shard.published->data = shard.data;
    }
    // The following publication includes this cost; the last cost is not yet included.
    shard.data.publicationNs += Ns() - start;
}
static void MaybePublish(Shard& shard)
{
    if (++shard.callsSinceCheck == 256) { shard.callsSinceCheck = 0; Publish(shard, false); }
}
void Flush() { if (auto* shard = Current()) Publish(*shard, false); }

static void Prepare(Shard& shard, BotState& state)
{
    if (state.epoch != shard.data.epoch)
    {
        state = BotState{};
        state.epoch = shard.data.epoch;
        state.lifetime = lifetimeSequence.fetch_add(1, std::memory_order_relaxed) + 1;
    }
}
static void Observe(Shard& shard, std::uint32_t id, BotState const& state)
{
    if (shard.data.bots.size() >= MaxBotsPerThread && !shard.data.bots.count(id))
        ++shard.data.counts.droppedBotObservations;
    else shard.data.bots[id] = state;
    MaybePublish(shard);
}

void PriorityProbe::Begin()
{
    shard = Current();
    if (!shard) return;
    counts = &shard->data.counts;
    if (shard->timing && (++shard->priorityCalls % 1024 == 0))
    {
        auto const a = Ns();
        auto const b = Ns();
        counts->emptyClockNs += b - a;
        started = Ns();
    }
}
void PriorityProbe::FinishImpl(unsigned result)
{
    if (started) { counts->priorityNs += Ns() - started; ++counts->timedCalls; }
    if (result < PriorityCount) ++counts->priorities[result];
    MaybePublish(*shard);
}
void DecisionProbe::BeginThread() { shard = Current(); }
void DecisionProbe::Begin(std::uint32_t botId, BotState& botState, unsigned a, unsigned p)
{
    id = botId; state = &botState; activity = a; priority = p;
}
bool DecisionProbe::Finish(bool allowed)
{
    if (!shard || !state || activity >= ActivityCount || priority >= PriorityCount) return allowed;
    auto& counts = shard->data.counts;
    ++(allowed ? counts.allowed : counts.denied)[activity * PriorityCount + priority];
    Prepare(*shard, *state);
    if (activity == AllActivity)
    {
        if (state->priority < PriorityCount && state->priority != priority)
            ++counts.transitions[state->priority * PriorityCount + priority];
        state->priority = priority; state->allowed = allowed; ++state->revision;
    }
    Observe(*shard, id, *state);
    return allowed;
}
void Cache(std::uint32_t id, BotState& state, bool hit, bool forced, bool allowed, std::int64_t timestamp)
{
    auto* shard = Current();
    if (!shard) return;
    auto& c = shard->data.counts;
    if (hit) ++c.cacheHits;
    else if (forced) ++c.forcedRefreshes;
    else ++c.ordinaryRefreshes;
    ++(allowed ? c.cacheAllowed : c.cacheDenied);
    Prepare(*shard, state);
    if (!hit && !forced) state.lastOrdinaryRefresh = timestamp;
    ++state.revision;
    Observe(*shard, id, state);
}
void Source(unsigned source)
{
    if (auto* shard = Current())
        if (source < shard->data.counts.activitySources.size()) ++shard->data.counts.activitySources[source];
}
void MapSample::Player(bool inWorld, bool priorityReal, bool isBotReal, std::uint32_t zone)
{
    ++entries;
    if (isBotReal) ++realByIsBot;
    if (!inWorld || !priorityReal) return;
    ++realByPriority;
    unsigned i = 0;
    for (; i < zoneCount; ++i) if (zones[i] == zone) break;
    if (i == zones.size()) { ++droppedZones; return; }
    if (i == zoneCount) { zones[i] = zone; ++zoneCount; }
    ++zonePlayers[i];
}
bool BeginMap(MapSample& sample, std::uint32_t map, std::uint32_t instance)
{
    auto* shard = Current();
    if (!shard) return false;
    auto const key = Key(map, instance);
    auto const now = Ms();
    auto const it = shard->data.maps.find(key);
    if (it != shard->data.maps.end() && now < it->second.observedMs + 5000) return false;
    if (it == shard->data.maps.end() && shard->data.maps.size() == MaxMapsPerThread)
    { ++shard->data.counts.droppedMapObservations; return false; }
    sample = MapSample{}; sample.map = map; sample.instance = instance; sample.observedMs = now;
    return true;
}
void EndMap(MapSample const& sample)
{
    if (auto* shard = Current())
    {
        shard->data.maps[Key(sample.map, sample.instance)] = sample;
        Publish(*shard, false);
    }
}
void Scale(ScaleSample sample, bool global)
{
    if (auto* shard = Current())
    {
        sample.observedMs = Ms();
        if (global) shard->data.global = sample;
        else
        {
            auto const key = Key(sample.map, sample.instance);
            if (shard->data.scales.size() < MaxMapsPerThread || shard->data.scales.count(key))
                shard->data.scales[key] = sample;
            else ++shard->data.counts.droppedMapObservations;
        }
    }
}

std::string Start(unsigned seconds, bool timing, bool pmo)
{
    if (Enabled()) return "[PRESENCE] already running; use status/stop";
    if (pmo) return "[PRESENCE] refused: PMO must already be OFF (no settings changed)";
    if (seconds < 30 || seconds > 3600) return "[PRESENCE] duration must be 30..3600 seconds";
    auto& r = Store();
    std::lock_guard<std::mutex> lock(r.mutex);
    r.shards.clear(); r.droppedThreads = 0;
    r.started = Ms(); r.deadline = r.started + seconds * 1000ULL; r.nextReport = r.started;
    sampleTiming.store(timing, std::memory_order_relaxed);
    activeEpoch.store(++r.epoch, std::memory_order_release);
    return "[PRESENCE] started seconds=" + std::to_string(seconds) + " timing_1_in_1024=" + std::to_string(timing);
}
std::string Status()
{
    if (!Enabled()) return "[PRESENCE] OFF";
    auto const now = Ms();
    return "[PRESENCE] ON remaining_seconds=" + std::to_string(now < Store().deadline ? (Store().deadline - now)/1000 : 0);
}

template<std::size_t N> static void Add(std::array<std::uint64_t, N>& a, std::array<std::uint64_t, N> const& b)
{ for (std::size_t i = 0; i < N; ++i) a[i] += b[i]; }
std::vector<std::string> Report()
{
    auto const reportStart = Ns();
    auto const now = reportStart / 1000000;
    std::vector<std::shared_ptr<Published>> published;
    std::uint64_t epoch, elapsed; unsigned droppedThreads;
    {
        auto& r = Store(); std::lock_guard<std::mutex> lock(r.mutex);
        published = r.shards; epoch = r.epoch; elapsed = now-r.started; droppedThreads = r.droppedThreads;
    }
    Snapshot merged; std::uint64_t oldest = now, publications = 0, publicationNs = 0; unsigned ready = 0;
    for (auto const& p : published)
    {
        Snapshot s;
        { std::lock_guard<std::mutex> lock(p->mutex); s = p->data; }
        if (s.epoch != epoch || !s.publishedMs) continue;
        ++ready; oldest = std::min(oldest, s.publishedMs);
        publications += s.publications; publicationNs += s.publicationNs;
        auto& a = merged.counts; auto const& b = s.counts;
        Add(a.priorities, b.priorities); Add(a.allowed, b.allowed); Add(a.denied, b.denied);
        Add(a.transitions, b.transitions); Add(a.activitySources, b.activitySources);
#define ADD_COUNT(n) a.n += b.n
        ADD_COUNT(snapshots); ADD_COUNT(copiedEntries); ADD_COUNT(friendChecks); ADD_COUNT(mapScans); ADD_COUNT(scannedEntries);
        ADD_COUNT(cacheHits); ADD_COUNT(ordinaryRefreshes); ADD_COUNT(forcedRefreshes); ADD_COUNT(cacheAllowed); ADD_COUNT(cacheDenied);
        ADD_COUNT(timedCalls); ADD_COUNT(priorityNs); ADD_COUNT(emptyClockNs); ADD_COUNT(droppedBotObservations); ADD_COUNT(droppedMapObservations);
#undef ADD_COUNT
        for (auto const& item : s.bots)
        {
            auto& dst = merged.bots[item.first]; auto const& src = item.second;
            if (std::make_pair(src.lifetime, src.revision) > std::make_pair(dst.lifetime, dst.revision)) dst = src;
        }
        for (auto const& item : s.maps)
            if (item.second.observedMs > merged.maps[item.first].observedMs) merged.maps[item.first] = item.second;
        for (auto const& item : s.scales)
            if (item.second.observedMs > merged.scales[item.first].observedMs) merged.scales[item.first] = item.second;
        if (s.global.observedMs > merged.global.observedMs) merged.global = s.global;
    }
    auto const& c = merged.counts;
    unsigned mapRealSum = 0;
    for (auto const& item : merged.maps) mapRealSum += item.second.realByPriority;
    std::vector<std::string> lines;
    auto emit = [&](std::string const& kind, std::string const& values) {
        lines.push_back("[PRESENCE] epoch=" + std::to_string(epoch) + " elapsed_ms=" + std::to_string(elapsed) + " " + kind + " " + values);
    };
    std::ostringstream out;
    out << "published_threads=" << ready << " registered_threads=" << published.size() << " dropped_threads=" << droppedThreads
        << " oldest_publication_age_ms=" << Age(now, oldest) << " unique_observed_bots=" << merged.bots.size()
        << " map_snapshot_real_sum=" << mapRealSum
        << " dropped_bot_observations=" << c.droppedBotObservations << " dropped_map_observations=" << c.droppedMapObservations
        << " publications=" << publications << " publication_ns=" << publicationNs << " counts=cumulative_published_tail_may_be_missing";
    emit("coverage", out.str());
    out.str("");
    out << "snapshots=" << c.snapshots << " copied_entries=" << c.copiedEntries << " friend_checks=" << c.friendChecks
        << " map_scans=" << c.mapScans << " scanned_entries=" << c.scannedEntries << " timed_calls=" << c.timedCalls
        << " sampled_priority_ns=" << c.priorityNs << " empty_clock_pair_ns=" << c.emptyClockNs;
    emit("cost", out.str());
    out.str("");
    out << "hits=" << c.cacheHits << " ordinary_refreshes=" << c.ordinaryRefreshes << " forced_refreshes=" << c.forcedRefreshes
        << " allowed_calls=" << c.cacheAllowed << " denied_calls=" << c.cacheDenied;
    std::int64_t oldestRefresh = INT64_MAX, newestRefresh = 0; unsigned unknown = 0;
    for (auto const& item : merged.bots)
    {
        auto const t = item.second.lastOrdinaryRefresh;
        if (!t) ++unknown;
        else { oldestRefresh = std::min(oldestRefresh,t); newestRefresh = std::max(newestRefresh,t); }
    }
    out << " last_ordinary_refresh_unix_min=" << (newestRefresh ? oldestRefresh : 0)
        << " last_ordinary_refresh_unix_max=" << newestRefresh << " unobserved_refresh_bots=" << unknown;
    emit("all_activity_cache", out.str());
    for (unsigned p = 0; p < PriorityCount; ++p)
    {
        std::uint64_t allAllow = c.allowed[AllActivity*PriorityCount+p], allDeny = c.denied[AllActivity*PriorityCount+p];
        std::uint64_t allow = 0, deny = 0; unsigned unique = 0;
        for (unsigned a = 0; a < ActivityCount; ++a) { allow += c.allowed[a*PriorityCount+p]; deny += c.denied[a*PriorityCount+p]; }
        for (auto const& b : merged.bots) if (b.second.priority == p) ++unique;
        if (!c.priorities[p] && !allow && !deny && !unique) continue;
        out.str(""); out << "id=" << p << " priority_returns=" << c.priorities[p] << " allowed=" << allow << " denied=" << deny
            << " all_allowed=" << allAllow << " all_denied=" << allDeny << " unique_latest_all=" << unique;
        emit("priority", out.str());
    }
    for (unsigned i = 0; i < c.transitions.size(); ++i) if (c.transitions[i])
        emit("all_transition", "from=" + std::to_string(i/PriorityCount) + " to=" + std::to_string(i%PriorityCount) + " calls=" + std::to_string(c.transitions[i]));
    out.str("");
    for (unsigned i = 0; i < c.activitySources.size(); ++i) out << "source" << i << "=" << c.activitySources[i] << " ";
    emit("activity_sources", out.str());
    auto scaleLine = [&](ScaleSample const& s, bool global) {
        if (!s.observedMs) return;
        out.str(""); out << "map=" << s.map << " instance=" << s.instance << " age_ms=" << Age(now, s.observedMs)
            << " wanted_ms=" << s.wanted << " current_ms=" << s.current << " before_A=" << s.before
            << " after_A=" << s.after << " samples=" << s.samples;
        emit(global ? "global_scale" : "local_scale", out.str());
    };
    scaleLine(merged.global, true);
    for (auto const& item : merged.scales) scaleLine(item.second, false);
    for (auto const& item : merged.maps)
    {
        auto const& m = item.second;
        out.str(""); out << "map=" << m.map << " instance=" << m.instance << " age_ms=" << Age(now, m.observedMs)
            << " list_entries=" << m.entries << " real_priority=" << m.realByPriority << " real_IsBot=" << m.realByIsBot
            << " missing_sessions=" << m.missingSessions
            << " local_A=" << m.localA << " current_ms=" << m.currentMs << " samples=" << m.samples << " dropped_zones=" << m.droppedZones
            << " source_at_snapshot=" << m.source << " wanted_if_local_enabled=" << m.wantedIfLocalEnabled;
        emit("map", out.str());
        for (unsigned i = 0; i < m.zoneCount; ++i)
            emit("player_zone", "map=" + std::to_string(m.map) + " instance=" + std::to_string(m.instance) + " zone=" + std::to_string(m.zones[i]) + " real=" + std::to_string(m.zonePlayers[i]) + " age_ms=" + std::to_string(Age(now, m.observedMs)));
    }
    emit("report_cost", "aggregation_ns=" + std::to_string(Ns()-reportStart));
    return lines;
}
std::vector<std::string> Stop(char const* reason)
{
    if (!Enabled()) return {"[PRESENCE] OFF"};
    if (auto* shard = Current()) Publish(*shard, true);
    activeEpoch.store(0, std::memory_order_release);
    auto lines = Report();
    lines.push_back(std::string("[PRESENCE] stopped reason=") + reason + " final=published_only; inactive-thread tails are not forced");
    return lines;
}
std::vector<std::string> Poll(bool pmo)
{
    if (!Enabled()) return {};
    if (pmo) return Stop("PMO_enabled");
    auto const now = Ms();
    if (now >= Store().deadline) return Stop("deadline");
    Flush();
    if (now < Store().nextReport) return {};
    Store().nextReport = now + 30000;
    return Report();
}
}

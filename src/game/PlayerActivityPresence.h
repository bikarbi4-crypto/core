#ifndef MANGOS_PLAYER_ACTIVITY_PRESENCE_H
#define MANGOS_PLAYER_ACTIVITY_PRESENCE_H

#include <atomic>
#include <cstdint>
#include <map>
#include <mutex>
#include <shared_mutex>

// Activity's definition of a human: an in-world character with a client session.
// This index owns scalar observations only. No Player/Session/Map pointers leave
// their owner through it. Writers publish at lifecycle/movement events, not ticks.
class PlayerActivityPresence
{
public:
    using Guid = std::uint32_t;
    using Token = std::uint64_t;
    struct Position { float x = 0, y = 0, z = 0; };
    struct Location
    {
        std::uint32_t map = 0, instance = 0, zone = 0;
        Position body;
    };
    struct Observation
    {
        Location location;
        Position camera;
        bool remoteCamera = false;
        bool excludedFromNearby = false;
    };
    struct MapPresence { bool map = false, zone = false; };

    static PlayerActivityPresence& Instance();
    Token Enter(Guid guid, Observation const& observation);
    void Leave(Guid guid, Token token);
    void Move(Guid guid, Token token, Location const& location);
    void SetCamera(Guid guid, Token token, Position position, bool remote);
    void SetExcluded(Guid guid, Token token, bool excluded);
    std::uint32_t Count() const { return m_count.load(std::memory_order_acquire); }
    bool HasPlayers() const { return Count() != 0; }
    bool Contains(Guid guid) const;
    MapPresence OnMap(std::uint32_t map, std::uint32_t instance, std::uint32_t zone) const;
    bool Nearby(std::uint32_t map, std::uint32_t instance, Position position,
                float range, bool threeDimensionalAndCamera) const;

    // Visit in GUID order, as with the old registry, without copying it. Callback
    // must be read-only and must not re-enter this index (e.g. SocialMgr::HasFriend).
    template<class Predicate> bool Any(Predicate&& predicate) const
    {
        if (!HasPlayers())
            return false;
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        for (auto const& entry : m_players)
            if (predicate(entry.first))
                return true;
        return false;
    }

private:
    struct Entry { Token token; Observation observation; };
    mutable std::shared_mutex m_mutex;
    std::map<Guid, Entry> m_players;
    std::atomic<std::uint32_t> m_count{0};
    Token m_nextToken = 0;
};

#define sPlayerActivityPresence PlayerActivityPresence::Instance()
#endif

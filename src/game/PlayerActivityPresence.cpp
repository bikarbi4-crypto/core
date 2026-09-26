#include "PlayerActivityPresence.h"

PlayerActivityPresence& PlayerActivityPresence::Instance()
{
    static PlayerActivityPresence instance;
    return instance;
}

PlayerActivityPresence::Token PlayerActivityPresence::Enter(Guid guid, Observation const& observation)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    Token const token = ++m_nextToken;
    m_players.insert_or_assign(guid, Entry{token, observation});
    m_count.store(static_cast<std::uint32_t>(m_players.size()), std::memory_order_release);
    return token;
}

void PlayerActivityPresence::Leave(Guid guid, Token token)
{
    if (!token)
        return;
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    auto it = m_players.find(guid);
    if (it != m_players.end() && it->second.token == token)
    {
        m_players.erase(it);
        m_count.store(static_cast<std::uint32_t>(m_players.size()), std::memory_order_release);
    }
}

void PlayerActivityPresence::Move(Guid guid, Token token, Location const& location)
{
    if (!token)
        return;
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    auto it = m_players.find(guid);
    if (it != m_players.end() && it->second.token == token)
        it->second.observation.location = location;
}

void PlayerActivityPresence::SetCamera(Guid guid, Token token, Position position, bool remote)
{
    if (!token)
        return;
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    auto it = m_players.find(guid);
    if (it != m_players.end() && it->second.token == token)
    {
        it->second.observation.camera = position;
        it->second.observation.remoteCamera = remote;
    }
}

void PlayerActivityPresence::SetExcluded(Guid guid, Token token, bool excluded)
{
    if (!token)
        return;
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    auto it = m_players.find(guid);
    if (it != m_players.end() && it->second.token == token)
        it->second.observation.excludedFromNearby = excluded;
}

bool PlayerActivityPresence::Contains(Guid guid) const
{
    if (!HasPlayers())
        return false;
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_players.find(guid) != m_players.end();
}

PlayerActivityPresence::MapPresence PlayerActivityPresence::OnMap(
    std::uint32_t map, std::uint32_t instance, std::uint32_t zone) const
{
    MapPresence result;
    if (!HasPlayers())
        return result;
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    for (auto const& entry : m_players)
    {
        auto const& location = entry.second.observation.location;
        if (location.map != map || location.instance != instance)
            continue;
        result.map = true;
        if (location.zone == zone)
        {
            result.zone = true;
            break;
        }
    }
    return result;
}

bool PlayerActivityPresence::Nearby(std::uint32_t map, std::uint32_t instance,
    Position position, float range, bool threeDimensionalAndCamera) const
{
    if (!HasPlayers())
        return false;
    float const sqRange = range * range;
    auto inRange = [&](Position other)
    {
        float const dx = position.x - other.x;
        float const dy = position.y - other.y;
        if (!threeDimensionalAndCamera)
            return dx * dx + dy * dy < sqRange;
        float const dz = position.z - other.z;
        return dx * dx + dy * dy + dz * dz < sqRange;
    };
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    for (auto const& entry : m_players)
    {
        auto const& observation = entry.second.observation;
        if (observation.excludedFromNearby || observation.location.map != map ||
            observation.location.instance != instance)
            continue;
        if (inRange(observation.location.body) || (threeDimensionalAndCamera &&
            observation.remoteCamera && inRange(observation.camera)))
            return true;
    }
    return false;
}


#include "playerbot/playerbot.h"
#include "WaitForAttackAction.h"
#include "playerbot/strategy/generic/CombatStrategy.h"
#include "playerbot/strategy/generic/KiteStrategy.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

using namespace ai;

namespace
{
    uint8 GetKiteStackOpenDirections(const WorldPosition& point)
    {
        const float probeDistance = std::max(18.0f, KiteStrategy::GetMinDistance() + 5.0f);
        const float angleIncrement = M_PI_F / 4.0f;
        uint8 openDirections = 0;

        for (uint8 direction = 0; direction < 8; ++direction)
        {
            const float angle = direction * angleIncrement;
            WorldPosition probe(point.getMapId(), point.getX() + probeDistance * cos(angle),
                                point.getY() + probeDistance * sin(angle), point.getZ());

            if (probe.isValid() && point.IsInLineOfSight(probe))
                ++openDirections;
        }

        return openDirections;
    }
} // namespace

bool WaitForAttackKeepSafeDistanceAction::Execute(Event& event)
{
    Unit* target = AI_VALUE(Unit*, "current target");

    if (target && !target->IsStopped() && ObjectAccessor::GetUnit(*target, target->GetTargetGuid()) && ObjectAccessor::GetUnit(*target, target->GetTargetGuid())->IsStopped())
        target = ObjectAccessor::GetUnit(*target, target->GetTargetGuid());


    if (target && target->IsAlive())
    {
        const float safeDistance = std::max(float(target->GetMeleeReach() + ATTACK_DISTANCE), WaitForAttackStrategy::GetSafeDistance());
        const float safeDistanceThreshold = WaitForAttackStrategy::GetSafeDistanceThreshold();

        // Find the best point around the target.
        const WorldPosition bestPoint = GetBestPoint(target, (safeDistance - safeDistanceThreshold), safeDistance);
        if (bestPoint)
        {
            // Move to the best point
            return MoveTo(bestPoint.getMapId(), bestPoint.getX(), bestPoint.getY(), bestPoint.getZ(), false, false, false, true);
        }
    }

    return false;
}

const ai::WorldPosition WaitForAttackKeepSafeDistanceAction::GetBestPoint(Unit* target, float minDistance, float maxDistance) const
{
    const Map* map = target->GetMap();
    const WorldPosition botPosition(bot);
    const WorldPosition targetPosition(target);
    const int8 startDir = urand(0, 1) * 2 - 1;
    const float radiansIncrement = (5.0f / 180.0f) * (M_PI_F);
    const float startAngle = targetPosition.getAngleTo(botPosition) + urand(0.f,radiansIncrement) * startDir;
    const float distance = frand(minDistance, maxDistance);
    const std::list<ObjectGuid> enemies = AI_VALUE(std::list<ObjectGuid>, "possible targets no los");

    if (ai->HasStrategy("debug move", BotState::BOT_STATE_COMBAT))
    {
        for (uint32 dist = 0; dist < distance; dist++)
        {
            WorldPosition point = targetPosition + WorldPosition(0, dist * cos(startAngle), dist * sin(startAngle), 1.0f);
            Creature* wpCreature = bot->SummonCreature(1, point.getX(), point.getY(), point.getZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN, 1000.0f + dist * 100.0f);
        }
    }

    for (float tryAngle = 0.0f; tryAngle < M_PI_F; tryAngle += radiansIncrement)
    {
        for (int8 tryDir = -1; tryAngle && tryDir < 1; tryDir += 2)
        {
            float pointAngle = startAngle;
            pointAngle += tryAngle * startDir * tryDir;

            WorldPosition point = targetPosition + WorldPosition(0, distance * cos(pointAngle), distance * sin(pointAngle), 1.0f);

            point.setZ(point.getHeight());

            if (ai->HasStrategy("debug move", BotState::BOT_STATE_COMBAT))
            {
                Creature* wpCreature = bot->SummonCreature(1, point.getX(), point.getY(), point.getZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN, 5000.0f + tryAngle * 1000.0f);
            }

            // Check if the target is visible from the point
            if (!target->IsWithinLOS(point.getX(), point.getY(), point.getZ() + bot->GetCollisionHeight()))
                continue;

            // Check if the point is not surrounded by other enemies
            if (IsEnemyClose(point, enemies))
                continue;

            // Check if the bot can move to this point.
            if (!botPosition.canPathTo(point,bot))
                continue;

            if (ai->HasStrategy("debug move", BotState::BOT_STATE_COMBAT))
            {
                Creature* wpCreature = bot->SummonCreature(15631, point.getX(), point.getY(), point.getZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN, 5000.0f + tryAngle * 1000.0f);
            }

            return point;
        }
    }



    return botPosition;
}

bool WaitForAttackKeepSafeDistanceAction::IsEnemyClose(const WorldPosition& point, const std::list<ObjectGuid>& enemies) const
{
    for (const ObjectGuid& enemyGUID : enemies)
    {
        Unit* enemy = ai->GetUnit(enemyGUID);
        if (enemy)
        {
            // If the enemy is visible in the same map
            if (enemy->IsWithinLOSInMap(bot))
            {
                // If the enemy is not neutral
                if (enemy->IsHostileTo(bot))
                {
                    const float enemyAttackRange = enemy->GetMeleeReach() + ATTACK_DISTANCE;
                    const float distanceToPoint = WorldPosition(enemy).sqDistance(point);
                    if (distanceToPoint <= (enemyAttackRange * enemyAttackRange))
                    {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

bool KitePositionAction::isUseful()
{
    //
    // Deliberately do NOT call MovementAction::isUseful().
    //
    // Normal movement actions are blocked by "stay".
    // Kite positioning must override that because this is combat survival
    // positioning, not ordinary movement behavior.
    //
    return ai->IsStateActive(BotState::BOT_STATE_COMBAT) && ai->HasStrategy("kite", BotState::BOT_STATE_COMBAT);
}

bool KitePositionAction::IsValidHostile(Unit* unit) const { return unit && unit->IsInWorld() && unit->IsAlive() && unit->GetMapId() == bot->GetMapId() && sServerFacade.IsHostileTo(bot, unit); }

Unit* KitePositionAction::GetClosestHostile(const std::list<Unit*>& hostiles) const
{
    Unit* closest = nullptr;
    float closestDistance = 999999.0f;

    for (Unit* hostile : hostiles)
    {
        if (!IsValidHostile(hostile))
            continue;

        const float distance = sServerFacade.GetDistance2d(bot, hostile);

        if (distance < closestDistance)
        {
            closest = hostile;
            closestDistance = distance;
        }
    }

    return closest;
}

bool KitePositionAction::IsSafePoint(const WorldPosition& point, const std::list<Unit*>& hostiles, bool keepCurrentTargetInRange, bool requireCurrentTargetLos, Unit* rangeTarget) const
{
    const float minDistanceSq = KiteStrategy::GetMinDistance() * KiteStrategy::GetMinDistance();

    const float maxDistanceSq = KiteStrategy::GetMaxDistance() * KiteStrategy::GetMaxDistance();

    Unit* currentTarget = rangeTarget ? rangeTarget : AI_VALUE(Unit*, "current target");

    if (IsValidHostile(currentTarget))
    {
        const float distanceSq = WorldPosition(currentTarget).sqDistance2d(point);

        if (distanceSq <= minDistanceSq)
            return false;

        if (keepCurrentTargetInRange && distanceSq > maxDistanceSq)
        {
            return false;
        }

        if (requireCurrentTargetLos && !currentTarget->IsWithinLOS(point.getX(), point.getY(), point.getZ() + bot->GetCollisionHeight()))
        {
            return false;
        }
    }

    //
    // Never choose a destination that leaves us within 10 yards
    // of ANY combat hostile.
    //
    for (Unit* hostile : hostiles)
    {
        if (!IsValidHostile(hostile))
            continue;

        const float distanceSq = WorldPosition(hostile).sqDistance2d(point);

        if (distanceSq <= minDistanceSq)
            return false;
    }

    return true;
}

const WorldPosition KitePositionAction::GetBestPoint(Unit* anchor, const std::list<Unit*>& hostiles, bool outerOnly,
                                                     const WorldPosition* referencePosition, bool checkPath, Unit* rangeTarget,
                                                     float preferredDistance, bool preferOpenSpace) const
{
    if (!IsValidHostile(anchor))
        return WorldPosition();

    const WorldPosition botPosition(bot);
    const WorldPosition anchorPosition(anchor);
    const WorldPosition& directionOrigin = referencePosition ? *referencePosition : botPosition;

    const float startAngle = anchorPosition.getAngleTo(directionOrigin);

    const float radiansIncrement = ((preferOpenSpace ? 30.0f : 15.0f) / 180.0f) * M_PI_F;
    const uint8 angleSteps = preferOpenSpace ? 6 : 12;

    const float distances[] = {32.0f, 31.0f, 30.0f, 28.0f, 26.0f, 24.0f, 21.0f, 18.0f, 16.0f};
    std::vector<float> preferredDistances;
    const float* distanceCandidates = distances;
    uint8 distanceCount = outerOnly ? 3 : 9;

    // Keep regular "kite" on its original candidate radii. "Kite stack" searches
    // around its configured preferred distance, within its configured safe range.
    if (preferredDistance > 0.0f)
    {
        const float minimumDistance = KiteStrategy::GetMinDistance() + 0.5f;
        const float maximumDistance = KiteStrategy::GetMaxDistance();

        if (minimumDistance <= maximumDistance)
        {
            const float desiredDistance = std::max(minimumDistance, std::min(preferredDistance, maximumDistance));
            preferredDistances.push_back(desiredDistance);

            const float maxOffset = std::max(maximumDistance - desiredDistance, desiredDistance - minimumDistance);
            for (float offset = 2.0f; offset <= maxOffset; offset += 2.0f)
            {
                const float outerDistance = desiredDistance + offset;
                if (outerDistance <= maximumDistance)
                    preferredDistances.push_back(outerDistance);

                const float innerDistance = desiredDistance - offset;
                if (innerDistance >= minimumDistance)
                    preferredDistances.push_back(innerDistance);
            }

            if (std::find(preferredDistances.begin(), preferredDistances.end(), maximumDistance) == preferredDistances.end())
                preferredDistances.push_back(maximumDistance);

            if (std::find(preferredDistances.begin(), preferredDistances.end(), minimumDistance) == preferredDistances.end())
                preferredDistances.push_back(minimumDistance);

            distanceCandidates = preferredDistances.data();
            distanceCount = static_cast<uint8>(preferredDistances.size());
        }
    }

    for (uint8 pass = 0; pass < 3; ++pass)
    {
        const bool keepCurrentTargetInRange = pass < 2;

        const bool requireCurrentTargetLos = pass == 0;

        WorldPosition bestPoint;
        float bestScore = -std::numeric_limits<float>::max();

        for (uint8 d = 0; d < distanceCount; ++d)
        {
            const float distance = distanceCandidates[d];

            // Normal stack positioning must stay inside the configured
            // preferred/settle band. Do not fall back to the maximum range
            // when a candidate in that combat band is temporarily blocked;
            // repeatedly chasing the outer edge interrupts ranged casts.
            if (preferOpenSpace && outerOnly)
            {
                const float bandMin = std::min(preferredDistance, KiteStrategy::GetSettleDistance());
                const float bandMax = std::min(std::max(preferredDistance, KiteStrategy::GetSettleDistance()),
                                               KiteStrategy::GetMaxDistance());

                if (distance < bandMin || distance > bandMax)
                    continue;
            }

            for (uint8 step = 0; step <= angleSteps; ++step)
            {
                const float offset = step * radiansIncrement;

                for (int8 dir = -1; dir <= 1; dir += 2)
                {
                    if (step == 0 && dir == 1)
                        continue;

                    const float pointAngle = startAngle + offset * dir;

                    WorldPosition point = anchorPosition + WorldPosition(0, distance * cos(pointAngle), distance * sin(pointAngle), 1.0f);

                    point.setZ(point.getHeight());

                    if (!anchor->IsWithinLOS(point.getX(), point.getY(), point.getZ() + bot->GetCollisionHeight()))
                    {
                        continue;
                    }

                    if (!IsSafePoint(point, hostiles, keepCurrentTargetInRange, requireCurrentTargetLos, rangeTarget))
                    {
                        continue;
                    }

                    if (checkPath && !IsSafePath(point, hostiles))
                        continue;

                    if (!preferOpenSpace)
                        return point;

                    const float referenceDistance = std::sqrt(directionOrigin.sqDistance2d(point));
                    const float desiredDistance = preferredDistance > 0.0f ? preferredDistance : distance;
                    const float rangePenalty = std::fabs(distance - desiredDistance);
                    const float movementPenalty = referenceDistance;
                    const float score = GetKiteStackOpenDirections(point) * 100.0f - rangePenalty * 3.0f - movementPenalty * 0.25f;

                    if (score > bestScore)
                    {
                        bestScore = score;
                        bestPoint = point;
                    }
                }
            }
        }

        if (bestPoint)
            return bestPoint;
    }

    return WorldPosition();
}

bool KitePositionAction::Execute(Event& event)
{
    Unit* currentTarget = AI_VALUE(Unit*, "current target");
    if (!IsValidHostile(currentTarget) || !currentTarget->IsWithinLOSInMap(bot))
    {
        // A reach action may already have started moving toward this target.
        bot->StopMoving();
        return false;
    }

    std::list<Unit*> hostiles = KiteStrategy::GetNearbyHostiles(ai);
    hostiles.remove_if([this](Unit* hostile) { return !hostile->IsWithinLOSInMap(bot); });

    Unit* closest = GetClosestHostile(hostiles);

    const bool tooClose = IsValidHostile(closest) && sServerFacade.GetDistance2d(bot, closest) <= KiteStrategy::GetMinDistance();

    if (sServerFacade.isMoving(bot) && !tooClose)
        return false;

    const time_t combatStart = ai->GetAiObjectContext()->GetValue<time_t>("combat start time")->Get();

    const time_t settledCombatStart = ai->GetAiObjectContext()->GetValue<time_t>("manual time", "kite settled combat start")->Get();

    const bool initialPositioning = combatStart && settledCombatStart != combatStart;

    Unit* anchor = nullptr;
    bool outerOnly = false;
    if (tooClose)
    {
        anchor = closest;
        outerOnly = false;
    }
    else if (initialPositioning && IsValidHostile(currentTarget))
    {
        anchor = currentTarget;
        outerOnly = true;
    }
    else if (IsValidHostile(currentTarget) && sServerFacade.GetDistance2d(bot, currentTarget) > KiteStrategy::GetMaxDistance())
    {
        anchor = currentTarget;
        outerOnly = true;
    }

    if (!anchor)
        return false;

    const WorldPosition bestPoint = GetBestPoint(anchor, hostiles, outerOnly);

    if (!bestPoint)
        return false;

    return MoveTo(bestPoint.getMapId(), bestPoint.getX(), bestPoint.getY(), bestPoint.getZ(), false, IsReaction(), false, true);
}

bool KiteStackPositionAction::isUseful()
{
    // Like ordinary kiting, stacking is combat movement and must bypass stay's movement block.
    return ai->IsStateActive(BotState::BOT_STATE_COMBAT) && ai->HasStrategy("kite stack", BotState::BOT_STATE_COMBAT);
}

bool KiteStackPositionAction::Execute(Event& event)
{
    Unit* currentTarget = AI_VALUE(Unit*, "current target");
    const bool currentTargetVisible = IsValidHostile(currentTarget) && currentTarget->IsWithinLOSInMap(bot);

    Group* group = bot->GetGroup();
    if (!group)
    {
        if (!currentTargetVisible)
        {
            bot->StopMoving();
            return false;
        }

        return KitePositionAction::Execute(event);
    }

    std::vector<Player*> members;
    uint32 botIndex = 0;
    bool foundBot = false;

    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->getSource();
        if (!member || !member->IsInWorld() || !member->IsAlive() || member->GetMapId() != bot->GetMapId())
            continue;

        PlayerbotAI* memberAI = member->GetPlayerbotAI();
        if (!memberAI || !memberAI->HasStrategy("kite stack", BotState::BOT_STATE_COMBAT))
            continue;

        if (member == bot)
        {
            botIndex = static_cast<uint32>(members.size());
            foundBot = true;
        }

        members.push_back(member);
    }

    // A bot kiting alone keeps the existing kite behavior.
    if (!foundBot)
        return false;

    if (members.size() < 2)
    {
        if (!currentTargetVisible)
        {
            bot->StopMoving();
            return false;
        }

        return KitePositionAction::Execute(event);
    }

    float sumX = 0.0f;
    float sumY = 0.0f;
    float sumZ = 0.0f;

    for (Player* member : members)
    {
        sumX += member->GetPositionX();
        sumY += member->GetPositionY();
        sumZ += member->GetPositionZ();
    }

    const float memberCount = static_cast<float>(members.size());
    const WorldPosition groupCenter(bot->GetMapId(), sumX / memberCount, sumY / memberCount, sumZ / memberCount);

    // Scan from the enrolled bot nearest the cohort center and add every enrolled bot's current target.
    // This keeps the candidate point consistent across the selected cohort.
    Player* representative = members.front();
    float representativeDistanceSq = std::numeric_limits<float>::max();

    for (Player* member : members)
    {
        const float dx = member->GetPositionX() - groupCenter.getX();
        const float dy = member->GetPositionY() - groupCenter.getY();
        const float distanceSq = dx * dx + dy * dy;

        if (distanceSq < representativeDistanceSq)
        {
            representative = member;
            representativeDistanceSq = distanceSq;
        }
    }

    PlayerbotAI* representativeAI = representative->GetPlayerbotAI();
    const float localThreatDistance = KiteStrategy::GetMinDistance() + 5.0f;
    const float localThreatDistanceSq = localThreatDistance * localThreatDistance;
    std::list<Unit*> hostiles = KiteStrategy::GetNearbyHostiles(representativeAI);
    hostiles.remove_if([this, localThreatDistanceSq](Unit* hostile)
    {
        return !hostile->IsWithinLOSInMap(bot) &&
               WorldPosition(hostile).sqDistance2d(WorldPosition(bot)) > localThreatDistanceSq;
    });

    // The representative scan covers close threats to a compact stack. Also scan
    // members outside that coverage (or across a wall) so a local slime cannot be missed.
    const WorldPosition representativePosition(representative);
    const float localScanDistance = std::max(0.0f, KiteStrategy::GetScanDistance() - KiteStrategy::GetMinDistance());
    const float localScanDistanceSq = localScanDistance * localScanDistance;

    for (Player* member : members)
    {
        PlayerbotAI* memberAI = member->GetPlayerbotAI();

        if (member != representative &&
            (representativePosition.sqDistance2d(WorldPosition(member)) > localScanDistanceSq ||
             !representative->IsWithinLOSInMap(member, true)))
        {
            const std::list<Unit*> memberHostiles = KiteStrategy::GetNearbyHostiles(memberAI);
            for (Unit* hostile : memberHostiles)
            {
                if (!IsValidHostile(hostile) ||
                    WorldPosition(hostile).sqDistance2d(WorldPosition(member)) > localThreatDistanceSq)
                    continue;

                if (std::find(hostiles.begin(), hostiles.end(), hostile) == hostiles.end())
                    hostiles.push_back(hostile);
            }
        }

        Unit* target = memberAI->GetAiObjectContext()->GetValue<Unit*>("current target")->Get();

        if (IsValidHostile(target) && target->IsWithinLOSInMap(bot) &&
            std::find(hostiles.begin(), hostiles.end(), target) == hostiles.end())
            hostiles.push_back(target);
    }

    Unit* anchor = nullptr;
    float closestDistanceSq = std::numeric_limits<float>::max();

    // Keep the stack anchored to the representative's visible target when possible.
    // Only switch to another hostile when it is substantially closer, preventing
    // the formation from orbiting between individual mobs in a large pack.
    Unit* representativeTarget = representativeAI->GetAiObjectContext()->GetValue<Unit*>("current target")->Get();
    if (IsValidHostile(representativeTarget) && representativeTarget->IsWithinLOSInMap(bot) &&
        std::find(hostiles.begin(), hostiles.end(), representativeTarget) != hostiles.end())
    {
        anchor = representativeTarget;
        const float dx = groupCenter.getX() - representativeTarget->GetPositionX();
        const float dy = groupCenter.getY() - representativeTarget->GetPositionY();
        closestDistanceSq = dx * dx + dy * dy;
    }

    const float anchorSwitchMarginSq = 8.0f * 8.0f;

    for (Unit* hostile : hostiles)
    {
        if (!IsValidHostile(hostile))
            continue;

        const float dx = groupCenter.getX() - hostile->GetPositionX();
        const float dy = groupCenter.getY() - hostile->GetPositionY();
        const float distanceSq = dx * dx + dy * dy;

        if (!anchor || distanceSq + anchorSwitchMarginSq < closestDistanceSq)
        {
            anchor = hostile;
            closestDistanceSq = distanceSq;
        }
    }

    if (!anchor)
        return false;

    const float minDistance = KiteStrategy::GetMinDistance();
    const float minDistanceSq = minDistance * minDistance;

    // Keep the whole assigned cohort in a compact disk around a shared destination.
    const float stackRadius = std::min(3.0f, std::max(1.0f, std::sqrt(memberCount) * 0.5f));

    // Range decisions are per bot. A member that is already inside its own
    // combat band must not be dragged toward a group destination merely
    // because another member is farther away from the pack.
    const float botMaxDistance = KiteStrategy::GetMaxDistance();
    const float botMaxDistanceSq = botMaxDistance * botMaxDistance;
    Unit* botClosestHostile = nullptr;
    float botClosestDistanceSq = std::numeric_limits<float>::max();

    for (Unit* hostile : hostiles)
    {
        if (!IsValidHostile(hostile))
            continue;

        const float dx = bot->GetPositionX() - hostile->GetPositionX();
        const float dy = bot->GetPositionY() - hostile->GetPositionY();
        const float distanceSq = dx * dx + dy * dy;

        // Do not let a hostile behind a wall pull this bot toward the shared
        // point. Keep only visible threats, except for very close emergency
        // threats that still need an escape response.
        if (!hostile->IsWithinLOSInMap(bot) && distanceSq > localThreatDistanceSq)
            continue;

        if (distanceSq < botClosestDistanceSq)
        {
            botClosestHostile = hostile;
            botClosestDistanceSq = distanceSq;
        }
    }

    const bool botHasHostile = IsValidHostile(botClosestHostile);
    // Per-bot state machine: <= minimum escapes, > minimum through maximum
    // holds for combat, and only > maximum requests the shared stack point.
    const bool botTooClose = botHasHostile && botClosestDistanceSq <= minDistanceSq;
    const bool botBeyondMaxRange = botHasHostile && botClosestDistanceSq > botMaxDistanceSq;

    // Do not replace an existing movement path every AI tick. Let normal combat
    // actions run while the bot is moving toward the shared kite point. Re-enter
    // emergency movement only while this bot is inside its own minimum range.
    if (sServerFacade.isMoving(bot) && !botTooClose)
        return false;

    const time_t combatStart = ai->GetAiObjectContext()->GetValue<time_t>("combat start time")->Get();
    const time_t settledCombatStart = ai->GetAiObjectContext()->GetValue<time_t>("manual time", "kite settled combat start")->Get();
    bool initialPositioning = combatStart && settledCombatStart != combatStart;

    // Treat the preferred and settle distances as the acceptable initial-positioning band.
    // This lets the stack keep a wide firing range without changing ordinary "kite".
    if (initialPositioning && !botTooClose)
    {
        const float anchorDistance = std::sqrt(closestDistanceSq);
        const float preferredDistance = KiteStrategy::GetPreferredDistance();
        const float settleDistance = KiteStrategy::GetSettleDistance();
        const float bandMin = std::min(preferredDistance, settleDistance);
        const float bandMax = std::min(std::max(preferredDistance, settleDistance), KiteStrategy::GetMaxDistance());

        if (anchorDistance >= bandMin && anchorDistance <= bandMax)
        {
            ai->GetAiObjectContext()->GetValue<time_t>("manual time", "kite settled combat start")->Set(combatStart);
            initialPositioning = false;
        }
    }

    const bool botInitialPositioning = initialPositioning && botBeyondMaxRange;

    if (!botTooClose && !botInitialPositioning && !botBeyondMaxRange)
        return false;

    const bool outerOnly = !botTooClose;
    const auto findEmergencyEscapeCenter = [&]() -> WorldPosition
    {
        float awayX = 0.0f;
        float awayY = 0.0f;

        for (Unit* hostile : hostiles)
        {
            if (!IsValidHostile(hostile))
                continue;

            const float dx = groupCenter.getX() - hostile->GetPositionX();
            const float dy = groupCenter.getY() - hostile->GetPositionY();
            const float distanceSq = dx * dx + dy * dy;
            const float weight = 1.0f / std::max(1.0f, distanceSq);
            awayX += dx * weight;
            awayY += dy * weight;
        }

        if (awayX * awayX + awayY * awayY < 0.0001f)
        {
            awayX = groupCenter.getX() - anchor->GetPositionX();
            awayY = groupCenter.getY() - anchor->GetPositionY();
        }

        if (awayX * awayX + awayY * awayY < 0.0001f)
        {
            awayX = cos(representative->GetOrientation());
            awayY = sin(representative->GetOrientation());
        }

        const float baseAngle = atan2(awayY, awayX);
        const float nearestDistance = std::sqrt(closestDistanceSq);
        const float recoveryDistance = std::max(minDistance + 1.0f,
                                                std::min(KiteStrategy::GetSettleDistance(), KiteStrategy::GetMaxDistance()));
        const float desiredMoveDistance = std::max(8.0f, recoveryDistance - nearestDistance + 1.0f);
        const float moveDistances[] = {
            desiredMoveDistance,
            std::min(desiredMoveDistance, 24.0f),
            std::min(desiredMoveDistance, 16.0f),
            8.0f,
            5.0f
        };
        const float angleIncrement = (45.0f / 180.0f) * M_PI_F;

        WorldPosition bestPoint;
        float bestScore = -std::numeric_limits<float>::max();

        // If every normal kite point is blocked by the pack, take the longest
        // reachable escape burst that moves the cohort beyond the early escape
        // band. Shorter distances remain fallback candidates for tight rooms.
        for (uint8 distanceIndex = 0; distanceIndex < 5; ++distanceIndex)
        {
            const float moveDistance = moveDistances[distanceIndex];
            if (moveDistance < 0.1f)
                continue;

            for (uint8 direction = 0; direction < 8; ++direction)
            {
                const float angle = baseAngle + direction * angleIncrement;
                WorldPosition point = groupCenter + WorldPosition(0, moveDistance * cos(angle), moveDistance * sin(angle), 0.0f);
                point.setZ(point.getHeight());

                if (!point.isValid())
                    continue;

                const std::vector<WorldPosition> representativePath = representativePosition.getPathTo(point, representative);
                if (representativePath.empty() || !point.isPathTo(representativePath))
                    continue;

                float nearestPointDistanceSq = std::numeric_limits<float>::max();
                for (Unit* hostile : hostiles)
                {
                    if (IsValidHostile(hostile))
                        nearestPointDistanceSq = std::min(nearestPointDistanceSq, WorldPosition(hostile).sqDistance2d(point));
                }

                if (nearestPointDistanceSq == std::numeric_limits<float>::max())
                    continue;

                const float score = std::sqrt(nearestPointDistanceSq) * 100.0f +
                                    GetKiteStackOpenDirections(point) * 10.0f + cos(angle - baseAngle) * 2.0f;

                if (score > bestScore)
                {
                    bestScore = score;
                    bestPoint = point;
                }
            }
        }

        return bestPoint;
    };

    const auto moveToEmergencyEscape = [&]() -> bool
    {
        const auto moveToPersonalEscape = [&]() -> bool
        {
            Unit* closestHostile = GetClosestHostile(hostiles);
            if (!IsValidHostile(closestHostile))
                return false;

            const WorldPosition botPosition(bot);
            float awayX = bot->GetPositionX() - closestHostile->GetPositionX();
            float awayY = bot->GetPositionY() - closestHostile->GetPositionY();

            if (awayX * awayX + awayY * awayY < 0.0001f)
            {
                awayX = groupCenter.getX() - closestHostile->GetPositionX();
                awayY = groupCenter.getY() - closestHostile->GetPositionY();
            }

            if (awayX * awayX + awayY * awayY < 0.0001f)
            {
                awayX = cos(bot->GetOrientation());
                awayY = sin(bot->GetOrientation());
            }

            const float baseAngle = atan2(awayY, awayX);
            const float currentDistance = sServerFacade.GetDistance2d(bot, closestHostile);
            const float recoveryDistance = std::max(minDistance + 1.0f,
                                                    std::min(KiteStrategy::GetSettleDistance(), KiteStrategy::GetMaxDistance()));
            const float desiredStepDistance = std::max(8.0f, recoveryDistance - currentDistance + 1.0f);
            const float stepDistances[] = {
                desiredStepDistance,
                std::min(desiredStepDistance, 24.0f),
                std::min(desiredStepDistance, 16.0f),
                8.0f,
                5.0f
            };
            const float angleIncrement = (30.0f / 180.0f) * M_PI_F;

            for (uint8 distanceIndex = 0; distanceIndex < 5; ++distanceIndex)
            {
                const float stepDistance = stepDistances[distanceIndex];
                if (stepDistance < 0.1f)
                    continue;

                for (uint8 direction = 0; direction < 7; ++direction)
                {
                    const float angle = baseAngle + (static_cast<float>(direction) - 3.0f) * angleIncrement;
                    WorldPosition point = botPosition + WorldPosition(0, stepDistance * cos(angle), stepDistance * sin(angle), 0.0f);
                    point.setZ(point.getHeight());

                    if (!point.isValid() || !IsEmergencyEscapePath(point, hostiles))
                        continue;

                    return MoveTo(point.getMapId(), point.getX(), point.getY(), point.getZ(), false, IsReaction(), false, true);
                }
            }

            return false;
        };

        // The minimum-range breach belongs to this bot only. Do not use the
        // cohort escape center for an individual emergency.
        if (botTooClose)
            return moveToPersonalEscape();

        const WorldPosition escapeCenter = findEmergencyEscapeCenter();
        if (!escapeCenter)
            return moveToPersonalEscape();

        const float escapeRadius = std::min(2.0f, std::max(0.75f, std::sqrt(memberCount) * 0.25f));
        const float escapeAngle = atan2(escapeCenter.getY() - anchor->GetPositionY(),
                                        escapeCenter.getX() - anchor->GetPositionX()) +
                                  static_cast<float>(botIndex) * 2.39996322972865332f;
        const float memberRadius = escapeRadius * std::sqrt((static_cast<float>(botIndex) + 0.5f) / memberCount);
        WorldPosition escapeDestination = escapeCenter + WorldPosition(0, memberRadius * cos(escapeAngle),
                                                                        memberRadius * sin(escapeAngle), 0.0f);
        escapeDestination.setZ(escapeDestination.getHeight());

        if (!IsEmergencyEscapePath(escapeDestination, hostiles))
        {
            escapeDestination = escapeCenter;
            if (!IsEmergencyEscapePath(escapeDestination, hostiles))
                return moveToPersonalEscape();
        }

        return MoveTo(escapeDestination.getMapId(), escapeDestination.getX(), escapeDestination.getY(),
                      escapeDestination.getZ(), false, IsReaction(), false, true);
    };

    if (botTooClose)
        return moveToEmergencyEscape();

    const float movementTargetDistance = KiteStrategy::GetPreferredDistance();
    // Keep the shared destination, but validate its range against this bot's
    // own nearest hostile rather than only the cohort anchor.
    Unit* botRangeTarget = botClosestHostile ? botClosestHostile : anchor;
    const WorldPosition stackCenter = GetBestPoint(anchor, hostiles, outerOnly, &groupCenter, true, botRangeTarget,
                                                   movementTargetDistance, !botTooClose);

    if (!stackCenter)
    {
        if (!botTooClose)
            return false;

        return moveToEmergencyEscape();
    }

    // Shrink the shared formation if the selected safe point has little room between hostile distance limits.
    float safeStackRadius = stackRadius;

    for (Unit* hostile : hostiles)
    {
        if (!IsValidHostile(hostile))
            continue;

        const float dx = stackCenter.getX() - hostile->GetPositionX();
        const float dy = stackCenter.getY() - hostile->GetPositionY();
        const float distance = std::sqrt(dx * dx + dy * dy);
        const float safeInnerClearance = distance - minDistance - 0.25f;
        safeStackRadius = std::min(safeStackRadius, safeInnerClearance);

        if (hostile == anchor)
        {
            const float safeOuterClearance = KiteStrategy::GetMaxDistance() - distance - 0.25f;
            safeStackRadius = std::min(safeStackRadius, safeOuterClearance);
        }
    }

    safeStackRadius = std::max(0.0f, safeStackRadius);

    const float angle = atan2(stackCenter.getY() - anchor->GetPositionY(), stackCenter.getX() - anchor->GetPositionX()) +
        static_cast<float>(botIndex) * 2.39996322972865332f;
    const float radius = safeStackRadius * std::sqrt((static_cast<float>(botIndex) + 0.5f) / memberCount);

    WorldPosition destination = stackCenter + WorldPosition(0, radius * cos(angle), radius * sin(angle), 0.0f);
    destination.setZ(destination.getHeight());

    if (!IsSafePoint(destination, hostiles, false, false, botRangeTarget))
    {
        if (!botTooClose || !IsSafePoint(stackCenter, hostiles, false, false, botRangeTarget))
            return false;

        // A tight hazard gap can leave no safe room for individual offsets.
        // Collapse this bot onto the shared safe center rather than stopping.
        destination = stackCenter;
    }

    if (!IsSafePath(destination, hostiles))
    {
        if (!botTooClose)
            return false;

        if (!IsEmergencyEscapePath(destination, hostiles))
        {
            destination = stackCenter;
            if (!IsSafePath(destination, hostiles) && !IsEmergencyEscapePath(destination, hostiles))
                return moveToEmergencyEscape();
        }
    }

    return MoveTo(destination.getMapId(), destination.getX(), destination.getY(), destination.getZ(), false, IsReaction(), false, true);
}

bool KitePositionAction::IsEmergencyEscapePath(const WorldPosition& point, const std::list<Unit*>& hostiles) const
{
    const WorldPosition start(bot);
    const std::vector<WorldPosition> path = start.getPathTo(point, bot);

    if (path.empty() || !point.isPathTo(path))
        return false;

    // Emergency movement must obey the same hostile-clearance checks as normal kite movement.
    // Checking only the endpoint allows a path to cut through the mob pack.
    if (!IsSafePath(point, hostiles))
        return false;

    Unit* closest = GetClosestHostile(hostiles);
    if (!IsValidHostile(closest))
        return false;

    const WorldPosition hostilePosition(closest);
    const float startDistanceSq = hostilePosition.sqDistance2d(start);
    const float endDistanceSq = hostilePosition.sqDistance2d(point);

    // During an emergency, allow the shortest available route as long as it is
    // navigable and takes this bot farther from its nearest hostile.
    return endDistanceSq > startDistanceSq + 0.25f;
}

bool KitePositionAction::IsSafePath(const WorldPosition& point, const std::list<Unit*>& hostiles) const
{
    const WorldPosition start(bot);

    std::vector<WorldPosition> path = start.getPathTo(point, bot);

    if (path.empty() || !point.isPathTo(path))
        return false;

    const float safeDistance = KiteStrategy::GetMinDistance();

    const float safeDistanceSq = safeDistance * safeDistance;

    for (Unit* hostile : hostiles)
    {
        if (!IsValidHostile(hostile))
            continue;

        const WorldPosition hostilePos(hostile);

        WorldPosition previous = start;

        float previousDistanceSq = hostilePos.sqDistance2d(previous);

        bool escaping = previousDistanceSq <= safeDistanceSq;

        for (const WorldPosition& next : path)
        {
            const float dx = next.getX() - previous.getX();

            const float dy = next.getY() - previous.getY();

            const float lengthSq = dx * dx + dy * dy;

            float t = 0.0f;

            if (lengthSq > 0.001f)
            {
                t = ((hostilePos.getX() - previous.getX()) * dx + (hostilePos.getY() - previous.getY()) * dy) / lengthSq;

                if (t < 0.0f)
                    t = 0.0f;
                else if (t > 1.0f)
                    t = 1.0f;
            }

            const float closestX = previous.getX() + t * dx;

            const float closestY = previous.getY() + t * dy;

            const float enemyDx = hostilePos.getX() - closestX;

            const float enemyDy = hostilePos.getY() - closestY;

            const float segmentDistanceSq = enemyDx * enemyDx + enemyDy * enemyDy;

            const float nextDistanceSq = hostilePos.sqDistance2d(next);

            if (escaping)
            {
                if (segmentDistanceSq + 0.01f < previousDistanceSq)
                {
                    return false;
                }

                if (nextDistanceSq > safeDistanceSq)
                    escaping = false;
            }
            else
            {
                if (segmentDistanceSq <= safeDistanceSq)
                    return false;
            }

            previousDistanceSq = nextDistanceSq;
            previous = next;
        }
    }

    return true;
}

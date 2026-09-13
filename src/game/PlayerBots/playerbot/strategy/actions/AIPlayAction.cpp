#include "playerbot/playerbot.h"
#include "AIPlayAction.h"

#include "playerbot/PlayerbotAI.h"
#include "playerbot/PlayerbotAIConfig.h"
#include "playerbot/PlayerbotLLMInterface.h"
#include "playerbot/PlayerbotTextMgr.h"
#include "playerbot/ServerFacade.h"
#include "playerbot/TravelMgr.h"
#include "playerbot/WorldPosition.h"
#include "playerbot/strategy/actions/ChooseTravelTargetAction.h"
#include "playerbot/strategy/AiObjectContext.h"
#include "playerbot/strategy/NamedObjectContext.h"
#include "playerbot/strategy/actions/SayAction.h"
#include "playerbot/strategy/values/NearestGameObjects.h"
#include "World.h"
#include "ObjectAccessor.h"
#include "Group.h"
#include "Log.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <exception>
#include <future>
#include <map>
#include <random>
#include <set>
#include <thread>
#include <vector>

using namespace ai;

namespace
{
    bool IsValidAIPlayAttackTarget(Player* bot, Unit* target)
    {
        return bot && target && target->IsInWorld() && sServerFacade.IsAlive(target) &&
            bot->IsValidAttackTarget(target) && bot->IsWithinLOSInMap(target);
    }

    Unit* GetAIPlayAttackTarget(PlayerbotAI* ai, Player* bot, ObjectGuid guid)
    {
        if (!ai || !bot || guid.IsEmpty())
            return nullptr;

        Unit* target = ai->GetUnit(guid);
        return IsValidAIPlayAttackTarget(bot, target) ? target : nullptr;
    }
}

bool AIPlayAttackAction::Execute(Event& event)
{
    (void)event;
    Player* bot = ai ? ai->GetBot() : nullptr;
    AiObjectContext* context = ai ? ai->GetAiObjectContext() : nullptr;
    if (!bot || !context)
        return false;

    // Prefer the master's selection, then the bot's selected/current target.
    Unit* target = nullptr;
    Player* master = ai->GetMaster();
    if (master)
        target = GetAIPlayAttackTarget(ai, bot, master->GetSelectionGuid());
    if (!target)
        target = GetAIPlayAttackTarget(ai, bot, bot->GetSelectionGuid());
    if (!target)
    {
        Value<Unit*>* currentTarget = context->GetValue<Unit*>("current target");
        if (currentTarget && IsValidAIPlayAttackTarget(bot, currentTarget->Get()))
            target = currentTarget->Get();
    }

    // With no selected target, attack the nearest valid hostile creature in sight.
    if (!target)
    {
        Value<std::list<ObjectGuid>>* possibleTargets = context->GetValue<std::list<ObjectGuid>>("possible targets");
        float closestDistance = 1000000000.0f;
        if (possibleTargets)
        {
            for (ObjectGuid guid : possibleTargets->Get())
            {
                Unit* candidate = GetAIPlayAttackTarget(ai, bot, guid);
                if (!candidate || !candidate->IsCreature() || !sServerFacade.IsHostileTo(candidate, bot))
                    continue;

                const float distance = sServerFacade.GetDistance2d(bot, candidate);
                if (distance < closestDistance)
                {
                    closestDistance = distance;
                    target = candidate;
                }
            }
        }
    }

    if (!target)
        return false;

    const ObjectGuid targetGuid = target->GetGUID();
    if (Value<GuidVector>* prioritizedTargets = context->GetValue<GuidVector>("prioritized targets"))
        prioritizedTargets->Set({ targetGuid });

    // It is already doing the requested thing.
    if (bot->GetVictim() == target)
        return true;

    const bool attacked = Attack(bot, target);
    if (attacked)
    {
        if (Value<ObjectGuid>* pullTarget = context->GetValue<ObjectGuid>("pull target"))
            pullTarget->Set(targetGuid);
    }
    return attacked;
}

bool AIPlayMoveToRequesterAction::Execute(Event& event)
{
    // Move once toward the player who asked; this does not install the persistent
    // follow movement generator or alter the bot's follow/stay strategies.
    Player* requester = event.getOwner() ? event.getOwner() : GetMaster();
    if (!requester || requester == bot || !requester->IsInWorld() ||
        requester->GetMapId() != bot->GetMapId() || requester->GetInstanceId() != bot->GetInstanceId())
    {
        return false;
    }

    const float followDistance = ai->GetRange("follow");
    if (sServerFacade.GetDistance2d(bot, requester) <= followDistance)
        return true;

    return MoveNear(requester, followDistance);
}

bool AIPlayMoveRandomAction::Execute(Event&)
{
    const uint32 randnum = urand(1, 2000);
    const float pi = 3.14159265358979323846f;
    const float angle = pi * (float)randnum / 1000.0f;
    const float distance = (float)urand(20, 200);

    return MoveTo(bot->GetMapId(), bot->GetPositionX() + std::cos(angle) * distance,
        bot->GetPositionY() + std::sin(angle) * distance, bot->GetPositionZ());
}

bool AIPlayStopAttackAction::Execute(Event&)
{
    if (!bot)
        return false;

    bot->AttackStop();
    return true;
}

namespace
{
    std::string LowerAIPlayText(std::string text)
    {
        std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return (char)std::tolower(c); });
        return text;
    }

    struct AIPlayCommand
    {
        const char* id;
        const char* action;
    };

    static const AIPlayCommand aiPlayCommands[] =
    {
        { "ATTACK", "attack my target" },
        { "COME", "ai play move to requester" },
        { "STOP", "ai play stop attack" },
        { "TRAVEL", "ai play travel" },
        { "EXPLORE", "ai play move random" },
        { "LOOT", "loot" },
        { "QUEST", "doquest" },
        { "INTERACT", "ai play interact" },
        { "GREET", "greet" },
        { "EMOTE", "emote" },
        { "EAT", "food" },
        { "DRINK", "drink" },
        { "HEAL", "ai play heal" },
        { "MOUNT", "mount" }
    };

    const AIPlayCommand* FindAIPlayCommand(const std::string& id)
    {
        const std::string loweredId = LowerAIPlayText(id);
        for (const AIPlayCommand& command : aiPlayCommands)
            if (loweredId == LowerAIPlayText(command.id))
                return &command;
        return nullptr;
    }

    std::string JoinStrings(const std::vector<std::string>& strings)
    {
        std::string result;
        for (const std::string& value : strings)
        {
            if (value.empty())
                continue;
            if (!result.empty())
                result += " ";
            result += value;
        }
        return result;
    }

    std::string GetNearbySight(PlayerbotAI* ai)
    {
        AiObjectContext* context = ai->GetAiObjectContext();
        Player* bot = ai->GetBot();
        std::vector<std::pair<ObjectGuid, bool>> visible;
        std::set<ObjectGuid> added;

        const char* playerLists[] = { "nearest non bot players", "nearest friendly players" };
        for (const char* valueName : playerLists)
        {
            Value<std::list<ObjectGuid>>* value = context->GetValue<std::list<ObjectGuid>>(valueName);
            if (!value)
                continue;

            for (ObjectGuid guid : value->Get())
            {
                if (added.insert(guid).second)
                    visible.emplace_back(guid, false);
            }
        }

        const char* creatureLists[] = { "nearest npcs", "possible targets" };
        for (const char* valueName : creatureLists)
        {
            Value<std::list<ObjectGuid>>* value = context->GetValue<std::list<ObjectGuid>>(valueName);
            if (!value)
                continue;

            for (ObjectGuid guid : value->Get())
            {
                if (added.insert(guid).second)
                    visible.emplace_back(guid, false);
            }
        }

        Value<std::list<ObjectGuid>>* gameObjects = context->GetValue<std::list<ObjectGuid>>("nearest game objects");
        if (gameObjects)
        {
            for (ObjectGuid guid : gameObjects->Get())
            {
                if (added.insert(guid).second)
                    visible.emplace_back(guid, true);
            }
        }

        if (visible.empty())
            return "No nearby players, creatures, or objects are visible.";

        static thread_local std::mt19937 generator(std::random_device{}());
        std::shuffle(visible.begin(), visible.end(), generator);
        const size_t maxVisible = std::min<size_t>(3, visible.size());
        const size_t selectedCount = urand(1, (uint32)maxVisible);

        std::vector<std::string> descriptions;
        for (size_t i = 0; i < selectedCount; ++i)
        {
            const ObjectGuid& guid = visible[i].first;
            if (visible[i].second)
            {
                GameObject* object = ai->GetGameObject(guid);
                if (object)
                    descriptions.push_back(std::string("object ") + object->GetName());
                continue;
            }

            Unit* unit = ai->GetUnit(guid);
            if (!unit)
                continue;

            std::string kind = unit->IsPlayer() ? "player" : "creature";
            if (unit->IsPlayer() && ai->IsRealPlayer(unit))
                kind = "real player";
            else if (sServerFacade.IsHostileTo(unit, bot))
                kind = "hostile creature";

            descriptions.push_back(kind + " " + unit->GetName() + " level " + std::to_string(unit->GetLevel()));
        }

        return descriptions.empty() ? "No nearby players, creatures, or objects are visible." : JoinStrings(descriptions);
    }

    uint32 NextControlInterval()
    {
        uint32 minimum = sPlayerbotAIConfig.llmControlMinInterval;
        uint32 maximum = sPlayerbotAIConfig.llmControlMaxInterval;
        if (maximum < minimum)
            maximum = minimum;
        return urand(minimum, maximum);
    }

    void CapActionGenerationLength(std::string& json);

    bool BuildActionRequest(PlayerbotAI* ai, const std::string& latestText, std::string& json)
    {
        Player* bot = ai ? ai->GetBot() : nullptr;
        AiObjectContext* context = ai ? ai->GetAiObjectContext() : nullptr;
        if (!bot || !context)
            return false;

        std::map<std::string, std::string> placeholders;
        ChatReplyAction::GetAIChatPlaceholders(placeholders, bot, bot);
        ChatReplyAction::GetAIChatPlaceholders(placeholders, bot, "bot", bot);
        placeholders["<other name>"] = "the world around you";
        placeholders["<other gender>"] = "unknown";
        placeholders["<other level>"] = "unknown";
        placeholders["<other class>"] = "unknown";
        placeholders["<other race>"] = "unknown";
        placeholders["<other type>"] = "your surroundings";
        placeholders["<channel name>"] = "while adventuring";
        placeholders["<initial message>"] = latestText.empty() ? "Choose your next action." : latestText;

        std::string previousContext = ai->GetAIPlayContext();
        const size_t maxRecentContext = 512;
        if (previousContext.size() > maxRecentContext)
            previousContext.erase(0, previousContext.size() - maxRecentContext);

        std::map<std::string, std::string> jsonFill;
        jsonFill["<pre prompt>"] = "Select one action. Do not roleplay.";
        jsonFill["<context>"] = previousContext;
        jsonFill["<prompt>"] = latestText.empty() ? "Choose the next useful action from the current situation." : "Recent chat: " + latestText;
        jsonFill["<prompt>"] += " Nearby: " + GetNearbySight(ai);
        jsonFill["<post prompt>"] = "IDs: " + AIPlayAction::GetCompactActionMenu() +
            " Output only one ID.";

        const uint32 fixedLength = jsonFill["<pre prompt>"].size() + jsonFill["<prompt>"].size() + jsonFill["<post prompt>"].size();
        PlayerbotLLMInterface::LimitContext(jsonFill["<context>"], fixedLength + jsonFill["<context>"].size());

        for (auto& field : jsonFill)
            field.second = PlayerbotLLMInterface::SanitizeForJson(field.second);
        for (auto& placeholder : placeholders)
            placeholder.second = PlayerbotLLMInterface::SanitizeForJson(placeholder.second);

        json = PlayerbotTextMgr::GetReplacePlaceholders(sPlayerbotAIConfig.llmApiJson, jsonFill);
        json = PlayerbotTextMgr::GetReplacePlaceholders(json, placeholders);
        CapActionGenerationLength(json);
        return !json.empty();
    }

    void CapActionGenerationLength(std::string& json)
    {
        const std::string key = "\"max_length\"";
        const size_t keyPosition = json.find(key);
        if (keyPosition == std::string::npos)
            return;

        const size_t colon = json.find(':', keyPosition + key.size());
        if (colon == std::string::npos)
            return;

        const size_t valueStart = json.find_first_not_of(" \t\r\n", colon + 1);
        if (valueStart == std::string::npos || !std::isdigit(static_cast<unsigned char>(json[valueStart])))
            return;

        size_t valueEnd = valueStart;
        uint32 value = 0;
        while (valueEnd < json.size() && std::isdigit(static_cast<unsigned char>(json[valueEnd])))
        {
            if (value < 1000)
                value = value * 10 + (json[valueEnd] - '0');
            ++valueEnd;
        }

        const uint32 maxActionTokens = 16;
        if (value > maxActionTokens)
            json.replace(valueStart, valueEnd - valueStart, std::to_string(maxActionTokens));
    }

    bool ExecuteAIPlayTravel(PlayerbotAI* ai)
    {
        Player* bot = ai ? ai->GetBot() : nullptr;
        if (!bot || bot->InBattleGround())
            return false;

        AiObjectContext* context = ai->GetAiObjectContext();
        if (!context)
            return false;

        TravelTarget* target = AI_VALUE(TravelTarget*, "travel target");
        if (!target)
            return false;

        // Do not interrupt an in-flight destination query. Otherwise expire
        // the current goal so this explicit request can replace it.
        if (target->GetStatus() == TravelStatus::TRAVEL_STATUS_PREPARE)
            return true;

        const TravelStatus previousStatus = target->GetStatus();
        target->SetStatus(TravelStatus::TRAVEL_STATUS_EXPIRED);
        context->ClearValues("travel target active");
        context->ClearValues("no active travel destinations");

        const uint32 purpose = (uint32)TravelDestinationPurpose::Explore;
        WorldPosition center(bot);
        PlayerTravelInfo travelInfo(bot);
        FutureDestinations* destinations = AI_VALUE(FutureDestinations*, "future travel destinations");
        if (!destinations)
        {
            target->SetStatus(previousStatus);
            context->ClearValues("travel target active");
            return false;
        }

        try
        {
            *destinations = std::async(std::launch::async,
                [partitions = travelPartitions, travelInfo, center, purpose]()
                {
                    return sTravelMgr.GetPartitions(center, partitions, travelInfo, purpose);
                });
        }
        catch (const std::exception&)
        {
            target->SetStatus(previousStatus);
            context->ClearValues("travel target active");
            return false;
        }

        SET_AI_VALUE2(std::string, "manual string", "future travel purpose", std::to_string(purpose));
        SET_AI_VALUE2(std::string, "manual string", "future travel condition", std::string());
        SET_AI_VALUE2(int, "manual int", "future travel relevance", 629);
        target->SetStatus(TravelStatus::TRAVEL_STATUS_PREPARE);
        return true;
    }

    bool ExecuteAIPlayInteract(PlayerbotAI* ai, Player* requester)
    {
        Player* bot = ai ? ai->GetBot() : nullptr;
        AiObjectContext* context = ai ? ai->GetAiObjectContext() : nullptr;
        if (!bot || !context)
            return false;

        Value<std::list<ObjectGuid>>* nearbyNpcs = context->GetValue<std::list<ObjectGuid>>("nearest npcs");
        if (nearbyNpcs)
        {
            for (const ObjectGuid& guid : nearbyNpcs->Get())
            {
                Unit* unit = ai->GetUnit(guid);
                if (!unit || !unit->IsInWorld() || unit->GetMapId() != bot->GetMapId() ||
                    sServerFacade.GetDistance2d(bot, unit) > INTERACTION_DISTANCE)
                {
                    continue;
                }

                // GossipHelloAction receives its NPC target in the event packet;
                // it opens the gossip interaction without selecting an option.
                if (ai->DoSpecificAction("gossip hello", Event("ai play", guid, requester), true))
                    return true;
            }
        }

        Value<std::list<ObjectGuid>>* nearbyObjects = context->GetValue<std::list<ObjectGuid>>("nearest game objects no los");
        if (!nearbyObjects)
            return false;

        GameObject* nearestObject = nullptr;
        float closestDistance = 9999.0f;
        for (const ObjectGuid& guid : nearbyObjects->Get())
        {
            GameObject* gameObject = ai->GetGameObject(guid);
            if (!gameObject || !gameObject->IsInWorld() || gameObject->GetMapId() != bot->GetMapId())
                continue;

            const float distance = bot->GetDistance3dToCenter(gameObject);
            if (distance < closestDistance)
            {
                nearestObject = gameObject;
                closestDistance = distance;
            }
        }

        if (!nearestObject || bot->GetDistance(nearestObject) > INTERACTION_DISTANCE)
            return false;

        // "go" makes the native UseAction activate the nearest gameobject,
        // including its built-in chest, door, and quest-object handling.
        return ai->DoSpecificAction("use", Event("ai play", "go", requester), true);
    }

    bool ExecuteAIPlayHeal(PlayerbotAI* ai, Player* requester)
    {
        Player* bot = ai ? ai->GetBot() : nullptr;
        if (!bot)
            return false;

        std::vector<const char*> healingActions;
        switch (bot->GetClass())
        {
        case CLASS_PRIEST:
            healingActions = { "flash heal on party", "greater heal on party", "heal on party", "lesser heal on party",
                "flash heal", "greater heal", "heal", "lesser heal" };
            break;
        case CLASS_DRUID:
            healingActions = { "healing touch on party", "regrowth on party", "rejuvenation on party",
                "healing touch", "regrowth", "rejuvenation" };
            break;
        case CLASS_PALADIN:
            healingActions = { "holy light on party", "flash of light on party", "lay on hands on party",
                "holy light", "flash of light", "lay on hands" };
            break;
        case CLASS_SHAMAN:
            healingActions = { "healing wave on party", "lesser healing wave on party",
                "healing wave", "lesser healing wave" };
            break;
        default:
            break;
        }

        for (const char* action : healingActions)
        {
            if (ai->DoSpecificAction(action, Event("ai play", "", requester), true))
                return true;
        }

        // Classes without healing spells, or healers without a usable spell,
        // can still recover with the existing potion action.
        return ai->DoSpecificAction("healing potion", Event("ai play", "", requester), true);
    }

    bool ExecuteAIPlayCommand(PlayerbotAI* ai, const std::string& commandId, ObjectGuid ownerGuid)
    {
        const AIPlayCommand* command = FindAIPlayCommand(commandId);
        if (!command || !ai || !ai->GetBot() || !ai->GetBot()->IsInWorld() ||
            !sServerFacade.IsAlive(ai->GetBot()) ||
            !ai->HasStrategy("ai play", BotState::BOT_STATE_NON_COMBAT) || !sPlayerbotAIConfig.llmEnabled)
        {
            return false;
        }

        if (sPlayerbotAIConfig.llmRequirePlayerPresence && !ai->HasRealPlayerNearbyOrInGroup())
            return false;

        Player* owner = ownerGuid.IsEmpty() ? nullptr : sObjectAccessor.FindPlayer(ownerGuid);
        if (!owner || !owner->IsInWorld() || !ai->IsRealPlayer(owner))
            owner = ai->GetMaster();

        if (command->id == std::string("TRAVEL"))
            return ExecuteAIPlayTravel(ai);
        if (command->id == std::string("INTERACT"))
            return ExecuteAIPlayInteract(ai, owner);
        if (command->id == std::string("HEAL"))
            return ExecuteAIPlayHeal(ai, owner);
        if (command->id == std::string("ATTACK"))
        {
            AIPlayAttackAction attackAction(ai);
            Event attackEvent("ai play", "", owner);
            if (!attackAction.Execute(attackEvent))
                return false;

            if (ai->HasStrategy("debug llm", BotState::BOT_STATE_NON_COMBAT))
                ai->TellPlayerNoFacing(ai->GetMaster(), "AI play selected action: attack my target");
            return true;
        }

        const std::string action = command->action;
        if (!ai->CanDoSpecificAction(action, true, true))
            return false;

        if (!ai->DoSpecificAction(action, Event("ai play", "", owner), true))
            return false;

        if (ai->HasStrategy("debug llm", BotState::BOT_STATE_NON_COMBAT))
            ai->TellPlayerNoFacing(ai->GetMaster(), "AI play selected action: " + action);

        return true;
    }

}

std::string AIPlayAction::GetCompactActionMenu()
{
    return "ATTACK, COME, STOP, TRAVEL, "
        "EXPLORE, LOOT, QUEST, "
        "INTERACT, GREET, EMOTE, EAT, DRINK, "
        "HEAL, MOUNT.";
}

std::string AIPlayAction::ExtractActionIntent(std::string& text)
{
    size_t position = 0;
    bool foundNone = false;

    while (position < text.size())
    {
        while (position < text.size() && !std::isalnum(static_cast<unsigned char>(text[position])) && text[position] != '_')
            ++position;

        size_t idStart = position;
        while (position < text.size())
        {
            const unsigned char c = static_cast<unsigned char>(text[position]);
            if (!std::isalnum(c) && text[position] != '_')
                break;
            ++position;
        }

        if (idStart == position)
            continue;

        std::string id = text.substr(idStart, position - idStart);
        std::transform(id.begin(), id.end(), id.begin(), [](unsigned char c) { return (char)std::toupper(c); });

        // Small models may add words around the ID or use this common synonym.
        if (id == "FOLLOW" || id == "FOLLOWING")
        {
            text.clear();
            return "COME";
        }

        if (id == "NONE")
        {
            foundNone = true;
            continue;
        }

        if (FindAIPlayCommand(id))
        {
            text.clear();
            return id;
        }
    }

    if (foundNone)
        text.clear();
    return std::string();
}

void AIPlayAction::StartActionSelection(PlayerbotAI* ai, const std::string& latestText, ObjectGuid ownerGuid)
{
    if (!ai || ai->aiPlayGenerationPending || !ai->GetBot() || !ai->GetBot()->IsInWorld() ||
        !ai->HasStrategy("ai play", BotState::BOT_STATE_NON_COMBAT) || !sPlayerbotAIConfig.llmEnabled)
    {
        return;
    }

    if (sPlayerbotAIConfig.llmRequirePlayerPresence && !ai->HasRealPlayerNearbyOrInGroup())
        return;

    std::string json;
    if (!BuildActionRequest(ai, latestText, json))
        return;

    ai->aiPlayGenerationPending = true;
    ObjectGuid botGuid = ai->GetBot()->GetObjectGuid();
    try
    {
        std::thread([botGuid, ownerGuid, json]()
        {
            std::string selected;
            try
            {
                std::vector<std::string> debugLines;
                std::string response = PlayerbotLLMInterface::Generate(json,
                    sPlayerbotAIConfig.llmGenerationTimeout, sPlayerbotAIConfig.llmMaxSimultaniousGenerations, debugLines);
                selected = AIPlayAction::ExtractActionIntent(response);
            }
            catch (const std::exception& e)
            {
                sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "AI play action selection error: %s", e.what());
            }

            sWorld.GetMessager().AddMessage([botGuid, ownerGuid, selected](World*)
            {
                Player* bot = sObjectAccessor.FindPlayer(botGuid);
                if (!bot || !bot->GetPlayerbotAI())
                    return;

                PlayerbotAI* botAI = bot->GetPlayerbotAI();
                botAI->aiPlayGenerationPending = false;
                if (!bot->IsInWorld())
                    return;

                ExecuteAIPlayCommand(botAI, selected, ownerGuid);
            });
        }).detach();
    }
    catch (const std::exception& e)
    {
        ai->aiPlayGenerationPending = false;
        sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "Unable to start AI play action selection: %s", e.what());
    }
}

bool AIPlayAction::isUseful()
{
    if (!sPlayerbotAIConfig.llmEnabled || !ai->HasStrategy("ai play", BotState::BOT_STATE_NON_COMBAT))
        return false;

    if (sPlayerbotAIConfig.llmRequirePlayerPresence && !ai->HasRealPlayerNearbyOrInGroup())
        return false;

    return !ai->aiPlayGenerationPending && (!ai->nextAIPlayGenerationTime || time(nullptr) >= ai->nextAIPlayGenerationTime);
}

bool AIPlayAction::Execute(Event& event)
{
    (void)event;
    TryStartAutonomous(ai);
    return true;
}

void AIPlayAction::TryStartAutonomous(PlayerbotAI* ai)
{
    if (!ai)
        return;

    AIPlayAction* action = dynamic_cast<AIPlayAction*>(ai->GetAiObjectContext()->GetAction("ai play"));
    if (!action || !action->isUseful())
        return;

    const time_t now = time(nullptr);
    if (!ai->nextAIPlayGenerationTime)
    {
        ai->nextAIPlayGenerationTime = now + NextControlInterval();
        return;
    }

    ai->nextAIPlayGenerationTime = now + NextControlInterval();
    StartActionSelection(ai, "", ai->GetMaster() ? ai->GetMaster()->GetObjectGuid() : ObjectGuid());
}

bool AIPlayAction::ProcessPlayerMessage(PlayerbotAI* ai, uint32 type, ObjectGuid sender, ObjectGuid receiver, const std::string& text)
{
    (void)receiver;
    if (!ai || !ai->HasStrategy("ai play", BotState::BOT_STATE_NON_COMBAT) || !sPlayerbotAIConfig.llmEnabled)
        return false;

    if (sPlayerbotAIConfig.llmRequirePlayerPresence && !ai->HasRealPlayerNearbyOrInGroup())
        return false;

    Player* player = sObjectAccessor.FindPlayer(sender);
    if (!player || !player->IsInWorld() || !ai->IsRealPlayer(player))
        return false;

    Player* bot = ai->GetBot();
    const bool addressed = type == CHAT_MSG_WHISPER;
    Group* group = bot->GetGroup();
    const bool grouped = group && player->GetGroup() == group;
    const std::string normalizedText = LowerAIPlayText(text);
    const std::string loweredBotName = LowerAIPlayText(bot->GetName());
    const bool mentioned = !loweredBotName.empty() && normalizedText.find(loweredBotName) != std::string::npos;
    if (!addressed && !grouped && !mentioned)
        return false;

    if (!ai->aiPlayContext.empty())
        ai->aiPlayContext += "\n";
    ai->aiPlayContext += std::string(player->GetName()) + ": " + text;
    if (ai->aiPlayContext.size() > 32768)
        ai->aiPlayContext.erase(0, ai->aiPlayContext.size() - 32768);

    // The following generated bot reply will be classified together with this
    // player message, so a turn produces at most one selected action.
    return false;
}

bool AIPlayAction::ProcessGeneratedText(PlayerbotAI* ai, const std::string& text, bool appendContext, Player* owner)
{
    if (!ai || !ai->HasStrategy("ai play", BotState::BOT_STATE_NON_COMBAT))
        return false;

    if (appendContext && ai->GetBot() && !text.empty())
    {
        if (!ai->aiPlayContext.empty())
            ai->aiPlayContext += "\n";
        ai->aiPlayContext += std::string(ai->GetBot()->GetName()) + ": " + text;
        if (ai->aiPlayContext.size() > 32768)
            ai->aiPlayContext.erase(0, ai->aiPlayContext.size() - 32768);
    }

    StartActionSelection(ai, text, owner ? owner->GetObjectGuid() : ObjectGuid());
    return ai->aiPlayGenerationPending;
}

void AIPlayAction::QueueGeneratedResponse(ObjectGuid botGuid, ObjectGuid ownerGuid, const std::string& text)
{
    sWorld.GetMessager().AddMessage([botGuid, ownerGuid, text](World*)
    {
        Player* bot = sObjectAccessor.FindPlayer(botGuid);
        if (!bot || !bot->IsInWorld() || !bot->GetPlayerbotAI())
            return;

        Player* owner = ownerGuid.IsEmpty() ? nullptr : sObjectAccessor.FindPlayer(ownerGuid);
        AIPlayAction::ProcessGeneratedText(bot->GetPlayerbotAI(), text, true, owner);
    });
}

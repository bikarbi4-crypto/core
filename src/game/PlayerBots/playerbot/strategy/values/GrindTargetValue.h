#pragma once
#include <unordered_map>
#include <vector>

#include "playerbot/strategy/Value.h"
#include "TargetValue.h"

class Group;

namespace ai
{
   
    class GrindTargetValue : public TargetValue
	{
	public:
        GrindTargetValue(PlayerbotAI* ai, std::string name = "grind target") : TargetValue(ai, name, 6) {}

    public:
        Unit* Calculate() override;

    private:
        // Owned by one Calculate invocation. List/travel Value reads stay on
        // their original paths so refresh/Reset policies are not frozen.
        struct CalculationScratch
        {
            Group* group = nullptr;
            bool groupReady = false;
            std::vector<Player*> groupMembers;
        };

        void PrepareGroupMembers(Group* group, CalculationScratch& scratch);
        int GetTargetingPlayerCount(Unit* unit);
        Unit* FindTargetForGrinding(int assistCount, std::unordered_map<uint32, bool>& needForQuestCache, CalculationScratch& scratch);
    };
}

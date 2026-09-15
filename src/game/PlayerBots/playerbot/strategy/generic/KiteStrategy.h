#pragma once
#include "playerbot/strategy/Strategy.h"

namespace ai
{
    class KiteLineOfSightMultiplier : public Multiplier
    {
    public:
        KiteLineOfSightMultiplier(PlayerbotAI* ai) : Multiplier(ai, "kite line of sight") {}

        float GetValue(Action* action) override;
    };

    class KiteMeleeMultiplier : public Multiplier
    {
    public:
        KiteMeleeMultiplier(PlayerbotAI* ai) : Multiplier(ai, "kite melee") {}

        float GetValue(Action* action) override;
    };

    class KiteStrategy : public Strategy
    {
    public:
        KiteStrategy(PlayerbotAI* ai) : Strategy(ai) {}

        std::string getName() override { return "kite"; }

        int GetType() override { return STRATEGY_TYPE_COMBAT | STRATEGY_TYPE_RANGED; }

        static float GetMinDistance() { return 15.0f; }
        static float GetPreferredDistance() { return 30.0f; }
        static float GetSettleDistance() { return 35.0f; }
        static float GetMaxDistance() { return 40.0f; }
        static float GetScanDistance() { return GetMaxDistance() + GetMinDistance() + 5.0f; }
        static std::list<Unit*> GetNearbyHostiles(PlayerbotAI* ai);

    protected:
        NextAction** GetDefaultCombatActions() override;
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitReactionTriggers(std::list<TriggerNode*>& triggers) override;
        void InitCombatMultipliers(std::list<Multiplier*>& multipliers) override;
    };

    class KiteStackStrategy : public KiteStrategy
    {
    public:
        KiteStackStrategy(PlayerbotAI* ai) : KiteStrategy(ai) {}

        std::string getName() override { return "kite stack"; }

    protected:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitReactionTriggers(std::list<TriggerNode*>& triggers) override;
    };
} // namespace ai

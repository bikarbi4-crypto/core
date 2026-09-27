#pragma once
#include AUDIT_HEADER
#undef time
namespace AUDIT_NAMESPACE {
struct State {
    Case scenario;
    Player player;
    std::unique_ptr<AiObjectContext> context;
    std::unique_ptr<SharedObjectContext> shared;
    std::unique_ptr<NeedForQuestValue> need;
    Value<std::list<int32>>* dropValue=nullptr;
    std::vector<std::unique_ptr<TravelDestination>> destinations;
    PurposeDestinationMap destinationMap;
};
void Bind(State&);
void* Create(const Case&);
void Destroy(void*);
void Policy();
}

#include "value_fixture.h"

static void RefreshMatrix() {
    for (int interval : {0, 1, 2, 3, 4, 5, 6, 30}) {
        for (bool monitor : {false, true}) {
            fixtureTime = 10000;
            sPlayerbotAIConfig.perfMonEnabled = monitor;
            OldList oldValue(interval);
            NewList newValue(interval);
            oldValue.generatedSize = newValue.generatedSize = 7;
            for (unsigned step = 0; step < 120; ++step) {
                if (step % 3 == 0) ++fixtureTime;
                if (step % 17 == 0) { oldValue.Reset(); newValue.Reset(); }
                if (step % 11 == 0) {
                    std::list<ObjectGuid> setValue(step % 9, ObjectGuid(55));
                    oldValue.Set(setValue); newValue.Set(setValue);
                }
                if (step % 13 == 0) oldValue.generatedSize = newValue.generatedSize = step % 12;
                auto operation = [step](auto& value, bool modern) -> std::size_t {
                    using V = std::decay_t<decltype(value)>;
                    if (step % 4 == 2) return value.LazyGet().size();
                    if (step % 4 == 3) return value.Get().size();
                    if constexpr (std::is_same_v<V, NewList>) {
                        (void)modern;
                        return step % 4 == 0 ? value.GetSize() : static_cast<std::size_t>(value.IsEmpty());
                    } else {
                        (void)modern;
                        return step % 4 == 0 ? value.Get().size() : static_cast<std::size_t>(value.Get().empty());
                    }
                };
                auto before = sPerformanceMonitor.starts;
                auto oldResult = operation(oldValue, false);
                auto oldStarts = sPerformanceMonitor.starts - before;
                before = sPerformanceMonitor.starts;
                auto newResult = operation(newValue, true);
                assert(oldResult == newResult);
                assert(oldStarts == sPerformanceMonitor.starts - before);
                assert(oldValue.calculates == newValue.calculates);
                assert(oldValue.LastCheck() == newValue.LastCheck());
                assert(oldValue.Expired() == newValue.Expired());
                assert(PerformanceMonitorOperation::live == 0);
            }
        }
    }
}

static void NoCopiesAndReentry() {
    static_assert(sizeof(v16::ObjectGuidListCalculatedValue)==sizeof(v17::ObjectGuidListCalculatedValue));
    sPlayerbotAIConfig.perfMonEnabled = false;
    fixtureTime = 10000;
    OldList oldValue(2); NewList newValue(2);
    oldValue.generatedSize = newValue.generatedSize = 17;
    assert(oldValue.Get().size() == newValue.GetSize());
    ObjectGuid::copies = 0;
    assert(oldValue.Get().size() == 17 && ObjectGuid::copies == 17);
    ObjectGuid::copies = 0;
    assert(newValue.GetSize() == 17 && !newValue.IsEmpty());
    assert(ObjectGuid::copies == 0);
    static_assert(std::is_same_v<decltype(newValue.GetSize()), std::size_t>);
    static_assert(std::is_same_v<decltype(newValue.IsEmpty()), bool>);
    auto oldRead = oldValue.Get();
    auto newRead = newValue.Get();
    oldRead.clear(); newRead.clear();
    assert(oldValue.Get().size() == newValue.GetSize()); // Get still returns an independent copy.

    std::size_t oldNested = 0, newNested = 0;
    oldValue.onCalculate = [&] { oldNested = oldValue.Get().size(); };
    newValue.onCalculate = [&] { newNested = newValue.GetSize(); };
    oldValue.Reset(); newValue.Reset();
    oldValue.generatedSize = newValue.generatedSize = 5;
    assert(oldValue.Get().size() == newValue.GetSize());
    assert(oldNested == 17 && newNested == oldNested);
    oldValue.onCalculate = [&] { oldValue.Reset(); };
    newValue.onCalculate = [&] { newValue.Reset(); };
    oldValue.Reset(); newValue.Reset();
    for (int i = 0; i < 3; ++i) assert(oldValue.Get().size() == newValue.GetSize());
    assert(oldValue.calculates == newValue.calculates);
    std::size_t observedSize=0;
    bool observedEmpty=true;
    {
        auto owner=std::make_unique<NewList>(2);
        owner->generatedSize=9;
        observedSize=owner->GetSize(); observedEmpty=owner->IsEmpty();
        owner->Set({}); owner->Reset();
    }
    assert(observedSize==9 && !observedEmpty); // Remain valid after Set/Reset/owner destruction.
}

template<class Base> class ManualFilter : public Base {
public:
    ManualFilter() : Base(nullptr, std::list<ObjectGuid>{1,2,3}, "filter") {}
    std::list<ObjectGuid> Get() override {
        ++gets;
        if (!this->value.empty()) this->value.pop_front();
        return this->value;
    }
    unsigned gets = 0;
};
template<class Base> class SingleProbe : public Base {
public:
    SingleProbe() : Base(nullptr, "single") {}
    unsigned calculates = 0;
    std::list<ObjectGuid> Calculate() override { ++calculates; return {1,2,3}; }
};
template<class Base> class MemoryProbe : public Base {
public:
    MemoryProbe() : Base(nullptr, "memory", 6) {}
    float Calculate() override { return 2.0f; }
    bool EqualToLast(float input) override { return input == this->lastValue; }
};

static void OtherPoliciesKeepVirtualGet() {
    ManualFilter<v16::ManualSetValue<std::list<ObjectGuid>>> oldManual;
    ManualFilter<v17::ManualSetValue<std::list<ObjectGuid>>> newManual;
    for (int i = 0; i < 8; ++i) {
        assert(oldManual.Get().size() == newManual.GetSize());
        assert(oldManual.Get().empty() == newManual.IsEmpty());
        assert(oldManual.gets == newManual.gets);
        if (i == 4) { oldManual.Reset(); newManual.Reset(); }
    }
    SingleProbe<v16::SingleCalculatedValue<std::list<ObjectGuid>>> oldSingle;
    SingleProbe<v17::SingleCalculatedValue<std::list<ObjectGuid>>> newSingle;
    for (int i = 0; i < 5; ++i) {
        fixtureTime += 100;
        assert(oldSingle.Get().size() == newSingle.GetSize());
        assert(oldSingle.calculates == newSingle.calculates);
    }
    assert(newSingle.calculates == 1);
    newSingle.Reset(); oldSingle.Reset();
    assert(oldSingle.Get().size() == newSingle.GetSize());
    assert(newSingle.calculates == 2);
    MemoryProbe<v16::MemoryCalculatedValue<float>> oldMemory;
    MemoryProbe<v17::MemoryCalculatedValue<float>> newMemory;
    for (int i = 0; i < 10; ++i) {
        ++fixtureTime;
        if (i == 3) { oldMemory.Set({3}); newMemory.Set({3}); }
        if (i == 5) { oldMemory.Reset(); newMemory.Reset(); }
        assert(oldMemory.Get() == newMemory.Get());
        assert(oldMemory.GetLastTime() == newMemory.GetLastTime());
        assert(oldMemory.GetLastValue() == newMemory.GetLastValue());
    }
}

static void ProductionConsumers() {
    for (std::size_t size : {0u, 1u, 17u, 255u, 256u, 300u}) {
        fixtureTime += 10;
        OldList oldList(2); NewList newList(2);
        oldList.generatedSize = newList.generatedSize = size;
        v16::ValueTestContext oldContext; v17::ValueTestContext newContext;
        for (const char* name : {"attackers", "possible attack targets", "attackers::1", "possible attack targets::1", "enemy player targets::1"}) {
            oldContext.values[name] = &oldList; newContext.values[name] = &newList;
        }
        assert(v16::AttackersCountValue{&oldContext}.Calculate() == v17::AttackersCountValue{&newContext}.Calculate());
        assert(v16::PossibleAttackTargetsCountValue{&oldContext}.Calculate() == v17::PossibleAttackTargetsCountValue{&newContext}.Calculate());
        assert(v16::HasAttackersValue{&oldContext}.Calculate() == v17::HasAttackersValue{&newContext}.Calculate());
        assert(v16::HasPossibleAttackTargetsValue{&oldContext}.Calculate() == v17::HasPossibleAttackTargetsValue{&newContext}.Calculate());
        assert(v16::HasEnemyPlayersValue{&oldContext}.Calculate() == v17::HasEnemyPlayersValue{&newContext}.Calculate());
        assert(oldContext.requests == newContext.requests);
    }
}

int main() {
    RefreshMatrix(); NoCopiesAndReentry(); OtherPoliciesKeepVirtualGet(); ProductionConsumers();
    std::cout << "PASS: production V16/V17 refresh, PMO, Calculate, LazyGet, Set, Reset, reentry, manual/single/memory policies, five consumers and zero element copies; scalar results cannot dangle.\n";
}

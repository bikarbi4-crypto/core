#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <sstream>
#include <vector>

using uint8 = std::uint8_t;
using int8 = std::int8_t;
using uint32 = std::uint32_t;
using int32 = std::int32_t;
class PlayerbotAI {};

// Exercise the production cache AND the production factory/context integration.
#include "NamedObjectCache.h"
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4458) // Existing Qualified constructor parameter names.
#endif
#include "NamedObjectContext.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

static void Compare(ai::NamedObjectCache<int>& cache, const std::map<std::string, int*>& reference)
{
    auto found = cache.begin();
    for (const auto& item : reference)
    {
        assert(found != cache.end());
        assert(found->first == item.first && found->second == item.second);
        assert(cache.find(item.first) == found);
        ++found;
    }
    assert(found == cache.end());
}

static void DifferentialCache()
{
    int objects[32] = {};
    std::vector<std::string> keys = {"", "a", "aa", "ab", std::string("a\0b", 3), std::string(1, '\x80'), std::string(1, '\xff')};
    for (int i = 0; i < 1024; ++i)
        keys.push_back(std::string(i % 200, 'x') + std::to_string(i));

    for (unsigned seed = 1; seed <= 8; ++seed)
    {
        std::mt19937 random(seed);
        ai::NamedObjectCache<int> cache;
        std::map<std::string, int*> reference;
        for (int step = 0; step < 50000; ++step)
        {
            const auto& key = keys[random() % keys.size()];
            switch (random() % 4)
            {
                case 0:
                case 1:
                {
                    int* value = random() % 3 ? &objects[random() % 32] : nullptr;
                    cache[key] = value;
                    reference[key] = value;
                    break;
                }
                case 2:
                    cache.erase(key);
                    reference.erase(key);
                    break;
                default:
                {
                    auto a = cache.find(key);
                    auto b = reference.find(key);
                    assert((a == cache.end()) == (b == reference.end()));
                    if (b != reference.end())
                        assert(a->second == b->second);
                    break;
                }
            }
            if (step % 127 == 0)
                Compare(cache, reference);
        }
        Compare(cache, reference);
        cache.clear();
        assert(cache.begin() == cache.end());
        cache["reused"] = nullptr;
        assert(cache.find("reused") != cache.end());
    }

    // Sorted insertion/deletion stresses balancing; saved node addresses must survive
    // rotations and removal of every other node (including two-child transplants).
    ai::NamedObjectCache<int> cache;
    std::map<std::string, int*> reference;
    std::map<std::string, const void*> addresses;
    for (int i = 0; i < 20000; ++i)
    {
        const std::string key = std::to_string(100000 + i);
        cache[key] = reference[key] = &objects[i % 32];
        addresses[key] = &*cache.find(key);
    }
    for (int i = 19998; i >= 0; i -= 2)
    {
        const std::string key = std::to_string(100000 + i);
        cache.erase(key);
        reference.erase(key);
        addresses.erase(key);
    }
    Compare(cache, reference);
    for (const auto& entry : addresses)
        assert(&*cache.find(entry.first) == entry.second);
}

struct Object : ai::Qualified
{
    inline static int live = 0;
    std::string name;
    std::function<void()> onUpdate;
    explicit Object(std::string name) : name(std::move(name)) { ++live; }
    virtual ~Object() { --live; }
    void Update() { if (onUpdate) onUpdate(); }
    void Reset() {}
};

struct Context : ai::NamedObjectContext<Object>
{
    using ai::NamedObjectContext<Object>::creators;
};

static void ContextContracts()
{
    {
        Context context;
        int misses = 0;
        context.creators["null"] = [&](PlayerbotAI*) -> Object* { ++misses; return nullptr; };
        assert(context.Create("null", nullptr) == nullptr);
        assert(context.Create("null", nullptr) == nullptr);
        assert(context.IsCreated("null") && misses == 1);
        context.Erase("null");
        assert(!context.IsCreated("null"));
        context.Create("null", nullptr);
        assert(misses == 2);

        context.creators["value"] = [](PlayerbotAI*) { return new Object("value"); };
        Object* value = context.Create("value::a::b", nullptr);
        assert(value->getQualifier() == "a::b");
        assert(context.Create("value::a::b", nullptr) == value);

        // V8's key snapshot must survive a reentrant creator modifying the caller's name.
        std::string name = "outer";
        context.creators["outer"] = [&](PlayerbotAI*)
        {
            name = "changed";
            context.Create("value::nested", nullptr);
            return new Object("outer");
        };
        context.Create(name, nullptr);
        assert(context.IsCreated("outer") && !context.IsCreated("changed"));
        assert(context.IsCreated("value::nested"));
        context.Clear();
        assert(Object::live == 0);
    }

    {
        ai::NamedObjectContextList<Object> list;
        auto* first = new Context;
        auto* second = new Context;
        int firstCalls = 0;
        int secondCalls = 0;
        first->creators["same"] = [&](PlayerbotAI*) -> Object* { ++firstCalls; return nullptr; };
        second->creators["same"] = [&](PlayerbotAI*) { ++secondCalls; return new Object("second"); };
        list.Add(first);
        list.Add(second);
        Object* object = list.GetObject("same", nullptr);
        assert(object && list.GetObject("same", nullptr) == object);
        assert(firstCalls == 1 && secondCalls == 1);
        list.Erase("same");
        assert(Object::live == 0);
    }

    {
        Context context;
        std::vector<std::string> visited;
        for (const std::string key : {"a", "b", "c", "d", "e"})
            context.creators[key] = [&, key](PlayerbotAI*)
            {
                auto* object = new Object(key);
                object->onUpdate = [&, key]
                {
                    visited.push_back(key);
                    if (key == "b")
                        for (const char* inserted : {"a", "c", "e"})
                            context.Create(inserted, nullptr);
                };
                return object;
            };
        context.Create("b", nullptr);
        context.Create("d", nullptr);
        context.Update();
        assert((visited == std::vector<std::string>{"b", "c", "d", "e"}));
        assert((context.GetCreated() == std::set<std::string>{"a", "b", "c", "d", "e"}));
    }
    assert(Object::live == 0);
}

int main()
{
    DifferentialCache();
    ContextContracts();
    std::cout << "PASS: 400000 differential operations; 20000 sorted inserts; stable nodes; cached nulls; qualified names; reentrant factories and ordered updates; ownership.\n";
}

#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>

using uint32 = std::uint32_t;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4458) // Existing production setter parameter names.
#endif
#include "action_contracts.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

static NextAction** Make(int kind)
{
    if (kind < 0)
        return nullptr;
    auto** result = new NextAction*[kind + 1];
    for (int i = 0; i < kind; ++i)
    {
        const std::string name = i % 2 ? std::string("a\0b", 3) : std::string(300, 'x');
        const uint32 bits = i % 2 ? 0x80000000u : 0x7fc01234u;
        float relevance;
        std::memcpy(&relevance, &bits, sizeof(bits));
        result[i] = new NextAction(name, relevance);
    }
    result[kind] = nullptr;
    return result;
}

static void CheckEqual(NextAction** a, NextAction** b)
{
    assert(a && b);
    assert(NextAction::size(a) == NextAction::size(b));
    for (int i = 0; a[i]; ++i)
    {
        assert(a[i]->getNameRef() == b[i]->getNameRef());
        float av = a[i]->getRelevance();
        float bv = b[i]->getRelevance();
        assert(std::memcmp(&av, &bv, sizeof(float)) == 0);
    }
}

struct Callback : Action, Trigger
{
    NextAction** templateList = nullptr;
    int calls = 0;
    NextAction** Invoke()
    {
        ++calls;
        // A callback can update the template. The clone must observe this value.
        *templateList[0] = NextAction("changed by callback", 17.0f);
        return nullptr;
    }
    NextAction** getContinuers() override { return Invoke(); }
    NextAction** getAlternatives() override { return Invoke(); }
    NextAction** getPrerequisites() override { return Invoke(); }
    NextAction** getHandlers() override { return Invoke(); }
};

int main()
{
    for (int a : {-1, 0, 1, 3})
        for (int b : {-1, 0, 1, 3})
        {
            NextAction** left = Make(a);
            NextAction** right = Make(b);
            NextAction** expected = NextAction::merge(Make(a), Make(b));
            NextAction** actual = NextAction::mergeOwned(left, right);
            CheckEqual(actual, expected);
            if (a >= 0 && b < 0)
                assert(actual == left);
            if (a < 0 && b >= 0)
                assert(actual == right);
            NextAction::destroy(actual);
            NextAction::destroy(expected);
        }

    for (int method = 0; method < 4; ++method)
    {
        Callback callback;
        auto** templateList = Make(1);
        callback.templateList = templateList;
        NextAction** output = nullptr;
        if (method == 3)
        {
            TriggerNode node("trigger", templateList);
            node.setTrigger(&callback);
            output = node.getHandlers();
        }
        else
        {
            ActionNode node("action", method == 0 ? templateList : nullptr,
                method == 1 ? templateList : nullptr, method == 2 ? templateList : nullptr);
            node.setAction(&callback);
            output = method == 0 ? node.getPrerequisites() : method == 1 ? node.getAlternatives() : node.getContinuers();
        }
        // The template was destroyed with its node; output is independently owned.
        assert(callback.calls == 1);
        assert(output && output[0] && !output[1]);
        assert(output[0]->getNameRef() == "changed by callback");
        assert(output[0]->getRelevance() == 17.0f);
        NextAction::destroy(output);
    }
    std::cout << "PASS: production V11-V13 ownership matrix, null/empty distinction, exact float bits, names, all four callback-before-clone paths.\n";
}

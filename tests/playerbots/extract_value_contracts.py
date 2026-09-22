"""Exercise checked-out Value policies and consumers against the accepted V16 source.

Only game services and the clock are substituted. The implementation bodies are
extracted verbatim; the baseline is read from its immutable Git commit.
"""
from pathlib import Path
import re
import subprocess
import sys

BASELINE = "cea7ba955f5c0d57ff76d63e53f97f34b1d3ab4c"
ROOT = Path(__file__).resolve().parents[2]
STRATEGY = "src/game/PlayerBots/playerbot/strategy/"


def source(path, baseline):
    if baseline:
        return subprocess.check_output(["git", "show", f"{BASELINE}:{path}"], cwd=ROOT).decode("utf-8-sig")
    return (ROOT / path).read_text(encoding="utf-8-sig")


def block(text, signature, semicolon=False):
    start = text.index(signature)
    opening = text.index("{", start)
    level, end = 1, opening + 1
    while level:
        if text[end] == "{": level += 1
        elif text[end] == "}": level -= 1
        end += 1
    return text[start:end] + (";" if semicolon else "")


result = ["// Generated from production source; do not edit."]
for ns, baseline in (("v16", True), ("v17", False)):
    h = source(STRATEGY + "Value.h", baseline)
    begin = re.search(r"    template<class T>\s+class Value(?:Base)?\b", h).start()
    finish = h.index("    template<class T>\n    class CalculatedValue", begin)
    result += [f"namespace {ns} {{", h[begin:finish]]
    for signature in ("template<class T>\n    class CalculatedValue", "template <class T> class SingleCalculatedValue",
                      "template<class T> class MemoryCalculatedValue", "template<class T> class LogCalculatedValue",
                      "class ObjectGuidListCalculatedValue", "template<class T>\n    class ManualSetValue"):
        result.append(block(h, signature, True))
    result.append('inline std::string ObjectGuidListCalculatedValue::Format() { return "fixture"; }')
    result.append("""
struct ValueTestContext {
    std::map<std::string, Value<std::list<ObjectGuid>>*> values;
    std::vector<std::string> requests;
    template<class T> Value<T>* GetValue(std::string name, int qualifier = -1) {
        static_assert(std::is_same_v<T, std::list<ObjectGuid>>);
        if (qualifier != -1) name += "::" + std::to_string(qualifier);
        requests.push_back(name);
        return values.at(name);
    }
};
class AttackersCountValue { public: ValueTestContext* context; uint8 Calculate(); };
class PossibleAttackTargetsCountValue { public: ValueTestContext* context; uint8 Calculate(); };
class HasAttackersValue { public: ValueTestContext* context; bool Calculate(); };
class HasPossibleAttackTargetsValue { public: ValueTestContext* context; bool Calculate(); };
class HasEnemyPlayersValue { public: ValueTestContext* context; bool Calculate(); };
""")
    c = source(STRATEGY + "values/AttackerCountValues.cpp", baseline)
    for signature in ("uint8 AttackersCountValue::Calculate()", "uint8 PossibleAttackTargetsCountValue::Calculate()",
                      "bool HasAttackersValue::Calculate()", "bool HasPossibleAttackTargetsValue::Calculate()"):
        result.append(block(c, signature))
    result.append(block(source(STRATEGY + "values/EnemyPlayerValue.cpp", baseline), "bool HasEnemyPlayersValue::Calculate()"))
    result.append("}")
Path(sys.argv[1]).write_text("\n\n".join(result), encoding="utf-8")

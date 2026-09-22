"""Compile the production list/queue code without loading a game world.

Only the game-facing Action/Trigger callbacks are substituted by small test doubles.
The class declarations and list implementations are read from the checked-out source,
not copied implementations of the algorithms being tested.
"""
from pathlib import Path
import sys

root = Path(__file__).resolve().parents[2] / "src/game/PlayerBots/playerbot/strategy"


def block(text, signature, semicolon=False):
    start = text.index(signature)
    opening = text.index("{", start)
    level = 1
    end = opening + 1
    while level:
        if text[end] == "{":
            level += 1
        elif text[end] == "}":
            level -= 1
        end += 1
    return text[start:end] + (";" if semicolon else "")


header = (root / "Action.h").read_text(encoding="utf-8-sig")
source = (root / "Action.cpp").read_text(encoding="utf-8-sig")
trigger_header = (root / "Trigger.h").read_text(encoding="utf-8-sig")
trigger_source = (root / "Trigger.cpp").read_text(encoding="utf-8-sig")
result = ["namespace ai {", block(header, "class NextAction\n", True)]
result += ["""
class Action {
public:
    virtual ~Action() = default;
    virtual NextAction** getContinuers() = 0;
    virtual NextAction** getAlternatives() = 0;
    virtual NextAction** getPrerequisites() = 0;
};
class Trigger {
public:
    virtual ~Trigger() = default;
    virtual NextAction** getHandlers() = 0;
};
""", block(header, "class ActionNode\n", True),
    block(trigger_header, "class TriggerNode\n", True), "}", "using namespace ai;"]
for name in ("int NextAction::size(", "NextAction** NextAction::clone(",
             "NextAction** NextAction::merge(", "NextAction** NextAction::mergeOwned(",
             "void NextAction::destroy("):
    result.append(block(source, name))
result += [block(trigger_source, "NextAction** TriggerNode::getHandlers("),
           block(trigger_source, "TriggerNode::~TriggerNode("),
           block(trigger_source, "float TriggerNode::getFirstRelevance(")]
Path(sys.argv[1]).write_text("\n\n".join(result), encoding="utf-8")

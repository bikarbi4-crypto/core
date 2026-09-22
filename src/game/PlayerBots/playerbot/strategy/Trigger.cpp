
#include "playerbot/playerbot.h"
#include "Trigger.h"
#include "Action.h"
#include "Unit.h"
#include "Value.h"

using namespace ai;

Event Trigger::Check()
{
	if (triggered)
	{
		if (owner)
			return Event(getName(), param, owner);
		else
			return Event(getName());
	}

	if (IsActive())
	{
		triggered = true;
		return Event(getName());
	}

	return Event();
}

Value<Unit*>* Trigger::GetTargetValue()
{
    return context->GetValue<Unit*>(GetTargetName());
}

Unit* Trigger::GetTarget()
{
    return GetTargetValue()->Get();
}

TriggerNode::~TriggerNode()
{
	NextAction::destroy(handlers);
}

NextAction** TriggerNode::getHandlers()
{
    // The binary baseline invokes the callback before cloning the template.
    // Spell out the order instead of relying on argument evaluation order.
    NextAction** dynamic = trigger->getHandlers();
    return NextAction::mergeOwned(NextAction::clone(handlers), dynamic);
}

float TriggerNode::getFirstRelevance()
{
	return handlers[0] ? handlers[0]->getRelevance() : -1;
}

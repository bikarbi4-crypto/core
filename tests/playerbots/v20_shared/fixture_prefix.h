#pragma once
#include "Common.h"
#include "Policies/Singleton.h"
#include <mutex>
#include "allocations.h"
class ObjectMgr;
class TerrainManager;
class PlayerbotAI;
#include "audit_monitor.h"
// These game singletons are unreachable in the botless constructor test.
// Any accidental attempt to use a live-world service aborts the test.
namespace MaNGOS {
template<> ObjectMgr& Singleton<ObjectMgr>::Instance();
template<> TerrainManager& Singleton<TerrainManager, ClassLevelLockable<TerrainManager, std::mutex>>::Instance();
}

#include "PlayerActivityPresence.h"
#include "measurements.h"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>
#undef sPlayerActivityPresence
#define sPlayerActivityPresence ::NAMESPACE::presence
namespace baseline {
#define NAMESPACE baseline
#include "fixture.h"
#define AI_VALUE(T,name) Value<T>(name)
#include "baseline.inc"
#undef AI_VALUE
#undef NAMESPACE
}
namespace candidate {
#define CANDIDATE
#define NAMESPACE candidate
#include "fixture.h"
#define AI_VALUE(T,name) Value<T>(name)
#include "candidate.inc"
#undef AI_VALUE
#undef NAMESPACE
#undef CANDIDATE
}
#undef sPlayerActivityPresence

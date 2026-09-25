#include "PresenceDiagnostics.h"
#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <map>
#include <new>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>
namespace baseline {
#define NAMESPACE baseline
#include "fixture.h"
#define AI_VALUE(T, name) Value<T>(name)
#include "baseline.inc"
#undef AI_VALUE
#undef NAMESPACE
}
namespace candidate {
#define NAMESPACE candidate
#include "fixture.h"
#define AI_VALUE(T, name) Value<T>(name)
#include "candidate.inc"
#undef AI_VALUE
#undef NAMESPACE
}

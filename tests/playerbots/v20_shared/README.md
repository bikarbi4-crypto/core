# V20 A: actual shared-context lifetime and constructor cost

Baseline is immutable V19 `33faa8e3c091d73483b42dc07f8f2934ba5f7958`.
The generator compiles the real PlayerbotAI header/layout (1344 bytes on the
tested MSVC build), default constructor and destructor, PlayerbotAIBase constructor,
AiObject constructor, ChatHelper constructor with its table assignments, all 15
concrete chat-filter classes' constructors/member storage and CompositeChatFilter
constructor/destructor, security constructor, and WorldPosition bookkeeping.
It preserves activity timers and standard container construction/destruction.
Only uncalled game methods/virtual slots and live-world singletons are aborting
fixtures. A call to one fails the test; no game world or database is initialized.

The actual headers retain NamedObjectFactory, NamedObjectContext, its hash cache,
NamedObjectContextList, Value policies and ItemDropListValue. SharedObjectContext
comes from the immutable baseline or checkout. The AI overload and underlying
PMO start/operation bodies are extracted unchanged from production. Monitor
member visibility alone is changed for observations of actual names/counts.
Hashes/assertions cover unchanged production constructors, headers, Value/cache
policies, PMO bodies and all 16 actual shared registration expressions.

The fixture registry uses the **actual** item-drop-list/global-string registrations
plus controlled probe, reentrant, throwing and null factories. Other production
factories are audited in `docs/V20_SHARED_OWNERSHIP_AUDIT.md`, not replaced with
claims of a full world simulation. ItemDropListValue Calculate returns a fixed
list fixture; actual world loot enumeration is not measured. A configuration
fixture supplies only the stable perfMonEnabled flag needed by these paths.

## Confirmed baseline defect

`shared_baseline --repro` follows the original `new PlayerbotAI -> real factory/
cache -> delete AI -> real SingleCalculatedValue::Get -> real PMO start` path.
MSVC ASan detects **heap-use-after-free** reading GetAiObjectContext inside PMO
start. The deleted allocation is the actual 1344-byte PlayerbotAI, not a dummy.
The candidate survives the same path and explicitly destroys values before its
owned AI/chat. Cache hits no longer construct temporary AIs.

CTest on ASan runs this baseline reproducer in a child and requires the specific
UAF, allocation/free and named call-chain diagnostics. An arbitrary nonzero exit
does not pass. This expected baseline failure is distinct from clean candidate
ASan results. The legal baseline OFF contracts also run under ASan.

## Contracts and limitations

Checks cover identity, cross-cast failure, qualifier bytes, missing/null negative
cache, repeated access, Set, Reset, LazyGet, Expired(0) without changing single-
calculation semantics, PMO OFF/ON/warm toggles, recursive factory/value reads,
factory/Calculate exceptions and their existing post-throw timestamp policy.
The candidate also checks full owner exception unwinding, erase/recreation and
destructors that access retained AI/chat to verify destruction order.

The OFF functional trace is compared by the same explicit expectations for V19
and the candidate. With ON, the candidate's actual monitor records the original
names in map/instance 0/0: 48 item-drop-list and 64 probe samples in the contract
matrix; no new qualifiers or stack components are introduced. V19 cold/reset ON
is undefined due to the demonstrated bug and is **not** normalized as parity.
Durations use the unchanged real clock; exact wall-clock duration equality is not
asserted. The separate cumulative PMO suite checks its deterministic timing policy.

Eight threads exercise concurrent same-key first factory lookup, distinct-key
insertion/negative cache and already-published immutable warm reads. They use an
already-created owner. The mutex protects lookup/factory operations, not arbitrary
concurrent value Calculate/Set/Reset, PMO reporting, singleton first initialization
or shutdown. ASan is not a race detector or evidence of general thread safety.

V19 leaks its borrowed shared registry, and a throwing factory can also skip its
temporary AI delete. The baseline fixture cleans cached values after observations
but does not prolong the AI or hide the UAF. MSVC ASan does not establish absence
of all leaks. Existing raw-map ownership limits remain a separate TODO.

## Build and measure

```text
cmake -S tests/playerbots/v20_shared -B build-shared -A x64
cmake --build build-shared --config RelWithDebInfo --parallel 1
ctest --test-dir build-shared -C RelWithDebInfo --output-on-failure
```

Repeat in a separate directory with `-DSHARED_ASAN=ON`. CTest sets the ASan runtime
PATH. Use a third directory with `-DSHARED_ALLOCATIONS=ON` for allocation counts;
it is deliberately separate from both ASan and timing builds. The timing build
uses the real CRT allocator, no allocation-counter hooks, `/O2 /GL- /LTCG:OFF`,
and distinct constructor translation units with normal virtual dispatch.

`--bench KIND ITERATIONS ON` supports lookup, long_lookup, unknown_lookup, warm_get,
cold_get and reset_get. Both variants do the same number of iterations and return
the same checksum. Cold means **a cache miss for one value inside an already
created SharedObjectContext**, including erase/recreation; it is not cold owner,
static helper-map or world initialization. Owner setup is outside the timer.
Warm/reset Get includes actual typed lookup, integer qualifier formatting and
list return policy, with fixture world Calculate. PMO ON warm values are primed
OFF before the timer. Baseline cold/reset ON is rejected because it invokes UAF.
No timing is reported for that undefined execution.

Allocation builds count the real constructor's entries separately as well as
heap allocations/bytes. A construction counter is injected only into that
separate accounting build; timing constructors retain their exact body. The
local sweep alternates variants, discards warmup, preserves every raw row and
reports medians and negative results. These numbers are not server diff gains.

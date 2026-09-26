# V21 Player Presence / Activity Hot Path

Development parent: accepted V20 `169cb7712c8b704a3fe274cbf42a0cbb53fcb90a`.
Branch: `vmangos-v21-player-presence`. V0–V20, including V17 A/B/C, remain enabled.
Diagnostic `4b9d62de1c5dbe1398f5067596fa9111f00e2fdd` is evidence only, not ancestry.
This is a validation release; live gameplay acceptance belongs to the user.

## Diagnosis and the mixed registry

The supplied V20 diagnostic run showed zero observed humans after logout while
the mixed registry retained 523 entries, including after Sessions=0. In the
15:36:16–15:43:17 interval it accumulated 536,578 VISIBLE_FOR_PLAYER and 119,039
NEARBY_PLAYER decisions, zero IN_EMPTY_SERVER, 103,014 registry snapshots,
53,876,336 copied entries, 53,875,953 friend checks and 35,866,849 map-list visits.
The diagnostic collector dropped thread registrations, so these are recorded
deltas, not a complete server-wide census or CPU attribution.

Producer/ownership audit (accepted source, not inferred from progress messages):

| Producer/remover | Membership rule and purpose |
|---|---|
| CharacterHandler → `OnPlayerLogin` | Client-socket login hook, including reconnect/selfbot. Keeps the existing `IsBot && IsFreeBot` exception; otherwise upserts by GUID. |
| PlayerbotHolder login callback → `OnBotLoginRegistration` | Runs after PlayerbotAI creation. Upserts bots that are **not free at registration time**. Random/free-alt normally take the non-inserting branch. This is the active nonhuman registration producer. |
| `MovePlayerBot` | Unconditional upsert before holder transfer. No external callsites found in this tree; retained. |
| `OnPlayerLogout` | Clears master relations, disables bot and erases only that GUID, before Player deletion. Bot logout also uses this session path. Logout of one human correctly leaves other entries. |
| `GetRandomPlayer` | Unused legacy method uses numeric `operator[]` under a shared lock; can insert a null slot if called. No callsites found. Unchanged, separately recorded below. |
| Mutable `GetPlayers` / login manager | Audited callers do not populate the map. `PlayerbotLoginMgr::Update` copies it for async login planning; the mixed population inputs remain unchanged. |

The previous log does **not** record category/producer for each of the 523 GUIDs.
It cannot prove that all are free, controlled or alt bots today. Classification
is time-dependent: the async pool loads accounts matching `rndbot%`, whereas
the configured random-account list enumerates the configured numeric range;
`PlayerLoginInfo::LoginBot` sets its timed `add` flag **after** the registration
callback. These are possible paths to registration as non-free, not proof that
either explains this particular run. V21's cold upsert counters make the producer
observable without changing classification, querying a live DB, or clearing it.

Consumer inventory and disposition:

| Consumer | Disposition |
|---|---|
| `UpdateAIInternal` → async login, login-with-player gates, level sync, LFG, offline group bots | Mixed registry retained. Population and login policy are outside this release. |
| `ProcessBot`, `CheckPlayers`, `Randomize`, teleport level/count arguments | Retained mixed inputs for population, level and placement policy. |
| `CheckBgQueue`, `CheckLfg`, `AddOfflineGroupBots` | Retained mixed inputs for queue/group orchestration. |
| `LogPlayerLocation`, statistics, `GetPlayer(s/Count/Snapshot)` | Retained mixed identity and APIs. |
| `SayToGuild`, `ChannelHasRealPlayer`, `HasPlayerRelation` | Retained existing social/persistent relationship semantics, including the existing cached friend flag. These are not the measured per-priority hot loop. |
| `HasPlayers`, priority empty/friend/map/zone, `HasPlayerNearby`, Map force-active proximity | Moved to human presence. Friend checks now visit human GUIDs in ascending order with no registry copies. |
| Activity PID target selection; local activity target/report | Human presence selects the **same existing** DiffEmpty/DiffWithPlayer and local target values. Numeric settings and PID/scoring remain unchanged. |
| Two BG-join lag threshold calls to `HasPlayers` | Same thresholds/multipliers; the presence predicate now means human. |
| `HasManyPlayersNearby` | No callsites; unchanged. Its unusual squared-range versus rounded 2D distance contract is explicitly outside the measured hot-path change. |

## Definition, ownership and lifetime

A human observation exists for an **in-world character owned by a connected client
socket**, independent of bot account, GM status, PlayerbotAI or the two legacy
IsBot/isRealPlayer predicates. Random, free, alt, controlled and legacy bots have
server-created sessions without a socket and do not enter the index. Human
selfbots and client takeover of a bot character do enter it. A controlled bot
does not itself become a human; its present human master still confers priority.
GM mode continues to exclude nearby/force tests, while GM humans count for global,
map, zone and friend presence just as a real in-world user did before.

`PlayerActivityPresence` owns only GUIDs, generation tokens and scalar map,
instance, zone, coordinates and GM/camera flags. A shared mutex protects entries;
an atomic count makes the empty fast path cheap. Reads allocate/copy no lists.
Work scales with the number of humans, not bots or map-list size. Friend callbacks
receive only a GUID under a read lock, then use SocialMgr's existing lock; they
must not re-enter the presence index. No raw gameplay pointers cross a worker
through this index and no thread registration or capacity limit is involved.

Player publishes on AddToWorld; RemoveFromWorld, logout and disconnect revoke the
token. Disconnect revocation precedes the existing two-minute body retention.
SetSession explicitly handles reconnect/takeover already in world. A generation
token prevents delayed old updates/removal from affecting a replacement with the
same GUID. Zero-token movement cannot resurrect a departed player. Destruction
also revokes defensively. World constructs the singleton during its own
construction, so the index outlives World/session shutdown.

Positions are event-maintained, with no timer cache: both coordinate assignment
paths (WorldObject::Relocate and Player::RelocateToLastClientPosition), SetMap,
GM changes, camera SetView/reset and every viewpoint relocation publish scalar
changes. The existing camera observer ownership is retained: SetView requires the
same map; map motion jobs finish before the next owner phase/teardown. The human
owner and camera source are not persisted in the shared index. Creature-motion
workers may publish remote-camera coordinates through their existing observer;
only the owner's immutable GUID and atomic token are read, not its mutable body.

Nearby preserves strict `< range²`, map+instance, 3D body **or** remote camera.
ForceActiveWhenNearPlayer preserves strict 2D body-only distance. No new radius,
refresh interval or rounding is introduced. Active human masters/groups/selfbots
retain their priority; legacy bots or disconnected bodies no longer impersonate
those humans. Higher combat, instance, travel, free-alt and queue priorities stay
in their original order; an empty server does not imply every bot must take the
IN_EMPTY_SERVER branch. Activity caching still refreshes at its existing 5 seconds.

## Validation and measurement

`tests/playerbots/v21_presence` extracts the actual V20/current priority, bracket,
AllowActive/AllowActivity, nearby, force and five new Player lifecycle method
bodies. World services are deterministic fixtures; it does not start a server.
51,840 comparisons cover human priority/decision/cache behavior and identical RNG.
Additional tests cover all seven client/bot categories, login without entering,
map/instance/zone changes, character-screen logout, retained disconnected body,
reconnect, generation replacement, GM/invisibility flags, strict distance/Z and
remote-camera movement/reset. Intentional fixes are asserted against independent
expected outcomes, not incorrectly labelled equality to V20's bugs.

The scalar index is exercised with eight concurrent workers over 160 thread
lifetimes, including ASan. This proves the tested container/lifetime contracts;
it is not ThreadSanitizer or a proof of every existing map/session race.
Protected-source gates retain brackets, timers, configuration, PMO and V17–V20
source. The PMO audit still extracts all 47 real callsites and checks actual
metric arguments, guards and scope/reset order. Only the declared non-PMO V21
functions/expressions are normalized in its historical remainder comparison.

Local raw rounds, medians, operation counts and test evidence are under
`docs/validation/v21-local-*`; fresh CI evidence is separate in the package.
Benchmarks use identical input fixtures, nine alternating-order rounds, 10,000
calls per round; counted operations use a separate build from timing. Inputs
include empty/one-human, 523 mixed entries, 3,000 map entries, nearby human and no
human on the bot's map. The human sorts last when mixed entries are present.
Changed erroneous results are labelled intentional correction. These numbers
are **not** a percentage improvement of the whole server.

| Scenario / priority call | V20 ns | V21 ns |
|---|---:|---:|
| empty | 20.95 | 16.62 |
| one_human_other_map | 75.08 | 38.55 |
| one_human_nearby_no_mixed | 10.06 | 17.26 |
| 523_mixed_no_human | 23813.10 | 14.65 |
| 523_mixed_one_human_other_map | 23777.30 | 37.65 |
| large_map_human_same_zone | 23932.80 | 40.14 |
| large_map_human_other_zone | 25845.70 | 42.18 |
| nearby_human | 1888.65 | 16.04 |
| nearby_only_human | 4321.75 | 30.55 |

Costs are retained too: with just one human on another map, scalar nearby/force
can be slightly slower than walking one legacy entry. Publishing a human position
adds a mutex-protected scalar write and a terrain zone lookup. The publisher
microbenchmark measures the write only, not the terrain lookup. Every relocation
also gains a cheap count test; with humans online, camera observers are notified.
No claim is made that these costs are zero.

## Manual runtime validation — user only

Work does not start mangosd (even --version), WoW, MySQL or any live server, does
not connect to them, and does not change the live D: installation or configuration.
The package contains Windows VS2022 x64 RelWithDebInfo mangosd.exe/PDB, matching
runtime DLL package, build-info.json and validation evidence; BUILD_PLAYERBOTS=1,
SUPPORTED_CLIENT_BUILD=5875. Preserve the accepted V20 executable as fallback.

Use the server console manually. In each phase record the time, run `rndbot presence`,
`rndbot cpu` and `rndbot diff` at the beginning and again after 5–10 minutes:

1. A: no human in world. `human_in_world=0`, `activity_has_players=0`.
2. B: one human enters. Both become 1. Check reaction, target switching, combat,
   heals, movement and activity near/far from the character; include an instance
   transfer, GM mode/farsight if used, and selfbot/group control if used.
3. C: logout to character selection, leave the client there. Both become 0 even
   if session count remains 1 and mixed_registry remains nonzero. Existing activity
   decisions may retain their normal cached value for up to five seconds.
4. D: fully disconnect. Both remain 0. Also test disconnect while in-world followed
   by reconnect if practical; the retained body must not count as a human.

Return Server.log, Perf.log, the four phase times and build-info.json. Keep the same
population/configuration and separate any PMO window. The `client_writes`,
`bot_registration_writes`, `move_writes`, `erased` fields are cumulative upsert/erase
operations since process startup, not category counts or unique online players.
Mixed registry persistence in C/D is expected; it must no longer drive human
activity presence. The broad V20 diagnostic collector is deliberately absent;
there is no 64-thread registration cap or dropped-thread accounting in V21.

Deferred: unused GetRandomPlayer insertion bug, unused HasManyPlayersNearby's
legacy range contract, social consumers' legacy names/definitions, async login
manager's existing raw-pointer snapshot lifetime, PMO concurrency and the 40-man
raid validation. V21 does not alter Mema, AddCooldown, UseItem, item-save errors,
raid logic, route/pathfinding, activity brackets, AI frequency/radii/population,
DisableBotOptimizations, botActiveAlone or any numeric activity target.

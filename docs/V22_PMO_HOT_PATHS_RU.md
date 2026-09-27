# V22 — PMO hot paths: отчёт и ручная проверка

Ветка: `vmangos-v22-pmo-hotpaths`. Родитель — accepted V21
`6a32bb9c97d45166719a9b5a9d3e0f0c0c8ac4c7`. Точный release HEAD,
SHA-256 EXE/PDB/DLL и параметры сборки записаны в `build-info.json` артефакта.
Этот файл описывает исходники; успех CI подтверждается отдельным завершённым
Windows workflow для этого HEAD, а не наличием текста отчёта.

Включены три изменения A/B/C. Исправление присутствия человека V21 и cumulative
V0–V21 сохранены. В20 `169cb7712c8b704a3fe274cbf42a0cbb53fcb90a` — fallback;
диагностическая ветка `4b9d62de1c5dbe1398f5067596fa9111f00e2fdd` не является
родителем. Настройки, population, частоты AI, scoring, RNG, PMO scopes,
CheckValues, Mema, cooldown/item/use-item код и partitions не изменены.

## Основание и что вошло

Проверен предоставленный Server1.log: V21, 2026-09-27, reset/enabled 01:55:47,
disabled 01:56:09, печать 01:56:13. До окна: 3000 ботов, 2214 active,
diff10 99 ms. Values 21.609 s / 4,687,122; Triggers 46.403 s / 55,820,544;
Actions 35.493 s / 1,299,822. Эти вложенные времена не суммируются.
PMO добавляет собственную нагрузку. Сырые пользовательские логи не публикуются.

**A.** Разбор `{questId,objective}` в NeedQuestObjective без временного
vector<string>, только для доказанного цифрового формата. Другие строки идут
через прежний parser; signed range/исключения std::stoi сохранены. Нет кеша
между вызовами. NeedForQuest, GetDestinations, GrindTarget и scoring не менялись.

**B.** В Vanilla без MEMORY_MONITOR FlyDirect получает маршрут по const
reference: эта функция по-прежнему сразу возвращает false. Бесполезная копия
пути исчезла; сам вызов и последующее pathfinding остаются. В MEMORY_MONITOR
и остальных client branches прежняя передача по значению сохранена. В isUseful
читаются условия TravelTarget через HasCondition без копии vector<string>.
Остальным consumers оставлен прежний GetConditions snapshot getter.

**C.** Event переносит строки из собственных параметров, уже переданных по
значению. Виртуальные getName, проверки/частоты triggers, packet/GUID/owner и
копирование Event между владельцами сохранены. Это помогает при создании
события; 55.8M trigger observations не означают столько же непустых событий.

Отклонено/отложено: повторное кеширование Value/target/destination, изменение
списка кандидатов, устранение оставшихся WorldPosition snapshots без доказанной
безопасности callback, общий parser/RTTI redesign, повторная переделка
CheckValues, перенос Teldrassil/Darnassus. Partition evidence сохранено в TODO.

## Измерения

Сначала сохранены отдельные baseline V21 до соответствующих production-правок.
Финальный замер — MSVC x64 RelWithDebInfo, 9 чередующихся раундов, одинаковая
работа, один выбранный logical CPU. Медианы ниже относятся к standalone
функциям с детерминированными world services, не к общему времени сервера.
Финальные benchmark и build не выполнялись одновременно.

| Участок, один вызов | V21, нс | V22, нс |
|---|---:|---:|
| Квест, 4 objectives, entry отсутствует, refresh | 1867.19 | 1123.59 |
| Квест, entry в первом destination | 735.59 | 546.84 |
| 20 квестов, entry отсутствует | 40713.56 | 24962.08 |
| Только отключённая проверка полёта, маршрут 512 точек | 893.64 | 1.42 |
| isUseful: 8 длинных условий, искомого нет | 311.64 | 66.89 |
| Event: source и param по 64 символа | 108.71 | 60.06 |
| Активный trigger, имя 64 символа | 61.15 | 36.66 |
| Неактивный trigger — контроль | 6.87 | 6.86 |

Отдельный подсчёт аллокаций/байтов за вызов:

| Сценарий | Аллокации V21 → V22 | Запрошенные байты V21 → V22 |
|---|---:|---:|
| Квест, 4 objectives, miss/refresh | 34 → 18 | 1508 → 740 |
| Проверка полёта, 512 точек | 1 → 0 | 20519 → 0 |
| isUseful, 8 длинных условий | 11 → 2 | 704 → 64 |
| Event, две строки по 64 символа | 4 → 2 | 320 → 160 |
| Активный trigger, имя 64 символа | 2 → 1 | 160 → 80 |
| Неактивный trigger | 0 → 0 | 0 → 0 |

Число relevant Value Get/Calculate, candidate iterations, destination/quest
checks и RNG одинаково в парных сценариях. В полном warm GrindTarget с уже
готовыми ответами NeedForQuest основная работа A не выполняется: убедительного
систематического ускорения этой контрольной цепочки не заявляется. При 512
обычных кандидатах остаётся 512 итераций; они не пропускаются ради скорости.
Empty/short/SSO cases и generic-parser fallback могут дать нулевой выигрыш
или небольшое ухудшение; все сырые раунды и отрицательные медианы сохранены
в `validation/v22-local-negative-samples.json` и соответствующих CSV.
Удаление копии в FlyDirect не означает ускорения всего MoveTo на величину
из этой таблицы: дорогое построение пути вообще не менялось.

## Проверки

- 19,360 quest observations: 0/1/2/4 objectives, несколько incomplete, item,
  creature/GO, completed/inactive/absent quests, несколько quests, destination
  first/last/miss, refresh/reset, PMO OFF/ON; точные traces без удаления событий.
- Более 100 тысяч parser input/position comparisons, включая границы signed
  int32, leading zero, signs, reward/nested/fallback, invalid_argument/out_of_range.
- 120 полных GrindTarget → NeedForQuest → objective observations: 0/1/8/64/512,
  duplicates, attacker-first/active target, no target, quest/non-quest,
  filters/group, одинаковые GUID/RNG/Value traces при PMO OFF/ON.
- 1,696 movement/Event/Trigger observations: пустые/короткие/длинные маршруты,
  неизменная/изменённая позиция назначения, условия first/last/miss/duplicates,
  group/follow/forced/loot, inactive/active/external events, SSO/long strings,
  copy/self-assignment/packet/GUID/owner, callback exceptions/reentry.
- Отдельные normal и AddressSanitizer прогоны новых и cumulative V17–V21
  suites. Дополнительный ASan MEMORY_MONITOR подтверждает прежние path-copy
  counts. V20 shared-registry и V21 lifecycle concurrency тесты сохранены.
- Cumulative historical guards адаптированы только к шести файлам с точными
  approved SHA-256; прежние assertions не удалены. Actual V22 bodies компилируют
  новые suites. Остальные production-файлы байт-в-байт равны V21.

Allocation-counter binaries не используются как источник timing. Изменение
числа аллокаций неизбежно меняет точки возможного bad_alloc; идентичность
«исключения на N-й аллокации» не заявляется. Старые malformed qualifiers с
небалансными скобками, потенциально некорректные в общем parser, не расширяют
scope V22. Реальный маршрут/бой/рейд не имитируется standalone fixtures.

## Сборка и граница выполнения

Windows-only VS2022 x64 RelWithDebInfo: BUILD_PLAYERBOTS=1,
SUPPORTED_CLIENT_BUILD=5875, BUILD_EXTRACTORS=0, BUILD_WARNINGS_AS_ERROR=0.
EXE/PDB, runtime DLLs, лицензия OpenSSL, build-info и validation лежат вместе.
Нужны установленные VC++ 2015–2022 x64 и VC++ 2008 x64 CRT, как у предыдущего
пакета. EXE/PDB проверяются статически по CodeView/PDB identity и SHA-256.

Work не запускал и не подключался к mangosd/WoW/HermesProxy/MySQL/DB,
не изменял рабочую установку D:. Местные сборки ограничены affinity четырьмя
logical CPUs и parallel=2, /MP1. Никакая runtime готовность не объявляется
только по компиляции и microbenchmarks.

## Ручной тест пользователя

1. Сохранить текущие EXE/PDB/DLL и конфигурацию V21. После самостоятельной
   остановки сервера установить комплект V22 из одного артефакта. БД,
   конфиги, population и AI настройки не менять. Сохранить build-info.
2. После прогрева при PMO OFF снять одинаковые по длительности окна
   `rndbot diff`, `rndbot cpu`, `rndbot presence`, примерно 5–10 минут каждое.
   Сравнивать с V21 при том же количестве ботов, active и тех же условиях.
3. Проверить обычный квестовый бой: несколько целей, убийство/смена цели,
   квестовые предметы, несколько незавершённых objectives, завершённый квест;
   отдельно пройти короткий и длинный travel маршрут, сменить destination,
   посмотреть отсутствие новой паузы или застревания. Отметить конкретные
   времена и ситуацию, если поведение отличается.
4. Повторить presence A/B/C/D: пусто → человек в мире → выбор персонажа →
   полное отключение. В C/D human_in_world и activity_has_players должны быть
   0. Непустой mixed registry сам по себе не означает присутствия человека.
5. Отдельно сделать короткое PMO окно 15–25 секунд: reset/enable, затем
   disable и print. Зафиксировать время. Не смешивать его и ближайшее время
   печати/восстановления со стабильными PMO OFF окнами.
6. Передать Server.log/Perf.log, build-info, времена фаз, число ботов/active,
   команды diff/cpu/presence и короткое описание ощущений от боя/движения.
   Новый PMO лог позволит проверить, уменьшилась ли стоимость нужных цепочек.

40-man raid остаётся отдельной неблокирующей проверкой. Reconnect/transfer,
GM/invisibility и selfbot не считаются автоматически проверенными прежним
A–D логом. При неожиданном поведении пользователь может вернуть сохранённый
V21 комплект; миграций БД в V22 нет.

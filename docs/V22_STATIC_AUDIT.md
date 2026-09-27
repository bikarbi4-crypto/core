# V22: аудит до изменения production-кода

Baseline: `6a32bb9c97d45166719a9b5a9d3e0f0c0c8ac4c7` (V21).
Evidence: предоставленный Server1.log; PMO включён примерно 01:55:47–01:56:09.
Вложенные PMO времена не складываются. Это выбор направлений, а не замер
нормальной производительности сервера.

## A — разбор qualifier квестовой цели

`AttackAnythingAction::isUseful -> GetTarget -> grind target ->
GrindTargetValue::Calculate/FindTargetForGrinding -> need for quest ->
NeedForQuestValue::Calculate -> need quest objective ->
NeedQuestObjectiveValue::Calculate`.

V16 уже хранит ответы need-for-quest по entry только внутри Calculate;
V17 сохраняет локальный список участников/счётчики целей; V18 уже ограничивает
GetDestinations одним вызовом на quest и строит числовой qualifier без stream.
Повторно убирать эти вызовы нельзя. Чтения Value в каждом assist pass и по
каждому objective нужны для исходного порядка refresh и возможных callbacks.

Осталось: два вызова getMultiQualifierInt создают временный vector<string> и
разбирают обе части одной короткой строки вида `{12345,0}`. Кандидат — отдельный
fast path только для `{1..10 ASCII digits,0..3}`, только в NeedQuestObjective.
Сохранить std::stoi для quest id, включая out_of_range. Любые другие строки,
знаки, reward, вложенность, неканонические/длинные входы — исходный parser.
Не сохранять разбор между Get: qualifier может быть изменён при reentry.
Число и место getQualifier/Value reads остаются прежними.

## B — движение: копии до неизменяемых проверок

`MoveToTravelTargetAction::isUseful -> TravelTarget::GetConditions` копирует
vector<string> только для сравнения строк с одним литералом. Внутри цикла нет
callbacks, Value reads, setters или RNG. Специализированный const HasCondition
читает тот же контейнер в том же порядке, не отдавая наружу ссылку. Старый
snapshot getter сохраняется для остальных consumers, включая CopyTarget и
передачу условий между ботами. Ownership остаётся у TravelTarget.

`MoveToTravelTargetAction::Execute -> MovementAction::MoveTo -> FlyDirect`
передаёт lastPath по значению. В MANGOSBOT_ZERO FlyDirect безусловно возвращает
false: копия всего vector<PathNodePoint> уничтожается без использования.
Кандидат — const-reference argument только для ZERO без MEMORY_MONITOR.
Для других clients и MEMORY_MONITOR оставить передачу по значению: копии
WorldPosition имеют Add/Rem hooks, которые нельзя молча удалить.
Все вызовы FlyDirect, route/pathfinding и выбор/копирование рабочего маршрута
после проверки остаются на своих местах. Ссылка не сохраняется, не передаётся
другому потоку и в данном варианте вообще не разыменовывается.

Не убирать прочие WorldPosition snapshots вслепую: CheckStatus, MoveTo и Value
callbacks способны изменить цель; MEMORY_MONITOR добавляет наблюдаемые hooks.

## C — Event construction в общей инфраструктуре

`Engine::ProcessTriggers -> Trigger::Check -> Event(getName(), ...)`.
getName виртуален: убирать/кешировать его нельзя. Event уже принимает source
и param по значению, затем снова копирует их в поля. Кандидат — std::move
только из этих собственных параметров. Копирование входных lvalues, вызовы
виртуальных getter, packet/GUID serialization, owner, copy ctor/assignment и
trigger scheduling остаются прежними. Event move ctor не добавляется.

Для коротких SSO strings и неактивных triggers выигрыш может отсутствовать;
эти результаты сохраняются. Десятки миллионов PMO observations не означают,
что все эти проверки создают непустой Event. PMO scopes и guard не меняются.

## Проверки и границы

Сначала отдельные baseline microbenchmarks V21, затем A/B/C с actual-source
extraction, детерминированными world services и отдельным подсчётом операций.
Тестировать result/order/duplicates, RNG и Value refresh traces, исключения и
reentry. Allocation failure в удалённой аллокации намеренно исчезает; обещать
идентичное количество bad_alloc после устранения аллокаций было бы неверно.
Проверять корректное unwinding оставшихся операций, обычные исключения и
отсутствие утечек, normal + ASan. Это не доказательство полного live gameplay;
ручной runtime test выполняет пользователь.

Никаких shared caches, новых потоков, пропуска триггеров, изменений AI policy,
частот, presence, PMO instrumentation, CheckValues, Mema или partition mapping.
Сервер, клиент и БД в ходе этой работы не запускаются.

// The fixed arena gives both variants the same relative pointer ordering.
struct State {
 struct alignas(std::max_align_t) Slot {unsigned char data[512];};
 std::array<Slot,640> arena;unsigned used=0;
 std::vector<Unit*> objects;std::map<uint64,Unit*> units;std::vector<std::unique_ptr<PlayerbotAI>> ais;
 Group group,otherGroup;Duel duel;Player* bot=nullptr;Player* peer=nullptr;Player* master=nullptr;AttackersValue* root=nullptr;
 time_t now=1000;uint64 rngState=12345;std::array<uint64,Counters::count> counts{};
 std::vector<std::string> trace;std::function<void(const char*,unsigned,const std::string*)> callback;bool hooked=false;
 std::string rootKey;unsigned sharedMode=0;
 template<class T>T* Make(unsigned id){
  static_assert(sizeof(T)<=sizeof(Slot));assert(used<arena.size());
  auto* p=new(arena[used++].data)T(id);objects.push_back(p);units[p->guid.value]=p;return p;
 }
 PlayerbotAI* AddAI(Player* p){
  ais.push_back(std::make_unique<PlayerbotAI>(p));p->ai=ais.back().get();return p->ai;
 }
 explicit State(const Case& c){
  state=this;sPlayerbotAIConfig=Config{};sPlayerbotAIConfig.perfMonEnabled=c.pmo;sPlayerbotAIConfig.sightDistance=c.range;sPlayerbotAIConfig.shareTargets=c.mode>=20 && c.mode<40;
  bot=Make<Player>(1);auto* owner=AddAI(bot);rootKey="attackers"+(c.qualifier.empty()?std::string():"::"+c.qualifier);
  for(unsigned i=0;i<c.members;++i){
   auto* member=Make<Player>(10+i);if(i%3!=2)AddAI(member);member->group=&group;group.members.push_back({member->guid});
   if(i==0)peer=member;
  }
  if(c.members){bot->group=&group;group.members.push_front({bot->guid});group.members.push_back({ObjectGuid(99999)});}
  master=Make<Player>(80);AddAI(master);master->group=&otherGroup;if(c.mode==5 || c.mode==6)owner->master=master;
  auto* pet=Make<Pet>(90);bot->pet=pet;
  auto* guardian=Make<Creature>(91);
  for(unsigned i=0;i<c.candidates;++i){
   Unit* unit=(i%11==7)?static_cast<Unit*>(Make<Player>(100+i)):static_cast<Unit*>(Make<Creature>(100+i));
   unit->victim=bot;unit->distance=2+(i%10);unit->targetGuid=bot->guid;
   if(c.mode==4 || c.mode==6){unit->safe=i%9!=0;unit->possibleValid=i%5!=0;if(peer && i%3==1)unit->victim=peer;}
   if(c.mode==6){unit->entry=i%7==0?190013:100;unit->level=i%4?10:30;sPlayerbotAIConfig.prohibited=i%2==0;}
   for(auto& a:ais){a->bot->possible.push_back(unit);if(i%7==0)a->bot->possible.push_back(unit);}
   if(i%5==0)bot->attackers.insert(unit);if(i%9==0)pet->attackers.insert(unit);
   if(i==0)unit->controlled={guardian,guardian};
  }
  bot->attackers.insert(nullptr);pet->attackers.insert(nullptr);
  Unit* first=c.candidates?units.at(100):nullptr;Unit* last=c.candidates?units.at(99+c.candidates):nullptr;
  for(auto& a:ais){
   a->context->Put<Unit*>("current target",first);a->context->Put<Unit*>("old target",first);
   a->context->Put<ObjectGuid>("attack target",last?last->guid:ObjectGuid());a->context->Put<Unit*>("pull target",last);
   a->context->Put<Unit*>("rti target",first);
   a->context->Put<bool>("travel target traveling",c.mode==6);a->context->Put<float>("distance::travel target",200);
   a->context->Put<bool>("trigger active::out of react range",c.mode==6);a->follow=c.mode==6;
  }
  std::list<ObjectGuid> nearbyGuids;
  if(peer){nearbyGuids={ObjectGuid(999999),bot->guid,master->guid,peer->guid,peer->guid};}
  owner->context->Put<std::list<ObjectGuid>>("nearest friendly players",nearbyGuids);
  root=dynamic_cast<AttackersValue*>(owner->context->GetUntypedValue(rootKey));assert(root);
  if(c.mode==1)owner->activity=false;
  if(c.mode==2)bot->teleport=true;
  if(c.mode==3){owner->focus=true;if(c.candidates && first)first->inWorld=false;}
  if(c.mode==7){bot->flying=true;bot->height=11;}
  if(c.mode==8)bot->confused=true;
  if(c.mode==9)bot->inWorld=false;
  if(c.mode==10){bot->bg=true;bot->bgType=BATTLEGROUND_AV;owner->quester=true;}
  if(c.mode==11){bot->bg=true;bot->bgType=BATTLEGROUND_AV;}
  if(c.mode==12){bot->bg=true;bot->bgType=2;}
  if(c.mode==13){duel.opponent=master;bot->m_duel=&duel;}
  if(c.mode>=20 && c.mode<40 && peer && peer->ai){
   auto* shared=dynamic_cast<AttackersValue*>(peer->ai->context->GetUntypedValue(rootKey));assert(shared);
   sPlayerbotAIConfig.shareTargets=false;
   try{shared->Get();}catch(const std::exception&){}
   std::list<ObjectGuid> values;
   if(first)values={first->guid,first->guid,ObjectGuid(999998),last->guid};
   shared->Set(values);sPlayerbotAIConfig.shareTargets=true;
   sharedMode=c.mode;
   if(c.mode==21)static_cast<UntypedValue*>(shared)->Reset();
   if(c.mode==22)bot->distance=50;
   if(c.mode==23)peer->safe=false;
   if(c.mode==24)peer->distance=12;
   if(c.mode==25)peer->ai->context->Put<std::list<ObjectGuid>>(rootKey,values);
   if(c.mode==26)peer->ai->context->values.erase(rootKey);
   if(c.mode==27)peer->map=2;
  }
  trace.clear();counts.fill(0);
 }
 ~State(){
  callback=nullptr;ais.clear();
  for(auto it=objects.rbegin();it!=objects.rend();++it)(*it)->~Unit();
  if(state==this)state=nullptr;
 }
 void ResetValues(bool all){
  for(auto& a:ais)for(auto& v:a->context->values)if(all || v.second.get()==root)v.second->Reset();
 }
 void ResetRoot(){static_cast<UntypedValue*>(root)->Reset();}
 void Hook(int mode){
  callback=[&,mode](const char* event,unsigned id,const std::string* key){
   if(hooked)return;
   const bool lookup=!std::strcmp(event,"lookup") && key && key->rfind("possible targets::",0)==0;
   if(lookup){hooked=true;
    if(mode==1)sPlayerbotAIConfig.sightDistance=17;
    if(mode==2)root->Qualify("1");
    if(mode==3)(void)root->Get();
    if(mode==4)throw std::runtime_error("fixture lookup callback");
   }
  };
 }
};
inline void Count(unsigned which){
#if defined(V23_TRACE) || defined(V23_ALLOCATIONS)
 if(state)++state->counts[which];
#endif
}
inline void Mark(const char* event,unsigned id,const std::string* key){
 if(!state)return;
#if defined(V23_TRACE) || defined(V23_ALLOCATIONS)
 if(!std::strcmp(event,"attackers.calculate") || !std::strcmp(event,"possible.calculate"))Count(Counters::calculations);
#endif
#ifdef V23_TRACE
 state->trace.push_back(std::string(event)+":"+std::to_string(id)+(key?":"+*key:""));
#endif
 if(state->callback)state->callback(event,id,key);
}
inline time_t AuditTime(time_t*){Count(Counters::clock);Mark("clock");return state->now;}
inline float Threat::getThreat(Unit* player){Mark("threat",owner->id);auto it=levels.find(player);return it==levels.end()?0:it->second;}
inline PlayerbotAI::PlayerbotAI(Player* bot):bot(bot),context(std::make_unique<Context>(this)){}
inline PlayerbotAI::~PlayerbotAI()=default;
inline Unit* PlayerbotAI::GetUnit(ObjectGuid guid){Count(Counters::objects);Mark("unit",unsigned(guid.value));auto it=state->units.find(guid.value);return it==state->units.end()?nullptr:it->second;}
inline uint32 PlayerbotAI::GetFixedBotNumber(BotTypeNumber type,int max,float fraction){assert(type==BotTypeNumber::DUMMY_ATTACK_NUMBER && max==10 && fraction==0.2f);Count(Counters::rng);Mark("rng",bot->id);state->rngState=state->rngState*1664525+1013904223;return uint32(state->rngState%max);}
inline Player* ObjectMgr::GetPlayer(ObjectGuid guid){Count(Counters::objects);Mark("player",unsigned(guid.value));auto it=state->units.find(guid.value);return it==state->units.end()?nullptr:dynamic_cast<Player*>(it->second);}
inline UntypedValue* Context::GetUntypedValue(const std::string& key){
 Count(Counters::lookups);Mark("lookup",owner->bot->id,&key);
 auto it=values.find(key);if(it!=values.end())return it->second.get();
 auto pos=key.find("::");auto name=key.substr(0,pos);auto qual=pos==std::string::npos?"":key.substr(pos+2);
 std::unique_ptr<UntypedValue> p;
 if(name=="attackers")p=std::make_unique<AttackersValue>(owner);
 else if(name=="possible targets")p=std::make_unique<PossibleTargetsValue>(owner);
 else throw std::runtime_error("unregistered fixture value "+key);
 if(auto* q=dynamic_cast<Qualified*>(p.get()))q->Qualify(qual);
 auto* ptr=p.get();values.emplace(key,std::move(p));return ptr;
}
inline void PossibleTargetsValue::FindPossibleTargets(Player* player,std::list<Unit*>& targets,float range){
 Mark("grid",player->id);for(auto* u:player->possible)if(u->distance<=range)targets.push_back(u);
}
inline bool PossibleTargetsValue::IsValid(Unit* target,Player* owner,bool ignoreLos){
 Count(Counters::validations);Mark("possible.valid",target?target->id:0);
 return target && target->possibleValid && target->inWorld && target->alive && target->map==owner->map;
}
#undef AI_VALUE
#undef AI_VALUE2
#undef PAI_VALUE
#undef PAI_VALUE2
#undef PHAS_AI_VALUE


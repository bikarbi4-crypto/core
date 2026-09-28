#include <algorithm>
#include <array>
#include <cassert>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <functional>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <new>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <windows.h>
#include "../../v22_hotpaths/allocations.h"
struct Case {unsigned candidates=0,members=0,mode=0;bool pmo=false;std::string qualifier;float range=60;bool cold=false;};
namespace v22 {
#include "world.inc"
#include "v22-policies.inc"
#include "context.inc"
#include "v22-declarations.inc"
#include "v22-bodies.inc"
#include "state.inc"
}
namespace v23 {
#include "world.inc"
#include "v23-policies.inc"
#include "context.inc"
#include "v23-declarations.inc"
#include "v23-bodies.inc"
#include "state.inc"
}
struct Observation {
 std::vector<uint64_t> guids;
 std::vector<std::string> trace,keys;
 std::array<uint64_t,10> counts{};
 uint64_t rng=0;float range=0;std::string qualifier;bool expired=false;int exception=0,error=0;
 bool operator==(const Observation& x)const{return guids==x.guids && trace==x.trace && keys==x.keys && counts==x.counts && rng==x.rng && range==x.range && qualifier==x.qualifier && expired==x.expired && exception==x.exception && error==x.error;}
};
template<class S> std::vector<Observation> Observe(const Case& c,int initial,int hook){
 auto state=std::make_unique<S>(c);if(hook)state->Hook(hook);
 std::vector<Observation> results;
 for(unsigned step=0;step<5;++step){
  if(step==2)++state->now;
  if(step==3)state->ResetValues(true);
  if(step==4){state->now+=3;state->bot->distance=3;}
  state->trace.clear();state->counts.fill(0);errno=initial;Observation o;
  try{auto value=state->root->Get();for(auto guid:value)o.guids.push_back(guid.value);}
  catch(const std::invalid_argument&){o.exception=1;}catch(const std::out_of_range&){o.exception=2;}catch(const std::runtime_error&){o.exception=3;}
  o.error=errno;o.expired=state->root->Expired();o.qualifier=state->root->getQualifier();
  for(auto& a:state->ais)for(auto& value:a->context->values)o.keys.push_back(std::to_string(a->bot->id)+":"+value.first);
  o.trace=state->trace;o.counts=state->counts;o.rng=state->rngState;
  // Range is present in the exact typed-lookup key; record the remaining config too.
  o.range=state->bot->distance;results.push_back(std::move(o));
 }return results;
}
static uint64_t checks=0;
static void Check(const Case& c,int initial=0,int hook=0){
 auto old=Observe<v22::State>(c,initial,hook),now=Observe<v23::State>(c,initial,hook);
 if(old!=now){
  std::cerr<<"Attackers parity mismatch n="<<c.candidates<<" members="<<c.members<<" mode="<<c.mode<<" pmo="<<c.pmo<<" qualifier="<<c.qualifier<<" range="<<c.range<<" hook="<<hook<<" errno="<<initial<<"\n";
  for(size_t i=0;i<old.size();++i)if(!(old[i]==now[i])){std::cerr<<"step="<<i<<" old/new GUIDs="<<old[i].guids.size()<<"/"<<now[i].guids.size()<<" trace="<<old[i].trace.size()<<"/"<<now[i].trace.size()<<" errno="<<old[i].error<<"/"<<now[i].error<<"\n";break;}
  std::exit(2);
 }checks+=old.size();
}
static std::vector<Case> BenchCases(){
 std::vector<Case> r;
 for(bool pmo:{false,true}){
  for(unsigned n:{0u,1u,8u,64u,512u})r.push_back({n,0,0,pmo,"0",60,false});
  for(unsigned members:{5u,40u})r.push_back({8,members,4,pmo,"0",60,false});
  for(unsigned mode:{1u,3u,6u,20u,21u,25u})r.push_back({8,5,mode,pmo,"0",60,false});
  r.push_back({64,5,4,pmo,"1",60,false});
  r.push_back({64,0,0,pmo,"0",60,true});
 }return r;
}
struct Measure {double ns=0,allocations=0,bytes=0;std::array<uint64_t,10> counts{};uint64_t result=0;};
static volatile uint64_t resultSink=0;
template<class S> Measure MeasureCase(const Case& c,unsigned n) {
 auto state=std::make_unique<S>(c);
 for(unsigned i=0;i<2;++i){state->ResetRoot();(void)state->root->Get();}
 state->counts.fill(0);
 auto begin=std::chrono::steady_clock::now();
#ifdef V23_ALLOCATIONS
 Allocations::reset();Allocations::active=true;
#endif
 uint64_t sum=0;
 for(unsigned i=0;i<n;++i){
  if(c.cold)state->ResetValues(true);else state->ResetRoot();
  auto result=state->root->Get();sum+=result.size();for(auto guid:result)sum+=guid.value;
 }
#ifdef V23_ALLOCATIONS
 Allocations::active=false;
#endif
 auto end=std::chrono::steady_clock::now();
 Measure m;m.ns=std::chrono::duration<double,std::nano>(end-begin).count()/n;
 m.allocations=double(Allocations::allocations)/n;m.bytes=double(Allocations::bytes)/n;m.counts=state->counts;m.result=sum;
 resultSink=sum;return m;
}
int main(int argc,char**argv){
 if(argc>1){
  SetThreadAffinityMask(GetCurrentThread(),DWORD_PTR(4));
  bool baseline=std::string(argv[1])=="--baseline";
#ifdef V23_ALLOCATIONS
  std::cout<<"candidates,members,mode,pmo,qualifier,cold,variant,allocations,bytes,lookups,gets,calculations,iterations,validations,rng,distances,objects,clock,pmo_starts,result\n";
#else
  std::cout<<"candidates,members,mode,pmo,qualifier,cold,round,variant,ns,result\n";
#endif
  for(const auto& c:BenchCases()){
   (void)MeasureCase<v22::State>(c,16);(void)MeasureCase<v23::State>(c,16);
#ifdef V23_ALLOCATIONS
   constexpr unsigned n=8;
   for(bool candidate:{false,true}){if(baseline && candidate)continue;auto m=candidate?MeasureCase<v23::State>(c,n):MeasureCase<v22::State>(c,n);
    std::cout<<c.candidates<<','<<c.members<<','<<c.mode<<','<<c.pmo<<','<<c.qualifier<<','<<c.cold<<','<<(candidate?"v23":"v22")<<','<<m.allocations<<','<<m.bytes;
    for(auto count:m.counts)std::cout<<','<<double(count)/n;std::cout<<','<<m.result/n<<'\n';}
#else
   const unsigned n=std::max(32u,10000/(1+c.candidates*(1+c.members)));
   for(unsigned round=0;round<9;++round)for(unsigned slot=0;slot<2;++slot){bool candidate=(round+slot)%2;if(baseline && candidate)continue;
    auto m=candidate?MeasureCase<v23::State>(c,n):MeasureCase<v22::State>(c,n);
    std::cout<<c.candidates<<','<<c.members<<','<<c.mode<<','<<c.pmo<<','<<c.qualifier<<','<<c.cold<<','<<round<<','<<(candidate?"v23":"v22")<<','<<std::fixed<<std::setprecision(3)<<m.ns<<','<<m.result/n<<'\n';}
#endif
  }return 0;
 }
 for(unsigned n:{0u,1u,8u,64u,512u})for(unsigned members:{0u,1u,5u,40u})for(unsigned mode:{0u,4u,5u,6u,10u,11u,12u,13u})
  for(bool pmo:{false,true})for(const auto& qualifier:{std::string(),std::string("0"),std::string("1")})Check({n,members,mode,pmo,qualifier});
 for(unsigned mode:{1u,2u,3u,7u,8u,9u,20u,21u,22u,23u,24u,25u,26u,27u})for(bool pmo:{false,true})
  for(unsigned n:{0u,1u,8u,64u})for(const auto& qualifier:{std::string(),std::string("0"),std::string("1")})Check({n,5,mode,pmo,qualifier});
 for(const auto& qualifier:{std::string("reward"),std::string("2147483648"),std::string("-1"),std::string(" 1"),std::string("+0")})
  for(bool pmo:{false,true})Check({8,5,4,pmo,qualifier});
 for(float range:{-2147483648.0f,-1.75f,0.0f,0.99f,17.5f,60.9f,2147483520.0f})
  for(int error:{0,EDOM,ERANGE})Check({8,5,4,false,"0",range},error);
 for(int hook:{1,2,3,4})for(bool pmo:{false,true})for(int error:{0,EDOM,ERANGE})Check({8,5,4,pmo,"0"},error,hook);
 std::cout<<"PASS "<<checks<<" complete Attackers observations; GUID/order/duplicates, sets, group/master/pet/guardians/duel, qualifiers, Value refresh, RNG, PMO, errno, exceptions/reentry\n";
}


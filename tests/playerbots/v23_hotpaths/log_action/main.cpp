#include <algorithm>
#include <array>
#include <cassert>
#include <cerrno>
#include <chrono>
#include <clocale>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <windows.h>
#include "../../v22_hotpaths/allocations.h"

namespace v22 {
#include "world.inc"
#include "v22.inc"
#undef sPlayerbotAIConfig
}
namespace v23 {
#include "world.inc"
#include "v23.inc"
#undef sPlayerbotAIConfig
}
struct Case {std::string name,history,message;int mode=0;bool pmo=false;};
static volatile uint64_t resultSink=0;
struct Observation {
 std::string history,file,log;
 std::vector<std::string> trace;
 size_t capacity=0;int error=0;bool thrown=false;uint64_t checksum=0;
 bool operator==(const Observation& x) const {return history==x.history && file==x.file && log==x.log && trace==x.trace && capacity==x.capacity && error==x.error && thrown==x.thrown && checksum==x.checksum;}
};
template<class W,class A,class E> Observation Observe(const Case& c,int initial,int callbackMode,W*& current) {
 W w;current=&w;A ai;E e(&ai);e.lastAction=c.history;e.testMode=c.mode==3;
 w.group=c.mode==2;w.logInGroupOnly=c.mode==1 || c.mode==2;w.perfMonEnabled=c.pmo;
 bool entered=false;
 w.callback=[&](const char* event) {
  if(entered)return;
  const bool selected=(callbackMode==1 && !std::strcmp(event,"bot")) || (callbackMode==2 && !std::strcmp(event,"log")) || (callbackMode==3 && !std::strcmp(event,"open"));
  if(selected){entered=true;e.LogAction("nested %s %d","event",7);}
  if(callbackMode==4 && !std::strcmp(event,"name"))throw std::runtime_error("fixture name callback");
  if(callbackMode==5 && !std::strcmp(event,"log"))throw std::runtime_error("fixture logger callback");
 };
 errno=initial;bool thrown=false;
 try {e.LogAction("%s",c.message.c_str());}catch(const std::runtime_error&){thrown=true;}
 const int error=errno;
 Observation o{e.lastAction,w.fileOutput,w.logOutput,w.trace,e.lastAction.capacity(),error,thrown,w.sink};
 current=nullptr;return o;
}
static Observation Old(const Case& c,int initial=0,int cb=0){return Observe<v22::World,v22::PlayerbotAI,v22::Engine>(c,initial,cb,v22::world);}
static Observation New(const Case& c,int initial=0,int cb=0){return Observe<v23::World,v23::PlayerbotAI,v23::Engine>(c,initial,cb,v23::world);}
static void Check(const Case& c,int initial,int cb,uint64_t& count) {
 auto a=Old(c,initial,cb),b=New(c,initial,cb);
 if(!(a==b)){std::cerr<<"LogAction parity mismatch "<<c.name<<" mode="<<c.mode<<" pmo="<<c.pmo<<" errno="<<initial<<" callback="<<cb<<" capacity="<<a.capacity<<"/"<<b.capacity<<"\n";std::exit(2);}++count;
}
static std::vector<Case> BenchCases() {
 std::vector<Case> r;
 for(int mode:{0,1,2,3}) for(bool pmo:{false,true}) {
  auto add=[&](const char* name,std::string h,std::string m){r.push_back({name,std::move(h),std::move(m),mode,pmo});};
  add("short",std::string(12,'a'),"A:attack - OK");
  add("boundary-512",std::string(499,'a'),"A:attack - OK");
  add("trim-no-pipe",std::string(500,'a'),std::string(40,'m'));
  add("trim-at-512",std::string(512,'a')+"|previous|",std::string(64,'m'));
  add("trim-last-pipe",std::string(900,'a')+"|",std::string(64,'m'));
  add("long-history",std::string(4096,'a')+"|tail",std::string(64,'m'));
  add("long-message",std::string(32,'a'),std::string(1000,'m'));
 }
 return r;
}
template<class W,class A,class E> uint64_t Batch(const Case& c,unsigned n,W*& current,bool timed,double& ns,uint64_t& allocations,uint64_t& bytes,size_t& capacity) {
 W w;current=&w;A ai;std::vector<E> engines;engines.reserve(n);
 for(unsigned i=0;i<n;++i){engines.emplace_back(&ai);engines.back().lastAction=c.history;engines.back().testMode=c.mode==3;}
 w.group=c.mode==2;w.logInGroupOnly=c.mode==1 || c.mode==2;w.perfMonEnabled=c.pmo;
 auto begin=std::chrono::steady_clock::now();
#ifdef V23_ALLOCATIONS
 Allocations::reset();Allocations::active=true;
#endif
 for(auto& e:engines)e.LogAction("%s",c.message.c_str());
#ifdef V23_ALLOCATIONS
 Allocations::active=false;allocations=Allocations::allocations;bytes=Allocations::bytes;
#endif
 auto end=std::chrono::steady_clock::now();
 ns=std::chrono::duration<double,std::nano>(end-begin).count()/n;
 uint64_t sum=w.sink;for(auto& e:engines){sum+=e.lastAction.size();if(!e.lastAction.empty())sum+=e.lastAction.back();}
 capacity=engines.back().lastAction.capacity();resultSink=sum;current=nullptr;return sum;
}
static void RunBatch(const Case& c,bool candidate,unsigned n,double& ns,uint64_t& allocations,uint64_t& bytes,size_t& capacity) {
 if(candidate)Batch<v23::World,v23::PlayerbotAI,v23::Engine>(c,n,v23::world,true,ns,allocations,bytes,capacity);
 else Batch<v22::World,v22::PlayerbotAI,v22::Engine>(c,n,v22::world,true,ns,allocations,bytes,capacity);
}
int main(int argc,char**argv) {
 if(argc>1){
  SetThreadAffinityMask(GetCurrentThread(),DWORD_PTR(4));
  const bool baseline=std::string(argv[1])=="--baseline";
#ifdef V23_ALLOCATIONS
  std::cout<<"case,mode,pmo,variant,allocations,bytes,capacity\n";
#else
  std::cout<<"case,mode,pmo,round,variant,ns,capacity\n";
#endif
  for(const auto& c:BenchCases()){
   double ns=0;uint64_t alloc=0,bytes=0;size_t capacity=0;
   for(bool candidate:{false,true})RunBatch(c,candidate,128,ns,alloc,bytes,capacity);
#ifdef V23_ALLOCATIONS
   for(bool candidate:{false,true}){if(baseline && candidate)continue;RunBatch(c,candidate,128,ns,alloc,bytes,capacity);std::cout<<c.name<<','<<c.mode<<','<<c.pmo<<','<<(candidate?"v23":"v22")<<','<<double(alloc)/128<<','<<double(bytes)/128<<','<<capacity<<'\n';}
#else
   for(unsigned round=0;round<9;++round)for(unsigned slot=0;slot<2;++slot){bool candidate=(round+slot)%2;if(baseline && candidate)continue;
    RunBatch(c,candidate,4096,ns,alloc,bytes,capacity);
    std::cout<<c.name<<','<<c.mode<<','<<c.pmo<<','<<round<<','<<(candidate?"v23":"v22")<<','<<std::fixed<<std::setprecision(3)<<ns<<','<<capacity<<'\n';
   }
#endif
  }return 0;
 }
 uint64_t checks=0;
 for(unsigned len:{0u,1u,15u,16u,127u,499u,511u,512u,513u,700u,1024u,4096u})
  for(int delimiter:{-1,0,511,512,513,699}) for(unsigned message:{0u,1u,13u,64u,511u,1000u})
   for(int mode:{0,1,2,3})for(bool pmo:{false,true}) {
    Case c{"boundaries",std::string(len,'x'),std::string(message,'m'),mode,pmo};
    if(delimiter>=0 && unsigned(delimiter)<len)c.history[delimiter]='|';
    for(int error:{0,EDOM,ERANGE})Check(c,error,0,checks);
   }
 for(int mode:{0,1,2,3})for(bool pmo:{false,true})for(int callback:{1,2,3,4,5}) {
  Case c{"callbacks",std::string(520,'a')+"|tail","A:use - OK",mode,pmo};
  for(int error:{0,EDOM,ERANGE})Check(c,error,callback,checks);
 }
 for(int mode:{0,1,2,3}){
  Case c{"bytes",std::string(510,'a')+std::string("\0|x",3),std::string("x\0tail",6),mode,true};
  Check(c,EDOM,0,checks);
 }
 // Repeated messages exercise trim cadence and retained capacity over time.
 for(int mode:{0,1,2,3})for(bool pmo:{false,true}){
  v22::World a;v23::World b;v22::world=&a;v23::world=&b;v22::PlayerbotAI aa;v23::PlayerbotAI ba;v22::Engine ae(&aa);v23::Engine be(&ba);
  ae.testMode=be.testMode=mode==3;a.group=b.group=mode==2;a.logInGroupOnly=b.logInGroupOnly=mode==1 || mode==2;a.perfMonEnabled=b.perfMonEnabled=pmo;
  for(unsigned i=0;i<2048;++i){
   ae.LogAction("A:%s - %u %.2f","repeat",i%7,double(i%11)/3);be.LogAction("A:%s - %u %.2f","repeat",i%7,double(i%11)/3);
   assert(ae.lastAction==be.lastAction && ae.lastAction.capacity()==be.lastAction.capacity());
   assert(a.fileOutput==b.fileOutput && a.logOutput==b.logOutput && a.trace==b.trace);++checks;
  }
 }
 std::cout<<"PASS "<<checks<<" complete LogAction observations; exact history/log bytes, capacity, modes, PMO, errno, exceptions/reentry, repetition\n";
}


#include "api.h"
#include "../allocations.h"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>
#define NOMINMAX
#include <windows.h>
static volatile std::uint64_t sink=0;
static const char* kinds[]={"fly-probe","travel-useful","event","trigger"};
static std::vector<Case> cases(){std::vector<Case> ret;
 for(unsigned kind=0;kind<4;++kind)for(unsigned size:{0u,1u,8u,64u,512u})for(unsigned mode=0;mode<4;++mode)ret.push_back({kind,size,mode});return ret;
}
static double time(Api api,void* state,unsigned n){auto start=std::chrono::steady_clock::now();sink+=api.run(state,n);return std::chrono::duration<double,std::nano>(std::chrono::steady_clock::now()-start).count()/n;}
int main(int argc,char** argv){
 std::string mode=argc>1?argv[1]:"test";std::vector<Api> apis{v21_api(),v22_api()};
 if(mode=="test"){
  unsigned n=0;auto scenarios=cases();for(unsigned mode=0;mode<256;++mode)for(unsigned size:{0u,1u,8u})scenarios.push_back({1,size,mode});
  for(auto c:scenarios){Observation base;for(auto api:apis){auto s=api.create(c);auto o=api.observe(s);api.destroy(s);if(api.name==apis[0].name)base=o;else{assert(base.result==o.result&&base.trace==o.trace&&base.gets==o.gets&&base.checks==o.checks);assert(o.copies<=base.copies);}++n;}}
  for(auto api:apis)api.contracts();
#ifdef MEMORY_MONITOR
  for(unsigned size:{0u,1u,8u,64u,512u}){auto a=apis[0].create({0,size,0}),b=apis[1].create({0,size,0});auto x=apis[0].observe(a),y=apis[1].observe(b);assert(x.copies==size&&x.copies==y.copies);apis[0].destroy(a);apis[1].destroy(b);}
  std::cout<<"MEMORY_MONITOR copy hooks preserved\n";
#endif
  std::cout<<"PASS "<<n<<" travel/event/trigger observations; exact service trace, result, input routes; copy/assignment/packet/GUID/exception/reentry contracts\n";return 0;
 }
 if(mode=="--operations"){
  std::cout<<"case,size,mode,variant,allocations,bytes,position_copies,value_gets,trigger_checks,result\n";
  for(auto c:cases())for(auto api:apis){auto s=api.create(c);api.run(s,1);Allocations::reset();Allocations::active=true;auto result=api.run(s,1);Allocations::active=false;auto o=api.observe(s);
   std::cout<<kinds[c.kind]<<','<<c.size<<','<<c.mode<<','<<api.name<<','<<Allocations::allocations<<','<<Allocations::bytes<<','<<o.copies<<','<<o.gets<<','<<o.checks<<','<<result<<'\n';api.destroy(s);}
  return 0;
 }
 assert(SetThreadAffinityMask(GetCurrentThread(),4));std::cout<<"case,size,mode,variant,round,iterations,ns_per_call\n";
 for(auto c:cases()){
  auto a=apis[0].create(c),b=apis[1].create(c);void* states[]={a,b};
  unsigned iterations=static_cast<unsigned>(std::clamp(3000000.0/std::max(time(apis[0],a,128),1.0),100.0,1000000.0));
  for(unsigned round=0;round<9;++round)for(unsigned j=0;j<2;++j){unsigned v=(round+j)%2;double ns=time(apis[v],states[v],iterations);
   std::cout<<kinds[c.kind]<<','<<c.size<<','<<c.mode<<','<<apis[v].name<<','<<round<<','<<iterations<<','<<std::fixed<<std::setprecision(3)<<ns<<'\n';}
  apis[0].destroy(a);apis[1].destroy(b);
 }
}

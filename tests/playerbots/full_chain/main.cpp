#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "api.h"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <vector>
static volatile std::uint64_t sink=0;
static const char* kinds[]={"bool","uint8","uint32","float","Unit_ptr","ObjectGuid_list"};
static const char* policies[]={"calculated","manual","single","memory","logged"};
static const char* modes[]={"warm","refresh","qualified","missing","wrong_type"};
static const char* operations[]={"get","size","empty"};
static const char* stages[]={"full","lookup","cast"};

double Measure(const Variant& variant,void* state,std::uint64_t iterations) {
    auto start=std::chrono::steady_clock::now();
    auto result=variant.run(state,iterations);
    auto finish=std::chrono::steady_clock::now();
    sink=result;
    return std::chrono::duration<double,std::nano>(finish-start).count()/iterations;
}
int main(int argc,char** argv) {
    // A single logical CPU, within the launcher's four-CPU mask. No priority boost.
    DWORD_PTR allowed=0,system=0;
    if(!GetProcessAffinityMask(GetCurrentProcess(),&allowed,&system)) return 2;
    const DWORD_PTR chosen=(allowed&4)?4:(allowed&(~allowed+1));
    if(!SetThreadAffinityMask(GetCurrentThread(),chosen)) return 2;
    bool quick=argc>1 && std::string(argv[1])=="--quick";
    bool checkOnly=argc>1 && std::string(argv[1])=="--check";
    std::vector<Variant> variants={v16_api(),c_only_api(),ab_only_api(),v17_api()};
#ifdef AUDIT_CANDIDATE
    variants.push_back(candidate_api());
#endif
    std::vector<Case> cases;
    for(Kind kind:{Kind::boolean,Kind::u8,Kind::u32,Kind::floating,Kind::unit,Kind::list})
        for(Policy policy:{Policy::calculated,Policy::manual,Policy::single,Policy::memory,Policy::logged}) {
            if((policy==Policy::memory || policy==Policy::logged) && kind!=Kind::u32 && kind!=Kind::floating) continue;
            if(quick && policy!=Policy::calculated && policy!=Policy::manual) continue;
            for(Mode mode:{Mode::warm,Mode::refresh,Mode::qualified,Mode::missing,Mode::wrong_type}) {
                if(quick && mode!=Mode::warm) continue;
                if(policy==Policy::manual && mode==Mode::refresh) continue;
                for(Operation op:{Operation::get,Operation::size,Operation::empty}) {
                    if(kind!=Kind::list && op!=Operation::get) continue;
                    if((mode==Mode::missing || mode==Mode::wrong_type) && op!=Operation::get) continue;
                    Case c; c.kind=kind; c.policy=policy; c.mode=mode; c.operation=op;
                    cases.push_back(c);
                    if(mode==Mode::warm && op==Operation::get && (policy==Policy::calculated || policy==Policy::manual)) {
                        c.stage=Stage::lookup; cases.push_back(c);
                        c.stage=Stage::cast; cases.push_back(c);
                    }
                    if(!quick && kind==Kind::list && mode==Mode::warm) {
                        c.stage=Stage::full;
                        c.listSize=0; cases.push_back(c);
                        c.listSize=64; cases.push_back(c);
                    }
                }
            }
        }
    std::cout<<"case,type,policy,mode,operation,stage,list_size,pass,variant,iterations,ns_per_call,checksum\n";
    for(std::size_t id=0;id<cases.size();++id) {
        const auto& c=cases[id];
        std::vector<void*> states;
        for(auto& v:variants) states.push_back(v.create(c));
        const double estimate=checkOnly?1.0:Measure(variants[0],states[0],10000);
        const auto iterations=checkOnly?3:static_cast<std::uint64_t>(std::clamp(12000000.0/std::max(estimate,1.0),1000.0,1000000.0));
        std::uint64_t perCall=7;
        if(c.kind==Kind::boolean || c.kind==Kind::unit) perCall=1;
        if(c.kind==Kind::list) perCall=c.operation==Operation::empty?(c.listSize==0):c.listSize;
        if(c.stage!=Stage::full) perCall=1;
        if(c.mode==Mode::missing || c.mode==Mode::wrong_type) perCall=0;
        for(unsigned pass=0;pass<(checkOnly?1u:9u);++pass) {
            const std::uint64_t expected=perCall*3;
            for(std::size_t j=0;j<variants.size();++j) {
                // Rotate the first variant and alternate direction to reduce order bias.
                auto idx=pass%2 ? (pass+variants.size()-j)%variants.size() : (pass+j)%variants.size();
                auto& v=variants[idx];
                if(v.run(states[idx],3)!=expected) { std::cerr<<"Semantic mismatch: "<<id<<' '<<v.name<<'\n'; return 3; }
                double ns=Measure(v,states[idx],iterations);
                if(pass || checkOnly) std::cout<<id<<','<<kinds[int(c.kind)]<<','<<policies[int(c.policy)]<<','<<modes[int(c.mode)]<<','
                    <<operations[int(c.operation)]<<','<<stages[int(c.stage)]<<','<<c.listSize<<','<<pass<<','<<v.name<<','
                    <<iterations<<','<<std::fixed<<std::setprecision(3)<<ns<<','<<sink<<'\n';
            }
        }
        for(std::size_t i=0;i<variants.size();++i) variants[i].destroy(states[i]);
        if(id%10==0) std::cerr<<"completed "<<id+1<<'/'<<cases.size()<<" cases\n";
    }
}

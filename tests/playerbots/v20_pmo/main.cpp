#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

using uint32 = uint32_t;
constexpr int LOG_BASIC=0, LOG_LVL_MINIMAL=0;
struct Row
{
    double percentage, time, average, count;
    uint32 min, max;
    std::string metric, name;
};
struct Log
{
    std::vector<Row> rows;
    std::vector<std::string> lines;
    template<class... T> void Out(int, int, const char* format, T... args)
    {
        int size=std::snprintf(nullptr, 0, format, args...);
        assert(size>=0);
        std::vector<char> buffer(size+1);
        std::snprintf(buffer.data(), buffer.size(), format, args...);
        lines.emplace_back(buffer.data());
        if constexpr(sizeof...(args)==8)
        {
            auto values=std::make_tuple(args...);
            rows.push_back({double(std::get<0>(values)), double(std::get<1>(values)),
                double(std::get<4>(values)), double(std::get<5>(values)),
                uint32(std::get<2>(values)), uint32(std::get<3>(values)),
                std::get<6>(values), std::get<7>(values)});
        }
    }
};
Log sLog;
#include "v19.inc"
#include "v20.inc"

template<class Monitor> auto Snapshot(Monitor& monitor)
{
    using Key=std::tuple<uint32,uint32,int,std::vector<std::string>>;
    std::map<Key, std::tuple<uint32,uint32,uint32,uint32>> result;
    for(auto& [map, instances]:monitor.mapsData)
        for(auto& [instance, metrics]:instances)
            for(auto& [metric, names]:metrics)
                for(auto& [stack, data]:names)
                    result[{map,instance,int(metric),stack}]={data.minTime,data.maxTime,data.totalTime,data.count};
    return result;
}
template<class Monitor, class Metric>
void Put(Monitor& monitor, uint32 map, uint32 instance, Metric metric,
         std::vector<std::string> key, uint32 min, uint32 max, uint32 time, uint32 count)
{
    auto& data=monitor.mapsData[map][instance][metric][key];
    data.minTime=min; data.maxTime=max; data.totalTime=time; data.count=count;
}
template<class Monitor, class Metric>
void Seed(Monitor& monitor, Metric value, Metric total, int mode)
{
    if(mode==0) return; // Completely empty registry.
    Put(monitor,0,0,value,{"All zero"},0,0,0,0);
    if(mode==1) return; // Existing entries immediately after Reset.
    Put(monitor,1,0,total,{"PlayerbotAI::UpdateAI 0"},10,700,5000,50);
    Put(monitor,1,0,total,{"PlayerbotAIBase::FullTick"},40,900,10000,100);
    Put(monitor,0,0,value,{"Outer","value"},2,90,300,7);
    Put(monitor,1,17,value,{"Outer","value"},5,120,700,11);
    // Zero elapsed partitions must not contaminate min/max, but counts remain.
    Put(monitor,33,999,value,{"Outer","value"},1,999,0,0);
    Put(monitor,33,1000,value,{"Outer","value"},0,0,0,3);
    Put(monitor,0,0,value,{"Count only"},0,0,0,9);
    // Explicitly characterize a malformed/inconsistent data source separately.
    if(mode==3) Put(monitor,0,0,value,{"Time without count"},1,250,500,0);
}
bool SameDouble(double a,double b) { return a==b || (std::isnan(a)&&std::isnan(b)); }
void SameExceptMax(const Row& a,const Row& b)
{
    assert(SameDouble(a.percentage,b.percentage));
    assert(SameDouble(a.time,b.time));
    assert(SameDouble(a.average,b.average));
    assert(SameDouble(a.count,b.count));
    assert(a.min==b.min && a.metric==b.metric && a.name==b.name);
}
std::string MaskMax(std::string text)
{
    auto begin=text.find(" .. ");
    if(begin==std::string::npos) return text;
    auto end=text.find(" (",begin+4);
    assert(end!=std::string::npos);
    return text.substr(0,begin+4)+"<max>"+text.substr(end);
}
int main()
{
    int calls=0, checkedRows=0, fixedRows=0, zeroReports=0;
    for(int mode=0;mode<4;++mode)
        for(bool perTick:{false,true})
            for(bool fullStack:{false,true})
                for(bool showMap:{false,true})
                {
                    v19::PerformanceMonitor old;
                    v20::PerformanceMonitor current;
                    Seed(old,v19::PERF_MON_VALUE,v19::PERF_MON_TOTAL,mode);
                    Seed(current,v20::PERF_MON_VALUE,v20::PERF_MON_TOTAL,mode);
                    auto before=Snapshot(current);
                    assert(before==Snapshot(old));
                    sLog={}; old.PrintStats(perTick,fullStack,showMap); auto baseline=sLog;
                    sLog={}; current.PrintStats(perTick,fullStack,showMap); auto candidate=sLog;
                    calls+=2;
                    assert(Snapshot(old)==before && Snapshot(current)==before);
                    assert(baseline.rows.size()==candidate.rows.size());
                    assert(baseline.lines.size()==candidate.lines.size());
                    for(size_t i=0;i<baseline.lines.size();++i)
                        assert(MaskMax(baseline.lines[i])==MaskMax(candidate.lines[i]));
                    for(size_t i=0;i<baseline.rows.size();++i)
                    {
                        const auto& a=baseline.rows[i]; const auto& b=candidate.rows[i];
                        SameExceptMax(a,b); ++checkedRows;
                        fixedRows+=(a.max!=b.max);
                        if(b.metric=="V" && b.name=="value [Outer]")
                        {
                            assert(!showMap && mode>=2);
                            assert(a.max==5 && b.max==120 && b.min==2);
                            assert(SameDouble(b.count,perTick ? double(float(21)/100) : 21));
                        }
                        if(b.metric=="V" && b.name=="value [Outer 0]")
                            assert(a.max==2 && b.max==90 && b.min==2);
                        if(b.metric=="V" && b.name=="value [Outer 1 (17)]")
                            assert(a.max==5 && b.max==120 && b.min==5);
                        if(mode==1 && b.name=="TOTAL")
                        {
                            assert(b.min==0 && b.max==0 && std::isnan(b.average));
                            ++zeroReports;
                        }
                        if(b.name=="Time without count" && !showMap)
                            assert(std::isinf(b.average) && b.max==250);
                    }
                    old.Reset(); current.Reset();
                    assert(Snapshot(old)==Snapshot(current));
                    for(auto& [key,counters]:Snapshot(current))
                        assert((counters==std::make_tuple(0u,0u,0u,0u)));
                    sLog={}; old.PrintStats(perTick,fullStack,showMap); baseline=sLog;
                    sLog={}; current.PrintStats(perTick,fullStack,showMap); candidate=sLog;
                    calls+=2;
                    assert(baseline.lines==candidate.lines);
                }
    assert(fixedRows>0 && zeroReports>0);
    std::cout<<"PrintStats production-body parity: "<<calls<<" calls, "<<checkedRows
             <<" rows compared; "<<fixedRows<<" corrected max fields.\n"
             <<"Baseline reproducer: aggregated min=2, max=5; corrected max=120.\n"
             <<"Count-only/zero partitions, every tick/stack/map option, Reset and source snapshots passed.\n"
             <<"Preserved limitation: zero denominators render NaN/Inf in existing report logic.\n";
}

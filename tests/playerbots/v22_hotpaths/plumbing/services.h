#pragma once
#include "api.h"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <ctime>
#include <cstring>
#include <functional>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <utility>
using uint8=std::uint8_t;using uint16=std::uint16_t;using uint32=std::uint32_t;
class Player{};class PlayerbotAI{};class Unit{};
struct ObjectGuid { std::uint64_t raw=0; bool operator==(ObjectGuid b) const{return raw==b.raw;} };
// Byte storage fixture, explicitly not network IO. Production Event constructors,
// copy/assignment, getObject and WorldPacket header are extracted unchanged.
class ByteBuffer {
protected: std::vector<uint8> _storage;
    unsigned pos=0;
public:
    explicit ByteBuffer(size_t reserve=4096){_storage.reserve(reserve);}
    ByteBuffer(const ByteBuffer& other):_storage(other._storage),pos(other.pos){}
    ByteBuffer(ByteBuffer&&)=default;ByteBuffer& operator=(ByteBuffer&&)=default;
    ByteBuffer& operator=(const ByteBuffer&)=default;
    void clear(){_storage.clear();pos=0;}
    size_t size()const{return _storage.size();}bool empty()const{return _storage.empty();}
    const uint8* contents()const{return _storage.data();}
    void append(const uint8* p,size_t n){_storage.insert(_storage.end(),p,p+n);}
    void rpos(unsigned n){pos=n;}
    ByteBuffer& operator<<(ObjectGuid g){auto p=reinterpret_cast<const uint8*>(&g.raw);append(p,8);return *this;}
    ByteBuffer& operator>>(ObjectGuid& g){if(size()<pos+8)throw std::out_of_range("packet");std::memcpy(&g.raw,contents()+pos,8);pos+=8;return *this;}
};
inline bool record=false;
inline Observation observation;
inline time_t now=10000;
inline int clockRand(){return 3;}
inline time_t clockTime(time_t*){return now;}
#ifdef PLUMBING_TRACE
inline void Trace(const std::string& s){
 if(record)observation.trace.push_back(s);
}
#else
#define Trace(...) ((void)0)
#endif
inline void PositionCopy(){
#ifdef PLUMBING_TRACE
 if(record)++observation.copies;
#endif
}

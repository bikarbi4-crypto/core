// Included inside each generated variant namespace, deliberately no pragma once.
class TravelMgr { public: PurposeDestinationMap destinationMap; DestinationList GetDestinations(const PlayerTravelInfo&,uint32,const std::vector<int32>&,bool,float) const; };
extern TravelMgr sTravelMgr;
template<class T> class MapFixtureValue : public SingleCalculatedValue<T*> {
    T* fixture;
public:
    MapFixtureValue(PlayerbotAI* ai,T* f) : SingleCalculatedValue<T*>(ai,"fixture-map"),fixture(f) {}
    T* Calculate() override { return fixture; }
};
class SharedValueContext : public NamedObjectContext<UntypedValue> {
public:
    // Owned by each disposable fixture; production's shared registration is
    // process-global. The value classes and retrieval path are unchanged.
    SharedValueContext() : NamedObjectContext(false) {
        creators["item drop list"]=[](PlayerbotAI* ai) { return new ItemDropListValue(ai); };
        creators["item vendor list"]=[](PlayerbotAI* ai) { return new ItemVendorListValue(ai); };
        creators["drop map"]=[](PlayerbotAI* ai) { return new MapFixtureValue<DropMap>(ai,&fixtureDrops); };
        creators["vendor map"]=[](PlayerbotAI* ai) { return new MapFixtureValue<VendorMap>(ai,&fixtureVendors); };
    }
};

struct Scene {char const* name;unsigned humans,mixed,mapBots;};
inline constexpr Scene scenes[]={
 {"empty",0,0,0},{"one_human_other_map",1,0,0},{"one_human_nearby_no_mixed",4,0,0},
 {"523_mixed_no_human",0,523,3000},{"523_mixed_one_human_other_map",1,523,3000},
 {"large_map_human_same_zone",3,523,3000},{"large_map_human_other_zone",2,523,3000},
 {"nearby_human",4,523,3000},{"nearby_only_human",5,523,3000}};
inline void Row(std::ostream& out,char const* scene,char const* variant,char const* operation,Work w)
{out<<scene<<','<<variant<<','<<operation<<','<<w.copies<<','<<w.copiedEntries<<','<<w.registryEntries<<','<<w.mapScans<<','<<w.mapEntries<<','<<w.friendChecks<<','<<w.presenceEntries<<'\n';}

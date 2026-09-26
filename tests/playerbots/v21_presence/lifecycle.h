namespace lifecycle {
inline PlayerActivityPresence presence;
constexpr int PLAYER_FLAGS=0,PLAYER_FLAGS_GM=1;
struct Socket {bool closing=false;bool IsClosing()const{return closing;}};
struct Session {Socket* socket=nullptr;bool logout=false;Socket* GetSocket(){return socket;}bool PlayerLogout(){return logout;}};
struct WorldObject
{
 float x=0,y=0,z=0;
 float GetPositionX()const{return x;}float GetPositionY()const{return y;}float GetPositionZ()const{return z;}
};
struct Camera {WorldObject* body=nullptr;WorldObject* GetBody(){return body;}};
struct Player:WorldObject
{
 unsigned id=1,map=0,instance=5,zone=1519;bool inWorld=false,gm=false,gmFlag=false;
 Session* session=nullptr;Camera camera;
 std::atomic<PlayerActivityPresence::Token> m_activityPresenceToken{0};
 Player(){camera.body=this;}~Player(){StopActivityPresence();}
 bool IsInWorld()const{return inWorld;}Session* GetSession(){return session;}
 unsigned GetGUIDLow()const{return id;}unsigned GetMapId()const{return map;}
 unsigned GetInstanceId()const{return instance;}unsigned GetZoneId()const{return zone;}
 bool IsGameMaster()const{return gm;}bool HasFlag(int,int)const{return gmFlag;}
 Camera& GetCamera(){return camera;}
 void StartActivityPresence();void StopActivityPresence();void UpdateActivityPosition();
 void UpdateActivityCamera(WorldObject const*);void UpdateActivityGmState();
};
#define sPlayerActivityPresence presence
#include "hooks.inc"
#undef sPlayerActivityPresence
inline void Test()
{
 Socket socket;Session client{&socket,false},botSession;
 // Categories: human, random, free, alt, controlled, legacy bot, human selfbot.
 for(unsigned category=0;category<7;++category)
 {
  Player p;p.session=(category==0||category==6)?&client:&botSession;
  p.StartActivityPresence();assert(!presence.HasPlayers()); // character screen
  p.inWorld=true;p.StartActivityPresence();
  bool const human=category==0||category==6;assert(presence.HasPlayers()==human);
  if(!human)continue;
  assert(presence.Count()==1&&presence.OnMap(0,5,1519).zone);
  auto old=p.m_activityPresenceToken.load();
  p.StopActivityPresence();p.inWorld=false;assert(!presence.HasPlayers()); // transfer gap
  p.map=1;p.instance=9;p.zone=100;p.x=30;p.inWorld=true;p.StartActivityPresence();
  assert(!presence.OnMap(0,5,1519).map&&presence.OnMap(1,9,100).zone);
  presence.Leave(p.id,old);presence.Move(p.id,old,{0,5,1519,{}});assert(presence.OnMap(1,9,100).zone);
  p.x=2;p.zone=101;p.UpdateActivityPosition();assert(presence.OnMap(1,9,101).zone);
  assert(presence.Nearby(1,9,{},10,true));
  p.gm=p.gmFlag=true;p.UpdateActivityGmState();assert(presence.HasPlayers()&&!presence.Nearby(1,9,{},10,true));
  p.gmFlag=false;p.UpdateActivityGmState();assert(presence.Nearby(1,9,{},10,true));
  p.x=1000;p.UpdateActivityPosition();WorldObject camera;camera.x=1;
  p.UpdateActivityCamera(&camera);assert(presence.Nearby(1,9,{},10,true));
  assert(!presence.Nearby(1,9,{},10,false));camera.x=1000;p.UpdateActivityCamera(&camera);
  assert(!presence.Nearby(1,9,{},10,true));camera.x=1;p.UpdateActivityCamera(&camera);
  p.UpdateActivityCamera(&p);assert(!presence.Nearby(1,9,{},10,true));
  p.StopActivityPresence();p.inWorld=false;assert(!presence.HasPlayers()); // logout, socket stays open
  p.UpdateActivityPosition();p.UpdateActivityCamera(&camera);assert(!presence.HasPlayers());
  p.inWorld=true;p.StartActivityPresence();assert(presence.HasPlayers()); // next login
  p.StopActivityPresence();p.session=&botSession;assert(p.inWorld&&!presence.HasPlayers()); // lost socket, body remains
  p.StartActivityPresence();assert(!presence.HasPlayers());
  p.session=&client;p.StartActivityPresence();assert(presence.Count()==1); // reconnect
  p.StopActivityPresence();socket.closing=true;p.StartActivityPresence();assert(!presence.HasPlayers());
  socket.closing=false;client.logout=true;p.StartActivityPresence();assert(!presence.HasPlayers());client.logout=false;
 }
 assert(!presence.HasPlayers());
 // A delayed logout/update for the old lifetime cannot remove/move its replacement.
 PlayerActivityPresence index;auto a=index.Enter(10,{});auto b=index.Enter(10,{{1,2,3,{}},{},false,false});
 index.Leave(10,a);index.Move(10,a,{9,9,9,{}});assert(index.Count()==1&&index.OnMap(1,2,3).zone);index.Leave(10,b);
 // No thread registry/capacity, no dropped readers/writers. Sequential thread
 // churn exceeds the old collector's 64 registrations; eight concurrent workers.
 for(unsigned wave=0;wave<20;++wave)
 {
  std::vector<std::thread> workers;
  for(unsigned i=0;i<8;++i)workers.emplace_back([&,i]{
   auto t=index.Enter(i,{{i,1,1,{}},{},false,false});
   for(unsigned k=0;k<500;++k){index.Move(i,t,{i,1,k%3,{}});index.Nearby(i,1,{},10,true);index.Contains(i);}
   index.Leave(i,t);
  });
  for(auto& worker:workers)worker.join();assert(index.Count()==0);
 }
 std::cout<<"Lifecycle: 7 categories, character screen/transfer/reconnect/disconnect/GM/camera/generation; 160 worker lifetimes passed\n";
}
}

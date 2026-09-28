// Included in each variant namespace. Only world/IO services are fixtures.
struct Group {};
struct Engine;
struct World {
 bool group=false, logInGroupOnly=false, perfMonEnabled=false;
 std::string fileOutput,logOutput;
 std::vector<std::string> trace;
 std::function<void(const char*)> callback;
 uint64_t sink=0;
 unsigned calls=0;
 void Hit(const char* label) {
#ifdef V23_TRACE
  trace.emplace_back(label);++calls;
#endif
  if(callback) callback(label);
 }
 void Consume(const char* text) { for(const unsigned char* p=(const unsigned char*)text;*p;++p) sink=sink*131+*p; }
};
inline World* world=nullptr;
struct ConfigProxy { bool& groupOnly() {return world->logInGroupOnly;} };
// Preserve the production field access without substituting the method body.
#define sPlayerbotAIConfig (*world)
struct Player {
 Group group;
 Group* GetGroup(){world->Hit("group");return world->group?&group:nullptr;}
 const char* GetName(){world->Hit("name");return "fixture-bot";}
};
struct PlayerbotAI {
 Player bot;
 Player* GetBot(){world->Hit("bot");return &bot;}
};
constexpr int LOG_BASIC=1,LOG_LVL_DETAIL=2;
struct Logger {
 __declspec(noinline) void Out(int type,int level,const char* format,...) {
  assert(type==LOG_BASIC && level==LOG_LVL_DETAIL);
  char buffer[2048];va_list args;va_start(args,format);
  std::vsnprintf(buffer,sizeof(buffer),format,args);va_end(args);
  world->Hit("log");
#ifdef V23_TRACE
  world->logOutput+=buffer;world->logOutput+='\n';
#endif
  world->Consume(buffer);
 }
};
inline Logger sLog;
struct FILE {};
inline FILE file;
inline FILE* fopen(const char* path,const char* mode) {
 assert(std::strcmp(path,"test.log")==0 && std::strcmp(mode,"a")==0);
 world->Hit("open");return &file;
}
inline int fprintf(FILE* handle,const char* format,...) {
 assert(handle==&file);
 char buffer[2048];va_list args;va_start(args,format);
 int count=std::vsnprintf(buffer,sizeof(buffer),format,args);va_end(args);
 world->Hit("write");
#ifdef V23_TRACE
 world->fileOutput+=buffer;
#endif
 world->Consume(buffer);return count;
}
inline int fclose(FILE* handle){assert(handle==&file);world->Hit("close");return 0;}
struct Engine {
 PlayerbotAI* ai;
 std::string lastAction;
 bool testMode=false;
 explicit Engine(PlayerbotAI* ai):ai(ai){}
 void LogAction(const char* format,...);
};


#ifndef __LUA_VISION_H__
#define __LUA_VISION_H__


#include <functional>
#include <lua.hpp>
#include <string>

using ResourceProvider = std::function<bool (const std::string path, std::string&)>;
auto setResourceProvider(ResourceProvider provider) -> void;
constexpr auto AUTOLUA_FIND_ORDER_NAME="FindOrder";
#ifdef __cplusplus
extern "C"
{
#endif


int luaopen_slv(struct lua_State*L);
void pushBitmapMetatable(struct lua_State*L);
using CompareColorMethodReceiver = void (*)(const char *, lua_CFunction, void *);
void eachCompareColorMethod(CompareColorMethodReceiver receiver, void*data);
void eachCompareColorMethodByUpData(CompareColorMethodReceiver receiver, void *data);
int injectOther(struct lua_State*L);
#ifdef __cplusplus
}
#endif
#endif // __LUA_VISION_H__
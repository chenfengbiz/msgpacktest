#include <stdio.h>
#include <lua.hpp>
#include <msgpack.hpp>
#include <string>
#include <iostream>
#include <sstream>
#include <google/profiler.h>
#include <sys/time.h>

int on_data(lua_State *l, std::string &str) {
    msgpack::object_handle oh = msgpack::unpack(str.data(), str.size());
    msgpack::object mo = oh.get();
    if (mo.type == msgpack::type::ARRAY) {
        for(int i = 0; i < mo.via.array.size; ++i) {
            msgpack::object *pmo = mo.via.array.ptr + i;
            switch(pmo->type) {
                case msgpack::type::BOOLEAN:
                    lua_pushboolean(l, pmo->via.boolean);
                break;
                case msgpack::type::POSITIVE_INTEGER:
                    lua_pushinteger(l, pmo->via.u64);
                break;
                case msgpack::type::NEGATIVE_INTEGER:
                    lua_pushinteger(l, pmo->via.i64);
                break;
                case msgpack::type::STR:
                    lua_pushlstring(l, pmo->via.str.ptr, pmo->via.str.size);
                break;
                default:
                    printf("unsupported type sub[%d]=%d\n", i, pmo->type);
                break;
            }
        }
        return mo.via.array.size;
    } else {
        printf("unsupported type %d\n", mo.type);
        return 0;
    }
}

int preloadLuaModule(lua_State *l) {
    lua_pushvalue(l, lua_upvalueindex(1));
    return 1;
}

void registerModule(lua_State *l) {
    lua_getglobal(l, "package");
    lua_getfield(l, -1, "preload");
    lua_newtable(l);
    lua_setfield(l, LUA_REGISTRYINDEX, "testpkg");
    lua_getfield(l, LUA_REGISTRYINDEX, "testpkg");
    lua_pushcclosure(l, preloadLuaModule, 1);
    lua_setfield(l, -2, "testpkg");
}

void loadFile(lua_State *l) {
    if luaL_dofile(l, "../init.lua") {
        lua_getglobal(l, "debug");
        lua_getfield(l, -1, "traceback");
        lua_pcall(l, 0, 1, 0);
        printf("error:%s\n", lua_tostring(l, -1));
    }
}

void unpackc(lua_State *l, int entityId, std::string &str) {
    int top = lua_gettop(l);
    lua_getfield(l, LUA_REGISTRYINDEX, "testpkg");
    lua_getfield(l, -1, "onGameDataEx");
    lua_pushinteger(l, entityId);
    int n = on_data(l, str) + 1;
    lua_pcall(l, n, 1, 0);
    const char * result = lua_tostring(l, -1);
    if (strcmp("100200", result)) {
        printf("unpackc wrong %s\n", result);
    }
    //printf("result1=%s\n", result);
    lua_settop(l, top);
}

void unpacklua(lua_State *l, int entityId, std::string &str) {
    int top = lua_gettop(l);
    lua_getfield(l, LUA_REGISTRYINDEX, "testpkg");
    lua_getfield(l, -1, "onGameData");
    lua_pushinteger(l, entityId);
    lua_pushstring(l, str.c_str());
    lua_pcall(l, 2, 1, 0);
    const char * result = lua_tostring(l, -1);
    if (strcmp("100200", result)) {
        printf("unpackc wrong %s\n", result);
    }
    //printf("result2=%s\n", result);
    lua_settop(l, top);
}

int main(int argc, char **argv) {
    // prepare the msgpack string
    int methodId = 1;
    msgpack::type::tuple<int, int, int> src(methodId, 100, 200);
    std::stringstream buffer;
    msgpack::pack(buffer, src);
    buffer.seekg(0);
    std::string str(buffer.str());

    msgpack::type::tuple<int, int, int, std::string, int> src2(methodId, 100, 200, "sample", 555);
    std::stringstream buffer2;
    msgpack::pack(buffer2, src2);
    buffer2.seekg(0);
    std::string str2(buffer2.str());

    // env init
    lua_State *l = luaL_newstate();
    luaL_openlibs(l);
    registerModule(l);
    loadFile(l);
    int entityId = 1;
    // run test
    int loopcnt = 100000;
    struct timeval time1,time2;
    //ProfilerStart("unpack");
    gettimeofday(&time1, NULL);
    for(int i = 0; i < loopcnt; ++i) {
        unpackc(l, entityId, str);
        unpackc(l, entityId, str2);
    }
    gettimeofday(&time2, NULL);
    printf("unpackc time=%d\n", (time2.tv_sec - time1.tv_sec) * 1000000 + (time2.tv_usec - time1.tv_usec));
    gettimeofday(&time1, NULL);
    for(int i = 0; i < loopcnt; ++i) {
        unpacklua(l, entityId, str);
        unpacklua(l, entityId, str2);
    }
    gettimeofday(&time2, NULL);
    printf("unpacklua time=%d\n", (time2.tv_sec - time1.tv_sec) * 1000000 + (time2.tv_usec - time1.tv_usec));
    //ProfilerStop();
    lua_close(l);
    return 0;
}

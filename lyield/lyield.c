#include "lyield.h"

#include "lua/lua.h"
#include "lua/lauxlib.h"
#include <stdlib.h>

#define IDX_QUEUE1  1
#define IDX_QUEUE2  2
#define IDX_SWITCH  3

static int resume(lua_State* L, int idx); // re-queue if true

extern int luaopen_lyield(lua_State* L){
	lua_pushlightuserdata(L, (void*)&lyield_run);
    lua_newtable(L);
    lua_newtable(L);
    lua_rawseti(L, -2, IDX_QUEUE1);
	lua_rawset(L, LUA_REGISTRYINDEX);
    
    // TODO:
    // lyield.queue
    luaL_Reg entris[] = {
        //{"resume", coresume},
        {"resume", lyield_resume},
        {"run", lyield_run},
        {NULL, NULL}
    };

    luaL_register(L, "lyield", entris);
    return 0;
}

extern int lyield_resume(lua_State* L){
    if(lua_isfunction(L, 1)){
        lua_State* nL = lua_newthread(L);
        lua_insert(L, 1);
        lua_xmove(L, nL, lua_gettop(L)-1);
    }else if(lua_isthread(L, 1)){
        luaL_argcheck(L, lua_gettop(L) == 1, 1, "only one coroutine can be queued at a time");
    }
    
	lua_pushlightuserdata(L, (void*)&lyield_run);
	lua_rawget(L, LUA_REGISTRYINDEX);
    
    if(!lua_istable(L, -1))
        luaL_error(L, "lyield not init?");

    lua_rawgeti(L, -1, IDX_SWITCH);
    int first = lua_toboolean(L, -1);
    lua_pop(L, 1);
    int queue = first ? IDX_QUEUE1 : IDX_QUEUE2;

    lua_rawgeti(L, -1, queue);
    lua_pushvalue(L, 1);
    lua_pushboolean(L, 1);
    lua_rawset(L, -3);
	return 0;
}

extern int lyield_run(lua_State* L){
    int num_queued = 0;

	lua_pushlightuserdata(L, (void*)&lyield_run);
	lua_rawget(L, LUA_REGISTRYINDEX);

    if(!lua_istable(L, -1))
        luaL_error(L, "lyield not init?");
    
    lua_rawgeti(L, -1, IDX_SWITCH);
    int first = lua_toboolean(L, -1);
    lua_pop(L, 1);
    lua_pushboolean(L, !first);
    lua_rawseti(L, -2, IDX_SWITCH); // switch queue
    
    // # table
    int queue1 = first ? IDX_QUEUE1 : IDX_QUEUE2;
    int queue2 = !first ? IDX_QUEUE1 : IDX_QUEUE2;
    lua_rawgeti(L, -1, queue2);
    if(!lua_istable(L, -2)) {
        lua_pop(L, 1);      // pop nil
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_rawseti(L, -3, queue2);
    }

    lua_rawgeti(L, -2, queue1);
    if(lua_istable(L, -1)){
        int t = lua_gettop(L);
        lua_pushnil(L);
        while(lua_next(L, t) != 0){
            // # tab2, tab1, coroutine, true
            if(resume(L, -2)){
                lua_pushvalue(L, -2);
                lua_insert(L, t+2);      // # tab2, tab1, coroutine, coroutine, true
                lua_rawset(L, t-1);
                ++num_queued;
            }else
                lua_pop(L, 1);
        }
    }
    lua_pop(L, -2);

    lua_newtable(L);
    lua_rawseti(L, -2, queue1);
    
    lua_pushnumber(L, num_queued);
	return 1;
}

static int pollAndClear(lua_State* L) {
	// evaluate every value in the stack
	for(int cur = lua_gettop(L); cur>0; --cur){
		if(lua_isfunction(L, cur)){	// function as a polling method
			lua_pushvalue(L, cur);		// push the function
            int base = lua_gettop(L) - 1;    // minus func
			int ret = lua_pcall(L, 0, LUA_MULTRET, 0); // TODO: error check
            int na = lua_gettop(L) - base;
			if(ret == 0 && na > 0){
				if(!lua_toboolean(L, -na)){// check the first one
					lua_pop(L, na);
					continue;
				}else{	// a 'true' value
					lua_pop(L, na);		// pop all results
					lua_remove(L, cur);		// remove the function
				}
			}else if(ret == 0){
				// nothing returns, ignore and keep waiting
			}else{ // error?
				// todo: trace back
                printf("error: %s", lua_tostring(L, -1));
				lua_pop(L, 1);		// pop the error
				lua_remove(L, cur);	// remove the function
			}
		}else if(lua_isthread(L, cur)){	// wait until coroutine is finished
			// coroutine is finished if the status is 0 and the stack is empty
            // or there is an error in the coroutine
            lua_State* cL = lua_tothread(L, cur);
			if((lua_status(cL) == 0 && lua_gettop(L) == 0 )||
               lua_status(cL) != LUA_YIELD){
				lua_remove(L, cur);	// remove the coroutine
			}
		}else if(lua_isuserdata(L, cur)){ 	// call metamethod __block if any
            int base = lua_gettop(L);
			int ret = luaL_callmeta(L, cur, "__block");
			if(ret <= 0 ||  // no meta table or meta method found or no return
               !lua_toboolean(L, base)){ // should not block?
				lua_remove(L, cur);
			}
			lua_settop(L, base);
			//todo: error handle?
		}else{ // for any other value, simply return it
			lua_remove(L, cur);
        }
	}
    
	return lua_gettop(L) == 0 ? 1 : 0;
}

static int resume(lua_State* L, int idx){
 	lua_State *cL = lua_tothread(L, idx);
	int status = lua_status(cL);
    
	if(status == 0 && !lua_isfunction(cL, 1))
		return 0;	// expect a funciont on a regular coroutine, otherwise drop it
	else if(status == 0 ||	// about to start
			(status == LUA_YIELD && pollAndClear(cL))){ // yielded but all events are cleared
		// function at the bottom for the first time
		int ret = lua_resume(cL, lua_gettop(L) - (status == 0 ? 1 : 0));
		if(ret == 0) {
			// the coroutine is finished and
			// clear the stack because they
			// won't go anywhere and an indicator
			// that it is finished
			lua_settop(cL, 0);
			return 0;
		}else if(ret == LUA_YIELD){
			return 1;	// evaluate in the next loop?
		}else{
			// todo: error trace back
            printf("%s", lua_tostring(cL, -1));
            return 0;
		}
	}else
        return 1;   // polling&waiting
}


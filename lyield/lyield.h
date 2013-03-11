#ifndef _LYIELD_H
#define _LYIELD_H

#if defined(__cplusplus)
extern "C" {
#endif
typedef struct lua_State lua_State;

/* open the lib */
int luaopen_lyield(lua_State*);

/* queue the coroutine or function (all the arguments can be passed followed by the function*/
int lyield_resume(lua_State*);

/* run the main scheduler; returns number of coroutines in the queue */
int lyield_run(lua_State*);

#if defined(__cplusplus)
}
#endif

#endif

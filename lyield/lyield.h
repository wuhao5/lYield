#ifndef _LYIELD_H
#define _LYIELD_H

#if defined(__cplusplus)
extern "C" {
#endif
struct lua_State;

/* open the lib */
void lyield_open(lua_State*);

/* queue the coroutine or function on top of the stack */
int lyield_queue(lua_State*);

/* run the main scheduler; returns number of coroutines in the queue */
int lyield_run(lua_State* );

#if defined(__cplusplus)
}
#endif

#endif

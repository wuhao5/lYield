lYield
======

Yielded coroutines scheduler

This toolkit essentially provides a simpler way to cooperate between Lua scripting and native framework by yielding. Also
an asynchronse operation can be a 'signal' that 'triggers' the script to simplify multi-threaded programs.

### void lyield_open(lua_State* L);
register/init the state

### void lyield_queue(lua_State* L);

it will queue the corouine on top of the stack of L. The coroutine is a state where the function is at the bottom and arguments are on the top.

it could also be a suspended coroutine.

since a scheduled coroutine can be resumed by lua_resume, in the next run loop, the schedule coroutine will continue from the last suspended point,
which could be unintended.

### int lyield_run(lua_State* L);

run the scheduler until every corouine in the queue is in pending.

### lyield.queue(coroutine/function)
manually queues the coroutine or the newly created coroutine from the function

Planned:
* lyield.queued, all queued coroutines

======
when the coroutine yielded the followings, it will automatically be pushed back to the queue:

* 1. a function, be queued if returns false
* 2. a coroutine, be queued unless the coroutine is finished
* 3. a userdata that has a __block metamethod, be queued if __block function returns true
* 4. null, be queued until manual re-queue by lyield.queue or lyield_queue
* 5. any other values will simply re-queue the coroutine (yielding to the next loop)

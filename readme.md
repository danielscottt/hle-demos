hle-demos
========

Some progams that showcase the performance benefits of Intel® HLE

## What is HLE?

HLE is [hardware lock elision](https://software.intel.com/en-us/node/514081). HLE omits lock acquisition if there is no contention on in the _critical section_ (section bookended by lock and unlock).

## building

This only works on OS X / BSD right now, but if you are on OS X 10.10+, then all it should take is `make`.

`make` builds two binaries:

#### avg

avg spawns a bunch of threads in two pools which share the same locks, one with HLE and one without.  The time spent holding these locks is monitored and then compared.

#### cthreads

cthreads is a simple example.  Four threads are spawned each with sharing the same lock.  While holding this lock, the threads sleep for 1 second.  In one scenario, the lock is hle-enabled.  The result should be a total runtime of ~1 second.  In the other, the lock should be serialized, resulting in a ~4 second runtime.

## disclaimer

You've come this far.  Neither of these examples, at the time of writing this, prove to be successful.  I will update ths repo as I find ways to demo hle.  I'm also blogging about it, so more on that later.

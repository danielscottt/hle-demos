hle-demo
========

A C progam that takes a bunch of locks in small thread pools pinned to a core, and showcases the performance benefits of Intel® HLE.

## What is HLE?

HLE is [hardware lock elision](https://software.intel.com/en-us/node/514081). HLE omits lock acquisition if there is no contention on in the _critical section_ (section bookended by lock and unlock).

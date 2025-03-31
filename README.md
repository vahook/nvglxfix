# nvglxfix

## What's this

A possible fix for the random nvidia linux driver hangs on X. Might also be related to https://github.com/ValveSoftware/gamescope/issues/1592 (NV 4924590) .

## Using it

Just add the library to `LD_PRELOAD`, something like this:
```
LD_PRELOAD="/path/to/libnvglxfix.so ${LD_PRELOAD}" my_application
```
Note that it should be first in the preload chain, as otherwise something else might resolve the real `dlsym` before we load, making our attempt at hooking it fail. (This is usually the case with wine / proton).

## Building

```sh
make
```

or

```sh
mkdir build && cd build
cmake ..
```
